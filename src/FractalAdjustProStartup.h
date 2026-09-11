// Hub startup lighting, independent of regular profiles. AI-generated.
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <vector>

struct FractalStartupEffect
{
    unsigned int kind = 3; // Meshify 3, Fade in 4, No effect 5, Off 10.
    unsigned int preset = 153;
    unsigned int brightness = 50;
    unsigned char color[3] = {255,255,255};
};

inline bool FractalStartupPacket(const FractalStartupEffect& effect, unsigned int leds,
                                 std::vector<unsigned char>& header, std::vector<unsigned char>& program)
{
    header.clear(); program.clear();
    if(leds == 0 || leds > 255 || effect.brightness > 100 || effect.preset > 255 ||
       (effect.kind != 3 && effect.kind != 4 && effect.kind != 5 && effect.kind != 10)) return false;
    const unsigned int meshify[] = {0,138,178,218,258,318};
    const unsigned int fade[] = {0,80,130,150};
    const unsigned int off[] = {0,562,1124};
    unsigned int frames = effect.kind == 3 ? 6 : effect.kind == 4 ? 4 : effect.kind == 10 ? 3 : 0;
    for(unsigned int f = 0; f < frames; f++)
    {
        unsigned int time = effect.kind == 3 ? meshify[f] : effect.kind == 4 ? fade[f] : off[f];
        program.push_back(time >> 8); program.push_back(time & 255);
        for(unsigned int i = 0; i < 3 * leds; i++)
        {
            unsigned int value = (effect.color[i % 3] * effect.brightness + 50) / 100;
            if(effect.kind == 10 || (effect.kind == 3 && (f == 2 || f == 3)) || (effect.kind == 4 && f == 0)) value = 0;
            if(effect.kind == 4 && f == 1) value /= 3;
            program.push_back(value);
        }
    }
    header.assign(64, 0);
    header[0] = 2; header[1] = 0xA4; header[2] = 0x11;
    header[3] = 0; // Startup slot. Normal lighting always uses slot 1.
    for(unsigned char value : program) header[4] += value;
    header[5] = program.size() >> 8; header[6] = program.size() & 255;
    header[8] = 1; // One selected accessory, normal save (not preview).
    header[9] = effect.preset; header[10] = effect.kind; header[11] = 100;
    if(effect.kind != 10) std::copy_n(effect.color, 3, header.begin() + 12);
    header[15] = effect.brightness;
    return true;
}
