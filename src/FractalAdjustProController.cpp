/*---------------------------------------------------------*\
| FractalAdjustProController.cpp                            |
| Experimental RGB HID transport, AI-generated              |
| 11 Sep 2026                                              |
| SPDX-License-Identifier: GPL-2.0-or-later                 |
\*---------------------------------------------------------*/
#include <chrono>
#include "FractalAdjustProController.h"
#include <cstdio>

FractalAdjustProController::FractalAdjustProController(hid_device* dev, const char* path)
{
    device = dev;
    location = std::string("HID: ") + path;
}

FractalAdjustProController::~FractalAdjustProController()
{
    StopStream();
    hid_close(device);
}

bool FractalAdjustProController::Exchange(const unsigned char* request, unsigned char* reply)
{
    if(hid_write(device, request, FRACTAL_REPORT_SIZE) != FRACTAL_REPORT_SIZE)
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] HID write failed; command %02X %02X\n", request[1], request[2]);
        return false;
    }
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    while(std::chrono::steady_clock::now() < deadline)
    {
        int remaining = (int)std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        int length = hid_read_timeout(device, reply, FRACTAL_REPORT_SIZE, std::max(remaining, 1));
        if(length <= 0)
        {
            break;
        }
        if(length >= 3 && reply[0] == 2 && reply[1] == request[1] && reply[2] == request[2])
        {
            if(FractalAdjustProProtocol::ValidReply(reply, length, request[1], request[2]))
            {
                return true;
            }
            std::fprintf(stderr, "[Fractal Adjust Pro] Invalid/error reply for %02X %02X (length %d)\n", request[1], request[2], length);
            return false;
        }
    }
    std::fprintf(stderr, "[Fractal Adjust Pro] Timeout/disconnect for %02X %02X\n", request[1], request[2]);
    return false;
}

bool FractalAdjustProController::Query(unsigned char family, unsigned char command, unsigned char argument, unsigned char* reply)
{
    unsigned char request[FRACTAL_REPORT_SIZE] = {2, family, command, argument};
    return Exchange(request, reply);
}

bool FractalAdjustProController::Initialize()
{
    std::lock_guard<std::mutex> lock(mutex);
    unsigned char reply[FRACTAL_REPORT_SIZE];
    if(!Query(0xF1, 0x01, 0, reply) || !std::memchr(reply + 4, 0, 16))
    {
        return false;
    }
    firmware = (char*)reply + 4;
    // ponytail: firmware 1.1.17 only; enable others after captures and hardware checks.
    if(firmware != "1.1.17")
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Untested firmware %s; refusing control\n", firmware.c_str());
        return false;
    }
    if(!Query(0xA4, 0x1A, 0, reply) || reply[4] != 0)
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Dynamic Lighting active or unreadable; refusing control\n");
        return false;
    }
    if(!Query(0xA4, 0x08, 0, reply) || !FractalAdjustProProtocol::Topology(reply, FRACTAL_REPORT_SIZE, targets))
    {
        return false;
    }
    wchar_t serial_wide[128] = {};
    if(hid_get_serial_number_string(device, serial_wide, 128) == 0)
    {
        for(unsigned int i = 0; i < 127 && serial_wide[i]; i++)
        {
            serial += serial_wide[i] < 128 ? (char)serial_wide[i] : '?';
        }
    }
    for(unsigned int i = 0; i < targets.size(); i++)
    {
        unsigned char select[FRACTAL_REPORT_SIZE] = {2, 0xA4, 0x0A, 1, targets[i].id};
        if(!Exchange(select, reply) || !Query(0xA4, 0x0E, 0, reply) || reply[4] > 59)
        {
            return false;
        }
        targets[i].name.assign((char*)reply + 5, reply[4]);
    }
    if(!ReadEffects()) return false;
    unsigned char attributes[23] = {3};
    if(hid_get_feature_report(device, attributes, sizeof(attributes)) == sizeof(attributes) && attributes[0] == 3)
    {
        const unsigned int count = attributes[1] | (attributes[2] << 8);
        unsigned int maximum = 0;
        for(const auto& target : targets) maximum = std::max(maximum, (unsigned int)target.leds);
        // Firmware shares one buffer across all outputs. Refuse unexpected bounds.
        if(count == maximum && count > 0 && count <= 255) stream_leds = count;
    }
    return true;
}

bool FractalAdjustProController::StopStreamLocked()
{
    if(!streaming) return true;
    const unsigned char control[] = {8, 1};
    if(hid_send_feature_report(device, control, sizeof(control)) != sizeof(control)) return false;
    streaming = false;
    return true;
}

bool FractalAdjustProController::StopStream()
{
    std::lock_guard<std::mutex> lock(mutex);
    return StopStreamLocked();
}

bool FractalAdjustProController::Stream(const std::vector<unsigned char>& rgb)
{
    if(!stream_leds || rgb.size() != stream_leds * 3) return false;
    std::lock_guard<std::mutex> lock(mutex);
    if(!streaming)
    {
        unsigned char reply[FRACTAL_REPORT_SIZE];
        if(!Query(0xA4, 0x1A, 0, reply) || reply[4] != 0) return false;
        const unsigned char control[] = {8, 0};
        // Retain ownership on uncertain writes so cleanup still resumes autonomous lighting.
        streaming = true;
        if(hid_send_feature_report(device, control, sizeof(control)) != sizeof(control))
        {
            StopStreamLocked();
            return false;
        }
    }
    // ponytail: synchronous full frames (~16 fps at 76 LEDs); dirty batches only if needed.
    for(unsigned int offset = 0; offset < stream_leds; offset += 8)
    {
        unsigned char report[51] = {6};
        const unsigned int count = std::min(8U, stream_leds - offset);
        report[1] = count;
        report[2] = offset + count == stream_leds ? 1 : 0;
        for(unsigned int i = 0; i < count; i++)
        {
            report[3 + i * 2] = (offset + i) & 255;
            report[4 + i * 2] = (offset + i) >> 8;
            std::copy_n(rgb.data() + (offset + i) * 3, 3, report + 19 + i * 4);
            report[22 + i * 4] = 255;
        }
        if(hid_send_feature_report(device, report, sizeof(report)) != sizeof(report))
        {
            StopStreamLocked();
            std::fprintf(stderr, "[Fractal Adjust Pro] Direct frame failed; resuming hardware effects\n");
            return false;
        }
    }
    return true;
}

bool FractalAdjustProController::ReadEffects()
{
    effects.assign(targets.size(), FractalAdjustProEffect());
    unsigned char reply[FRACTAL_REPORT_SIZE];
    for(unsigned int i = 0; i < targets.size(); i++)
    {
        unsigned char select[FRACTAL_REPORT_SIZE] = {2, 0xA4, 0x0A, 1, targets[i].id};
        if(!Exchange(select, reply) || !Query(0xA4, 0x05, 1, reply) ||
           !FractalAdjustProProtocol::ReadEffect(reply, FRACTAL_REPORT_SIZE, effects[i]))
        {
            std::fprintf(stderr, "[Fractal Adjust Pro] Cannot preserve current preview; use vendor app to end it first\n");
            return false;
        }
    }
    return true;
}

bool FractalAdjustProController::Apply(unsigned int target, unsigned int mode, unsigned int brightness, unsigned int speed,
                                      const unsigned char* colors, unsigned int color_count, FractalThemes::Wave wave)
{
    if(target >= targets.size())
    {
        return false;
    }
    std::vector<unsigned char> header;
    std::vector<unsigned char> program;
    if(!FractalAdjustProProtocol::Effect(mode, targets[target].leds, brightness, speed, colors, color_count, header, program, 0, false, wave))
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex);
    unsigned char reply[FRACTAL_REPORT_SIZE];
    if(!StopStreamLocked()) return false;
    if(!Query(0xA4, 0x1A, 0, reply) || reply[4] != 0)
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Dynamic Lighting active or unreadable; update refused\n");
        return false;
    }
    unsigned char select[FRACTAL_REPORT_SIZE] = {2, 0xA4, 0x0A, 1, targets[target].id};
    if(!Exchange(select, reply)) return false;
    if(mode >= FractalThemes::FirstMode)
    {
        // Spatial programs must respect existing orientation without changing it.
        if(!Query(0xA4, 0x16, 0, reply) || reply[4] > 7) return false;
        unsigned int rotation = reply[4];
        if(!Query(0xA4, 0x18, 0, reply) || reply[4] > 1) return false;
        if(!FractalAdjustProProtocol::Effect(mode, targets[target].leds, brightness, speed, colors, color_count,
                                             header, program, rotation, reply[4] != 0, wave)) return false;
    }
    if(mode != FRACTAL_SAVED)
    {
        header[7] = 0; // Normal Apply: commit the selected accessory, not the shared hover preview.
    }
    return Upload(header, program, mode != FRACTAL_SAVED);
}

bool FractalAdjustProController::Upload(const std::vector<unsigned char>& header, const std::vector<unsigned char>& program, bool commit)
{
    unsigned char reply[FRACTAL_REPORT_SIZE];
    if(!Exchange(header.data(), reply))
    {
        return false;
    }
    for(unsigned int offset = 0; offset < program.size(); offset += FRACTAL_CHUNK_SIZE)
    {
        std::vector<unsigned char> chunk = FractalAdjustProProtocol::Chunk(program, offset);
        if(!Exchange(chunk.data(), reply))
        {
            return false;
        }
    }
    if(commit)
    {
        // Like the vendor Apply action, save once per mode change, never per frame.
        if(!Query(0xA4, 0x13, 0, reply))
        {
            return false;
        }
    }
    if(header[7] == 0 && !Query(0xA4, 0x78, 0, reply)) return false;
    return true;
}

bool FractalAdjustProController::ReadStartup(unsigned int target, FractalStartupEffect& effect)
{
    if(target >= targets.size()) return false;
    std::lock_guard<std::mutex> lock(mutex);
    unsigned char reply[FRACTAL_REPORT_SIZE];
    unsigned char select[FRACTAL_REPORT_SIZE] = {2, 0xA4, 0x0A, 1, targets[target].id};
    if(!Exchange(select, reply) || !Query(0xA4, 0x05, 0, reply)) return false;
    FractalStartupEffect current;
    current.preset = reply[11]; current.kind = reply[12]; current.brightness = reply[17];
    std::copy_n(reply + 14, 3, current.color);
    std::vector<unsigned char> header, program;
    if(reply[13] != 100 || !FractalStartupPacket(current, targets[target].leds, header, program)) return false;
    if(!std::equal(header.begin() + 9, header.begin() + 16, reply + 11)) return false;
    effect = current;
    return true;
}

bool FractalAdjustProController::ApplyStartup(unsigned int target, const FractalStartupEffect& effect)
{
    if(target >= targets.size()) return false;
    std::vector<unsigned char> header, program;
    if(!FractalStartupPacket(effect, targets[target].leds, header, program)) return false;
    std::lock_guard<std::mutex> lock(mutex);
    if(!StopStreamLocked()) return false;
    unsigned char reply[FRACTAL_REPORT_SIZE];
    if(!Query(0xA4, 0x1A, 0, reply) || reply[4] != 0) return false;
    unsigned char select[FRACTAL_REPORT_SIZE] = {2, 0xA4, 0x0A, 1, targets[target].id};
    if(!Exchange(select, reply)) return false;
    // Vendor InstantBootEffect saves its zero-length header directly; no chunks or commit.
    return Upload(header, program, effect.kind != 5);
}
