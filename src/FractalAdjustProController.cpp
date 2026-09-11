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
    return ReadEffects();
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
                                      const unsigned char* colors, unsigned int color_count)
{
    if(target >= targets.size())
    {
        return false;
    }
    std::vector<unsigned char> header;
    std::vector<unsigned char> program;
    if(!FractalAdjustProProtocol::Effect(mode, targets[target].leds, brightness, speed, colors, color_count, header, program))
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex);
    unsigned char reply[FRACTAL_REPORT_SIZE];
    if(!Query(0xA4, 0x1A, 0, reply) || reply[4] != 0)
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Dynamic Lighting active or unreadable; update refused\n");
        return false;
    }
    unsigned char select[FRACTAL_REPORT_SIZE] = {2, 0xA4, 0x0A, 1, targets[target].id};
    if(mode != FRACTAL_SAVED)
    {
        header[7] = 0; // Normal Apply: commit the selected accessory, not the shared hover preview.
    }
    if(!Exchange(select, reply) || !Exchange(header.data(), reply))
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
    if(mode != FRACTAL_SAVED)
    {
        // Like the vendor Apply action, save once per mode change, never per frame.
        if(!Query(0xA4, 0x13, 0, reply) || !Query(0xA4, 0x78, 0, reply))
        {
            return false;
        }
    }
    return true;
}
