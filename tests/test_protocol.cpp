/* Experimental Fractal RGB regression check. AI-generated, 11 Sep 2026.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <cassert>
#include <iostream>
#include "FractalAdjustProController.h"

// Fake HID exercises the real transport without opening any hardware.
static std::vector<std::vector<unsigned char>> writes;
static int fault = 0;
static unsigned int fail_at_write = 0;
static int closes = 0;
static bool second_target = false;
static unsigned char selected = 1;
int hid_write(hid_device*, const unsigned char* data, size_t length)
{
    writes.emplace_back(data, data + length);
    if(data[1] == 0xA4 && data[2] == 0x0A) selected = data[4];
    if(fail_at_write && writes.size() == fail_at_write) fault = 5;
    return fault == 1 ? -1 : (int)length;
}
int hid_read_timeout(hid_device*, unsigned char* data, size_t length, int milliseconds)
{
    assert(milliseconds > 0 && milliseconds <= 1000);
    if(fault == 2) return 0;
    if(fault == 3) return -1;
    std::memset(data, 0, length);
    data[0] = 2; data[1] = writes.back()[1]; data[2] = writes.back()[2];
    if(fault == 4) return 3;
    if(fault == 5) data[3] = 1;
    if(fault == 6) { data[2] ^= 1; fault = 2; return 64; }
    if(data[1] == 0xF1 && data[2] == 1) std::memcpy(data + 4, "1.1.17", 7);
    if(data[1] == 0xA4 && data[2] == 8)
    {
        data[4] = second_target ? 2 : 1; data[5] = 1; data[6] = 20; data[8] = 2; data[9] = 20;
    }
    if(data[1] == 0xA4 && data[2] == 5 && second_target && selected == 2)
    {
        const unsigned char green[] = {0x99,1,100,0,255,0,25};
        std::memcpy(data + 11, green, sizeof(green));
    }
    return 64;
}
void hid_close(hid_device*) { closes++; }
int hid_get_serial_number_string(hid_device*, wchar_t* text, size_t) { text[0] = 0; return 0; }

int main()
{
    unsigned char topology[64] = {2,0xA4,8,0,11,1,20,0,2,20,0,3,20,0,4,20,0,17,20,0,18,20,0,19,20,0,33,11,1,49,3,0,50,35,0,51,76,0};
    std::vector<FractalAdjustProTarget> targets;
    assert(FractalAdjustProProtocol::Topology(topology, 64, targets) && targets.size() == 11);
    unsigned int total = 0;
    for(const FractalAdjustProTarget& t : targets) total += t.leds;
    assert(total == 265 && targets[7].generation == 1 && targets[10].id == 51);
    for(int n = 0; n < 64; n++) assert(!FractalAdjustProProtocol::Topology(topology, n, targets));
    for(unsigned int field : {0U,1U,2U,3U,4U,5U,6U,7U})
    {
        unsigned char old = topology[field]; topology[field] = 255;
        assert(!FractalAdjustProProtocol::Topology(topology, 64, targets)); topology[field] = old;
    }
    topology[8] = 1;
    assert(!FractalAdjustProProtocol::Topology(topology, 64, targets));
    topology[8] = 2;
    topology[6] = topology[9] = 255;
    assert(!FractalAdjustProProtocol::Topology(topology, 64, targets));

    std::vector<unsigned char> header, program;
    const unsigned char red_blue[] = {255,0,0,0,0,255};
    assert(FractalAdjustProProtocol::Effect(FRACTAL_STATIC, 3, 100, 100, red_blue, 1, header, program));
    const unsigned char expected_header[] = {2,0xA4,0x11,1,0x93,0,0x21,1,1,0x99,1,100,255,0,0,100};
    const unsigned char expected_program[] = {0,0,255,0,0,255,0,0,255,0,0,2,50,255,0,0,255,0,0,255,0,0,4,100,255,0,0,255,0,0,255,0,0};
    assert(std::equal(std::begin(expected_header), std::end(expected_header), header.begin()));
    assert(program == std::vector<unsigned char>(std::begin(expected_program), std::end(expected_program)));
    assert(FractalAdjustProProtocol::Effect(FRACTAL_SAVED, 20, 100, 100, nullptr, 0, header, program));
    const unsigned char cancel[] = {2,0xA4,0x11,1,0x9C,0,6,1,0,0x99,1,100,0,0,0,50};
    assert(program.empty() && std::equal(std::begin(cancel), std::end(cancel), header.begin()));
    assert(FractalAdjustProProtocol::Effect(FRACTAL_BREATHING, 20, 50, 50, red_blue, 2, header, program));
    assert(program.size() == 558 && program[2] == 128 && program[62] == 0 && program[63] == 120);
    assert(program[124] == 0 && program[125] == 160 && program[126] == 7);
    assert(FractalAdjustProProtocol::Effect(FRACTAL_CYCLE, 76, 100, 50, nullptr, 0, header, program));
    assert(program.size() == 1610 && program[230] == 0 && program[231] == 70);
    for(unsigned int size : {1U,59U,60U,61U,120U,121U,1610U})
    {
        program.resize(size);
        for(unsigned int i = 0; i < size; i++) program[i] = i & 255;
        std::vector<unsigned char> decoded;
        for(unsigned int offset = 0; offset < size; offset += 60)
        {
            std::vector<unsigned char> packet = FractalAdjustProProtocol::Chunk(program, offset);
            assert(packet.size() == 64 && packet[3] >= 1 && packet[3] <= 60);
            decoded.insert(decoded.end(), packet.begin() + 4, packet.begin() + 4 + packet[3]);
            for(unsigned int j = 4 + packet[3]; j < 64; j++) assert(packet[j] == 0);
        }
        assert(decoded == program && FractalAdjustProProtocol::Chunk(program, size).empty());
    }
    assert(!FractalAdjustProProtocol::Effect(FRACTAL_STATIC, 0, 100, 50, red_blue, 1, header, program));
    assert(!FractalAdjustProProtocol::Effect(FRACTAL_STATIC, 256, 100, 50, red_blue, 1, header, program));
    assert(!FractalAdjustProProtocol::Effect(FRACTAL_STATIC, 20, 101, 50, red_blue, 1, header, program));
    assert(!FractalAdjustProProtocol::Effect(FRACTAL_STATIC, 20, 100, 101, red_blue, 1, header, program));
    assert(!FractalAdjustProProtocol::Effect(FRACTAL_BREATHING, 20, 100, 50, red_blue, 1, header, program));
    assert(!FractalAdjustProProtocol::Effect(99, 20, 100, 50, nullptr, 0, header, program));
    {
        FractalAdjustProController controller(nullptr, "fake");
        assert(controller.Initialize());
        writes.clear();
        assert(!controller.Apply(1, FRACTAL_STATIC, 100, 50, red_blue, 1) && writes.empty());
        assert(controller.Apply(0, FRACTAL_STATIC, 100, 50, red_blue, 1));
        assert(writes.size() == 9 && writes[1][2] == 0x0A && writes[1][4] == 1);
        for(const std::vector<unsigned char>& w : writes)
            assert(w[1] == 0xA4 && (w[2] == 0x13 || w[2] == 0x78 || w[2] == 0x1A || w[2] == 0x0A || w[2] == 0x11 || w[2] == 0x12));
        for(int failure = 1; failure <= 6; failure++)
        {
            writes.clear(); fault = failure;
            assert(!controller.Apply(0, FRACTAL_STATIC, 100, 50, red_blue, 1));
            assert(writes.size() == 1); // No blind retry or writes following a failed query.
        }
        for(unsigned int failure = 2; failure <= 9; failure++)
        {
            writes.clear(); fault = 0; fail_at_write = failure;
            assert(!controller.Apply(0, FRACTAL_STATIC, 100, 50, red_blue, 1));
            assert(writes.size() == failure); // Stop at selection/header/chunk rejection.
        }
    }
    fault = 0; fail_at_write = 0; second_target = true;
    {
        FractalAdjustProController controller(nullptr, "fake");
        assert(controller.Initialize());
        assert(controller.effects[1].mode == FRACTAL_STATIC && controller.effects[1].colors[1] == 255);
        writes.clear();
        assert(controller.Apply(0, FRACTAL_STATIC, 25, 100, red_blue, 1));
        unsigned int headers = 0;
        for(const std::vector<unsigned char>& packet : writes)
        {
            if(packet[2] != 0x11) continue;
            assert(packet[7] == 0 && packet[8] == 1); // Normal Apply preserves the other target.
            assert(packet[12 + headers] == 255 && packet[15] == 25);
            headers++;
        }
        assert(headers == 1);
        assert(writes[1][4] == 1 && writes[7][2] == 0x13 && writes[8][2] == 0x78);
        writes.clear();
        assert(controller.Apply(0, FRACTAL_SAVED, 100, 100, nullptr, 0));
        assert(writes.size() == 3 && writes[2][7] == 1 && writes[2][8] == 0);
    }
    assert(closes == 2);
    unsigned char readback[64] = {2,0xA4,5,0};
    FractalAdjustProEffect effect;
    readback[11] = 153; readback[12] = 1; readback[13] = 100; readback[14] = 255; readback[17] = 25;
    assert(FractalAdjustProProtocol::ReadEffect(readback, 64, effect) && effect.mode == FRACTAL_STATIC && effect.brightness == 25);
    assert(!FractalAdjustProProtocol::ReadEffect(readback, 63, effect));
    readback[17] = 101;
    assert(!FractalAdjustProProtocol::ReadEffect(readback, 64, effect));
    readback[17] = 25; readback[12] = 0;
    assert(!FractalAdjustProProtocol::ReadEffect(readback, 64, effect)); // Unknown vendor preview palette.
    readback[12] = 9;
    assert(!FractalAdjustProProtocol::ReadEffect(readback, 64, effect));
    std::cout << "PASS: packets, topology, bounds, chunks, HID errors/timeouts normal Apply, preview cancel, and no cooling commands\n";
}
