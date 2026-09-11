/*---------------------------------------------------------*\
| FractalAdjustProProtocol.h                                |
| Experimental RGB packet encoding, AI-generated            |
| 11 Sep 2026                                              |
| SPDX-License-Identifier: GPL-2.0-or-later                 |
\*---------------------------------------------------------*/
#pragma once

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#define FRACTAL_REPORT_SIZE 64
#define FRACTAL_CHUNK_SIZE  60

enum
{
    FRACTAL_SAVED,
    FRACTAL_STATIC,
    FRACTAL_OFF,
    FRACTAL_BREATHING,
    FRACTAL_CYCLE
};

struct FractalAdjustProTarget
{
    unsigned char id;
    unsigned char leds;
    unsigned char generation;
    std::string name;
};

struct FractalAdjustProEffect
{
    unsigned int mode = FRACTAL_SAVED;
    unsigned int brightness = 100;
    unsigned int speed = 100;
    unsigned char colors[6] = {};
};

class FractalAdjustProProtocol
{
public:
    static bool ReadEffect(const unsigned char* reply, int length, FractalAdjustProEffect& effect)
    {
        effect = FractalAdjustProEffect();
        if(!ValidReply(reply, length, 0xA4, 0x05))
        {
            return false;
        }
        const unsigned char* metadata = reply + 11;
        if(metadata[0] != 153)
        {
            return true; // Vendor saved preset: resume it rather than reconstruct it.
        }
        unsigned int color_count = 1;
        switch(metadata[1])
        {
        case 1: effect.mode = FRACTAL_STATIC; break;
        case 10: effect.mode = FRACTAL_OFF; break;
        case 2: effect.mode = FRACTAL_BREATHING; color_count = 2; break;
        case 0:
        {
            const unsigned char palette[] = {255,0,0,255,255,0,0,255,0,0,255,255,0,0,255,255,0,255};
            if(std::memcmp(metadata + 3, palette, sizeof(palette)) != 0)
            {
                return false; // An unknown preview cannot be reconstructed losslessly.
            }
            effect.mode = FRACTAL_CYCLE;
            color_count = 6;
            break;
        }
        default: return false;
        }
        effect.speed = metadata[2];
        effect.brightness = metadata[3 + color_count * 3];
        std::memcpy(effect.colors, metadata + 3, std::min(color_count, 2U) * 3);
        return effect.speed <= 100 && effect.brightness <= 100;
    }

    static bool ValidReply(const unsigned char* reply, int length, unsigned char family, unsigned char command)
    {
        return length == FRACTAL_REPORT_SIZE && reply[0] == 0x02 && reply[1] == family && reply[2] == command && reply[3] == 0;
    }

    static bool Topology(const unsigned char* reply, int length, std::vector<FractalAdjustProTarget>& targets)
    {
        targets.clear();
        if(!ValidReply(reply, length, 0xA4, 0x08) || reply[4] > 16)
        {
            return false;
        }
        unsigned int total = 0;
        for(unsigned int i = 0; i < reply[4]; i++)
        {
            unsigned int offset = 5 + 3 * i;
            FractalAdjustProTarget target;
            target.id = reply[offset];
            target.leds = reply[offset + 1];
            target.generation = reply[offset + 2] == 0 ? 2 : reply[offset + 2];
            if((target.id >> 4) > 3 || (target.id & 0x0F) < 1 || (target.id & 0x0F) > 4 ||
               target.leds == 0 || target.generation > 3)
            {
                targets.clear();
                return false;
            }
            for(unsigned int j = 0; j < targets.size(); j++)
            {
                if(targets[j].id == target.id)
                {
                    targets.clear();
                    return false;
                }
            }
            total += target.leds;
            targets.push_back(target);
        }
        if(total > 400)
        {
            targets.clear();
            return false;
        }
        return true;
    }

    static std::vector<unsigned char> Chunk(const std::vector<unsigned char>& program, unsigned int offset)
    {
        if(offset >= program.size())
        {
            return {};
        }
        unsigned int count = std::min((unsigned int)program.size() - offset, (unsigned int)FRACTAL_CHUNK_SIZE);
        std::vector<unsigned char> result(FRACTAL_REPORT_SIZE, 0);
        result[0] = 2;
        result[1] = 0xA4;
        result[2] = 0x12;
        result[3] = count;
        std::copy(program.begin() + offset, program.begin() + offset + count, result.begin() + 4);
        return result;
    }

    static bool Effect(unsigned int mode, unsigned int leds, unsigned int brightness, unsigned int speed,
                       const unsigned char* colors, unsigned int color_count,
                       std::vector<unsigned char>& header, std::vector<unsigned char>& program)
    {
        header.clear();
        program.clear();
        if(mode > FRACTAL_CYCLE || leds == 0 || leds > 255 || brightness > 100 || speed > 100 ||
           ((mode == FRACTAL_STATIC || mode == FRACTAL_BREATHING) && (!colors || color_count < (mode == FRACTAL_BREATHING ? 2U : 1U))))
        {
            return false;
        }
        unsigned char palette[6][3] = {};
        if(mode == FRACTAL_SAVED)
        {
            leds = 0;
            brightness = 50;
            speed = 100;
        }
        unsigned int frames = 3;
        unsigned int times[9] = {0, 562, 1124};
        unsigned int indices[9] = {};
        bool dim[9] = {};
        unsigned char kind = mode == FRACTAL_OFF ? 10 : 1;
        unsigned int palette_size = 1;
        if(mode == FRACTAL_STATIC || mode == FRACTAL_BREATHING)
        {
            palette_size = mode == FRACTAL_BREATHING ? 2 : 1;
            std::memcpy(palette, colors, 3 * palette_size);
        }
        if(mode == FRACTAL_BREATHING)
        {
            kind = 2;
            frames = 9;
            unsigned int n = std::max(100 - speed, 1U);
            unsigned int hold = (n * 120 + 25) / 50;
            unsigned int fade = (n * 40 + 25) / 50;
            unsigned int steps[8] = {hold, fade, 1, fade, hold, fade, 1, fade};
            for(unsigned int i = 1; i < frames; i++)
            {
                times[i] = times[i - 1] + steps[i - 1];
            }
            indices[3] = indices[4] = indices[5] = indices[6] = 1;
            dim[2] = dim[3] = dim[6] = dim[7] = true;
        }
        else if(mode == FRACTAL_CYCLE)
        {
            kind = 0;
            palette_size = 6;
            frames = 7;
            const unsigned char rainbow[6][3] = {{255,0,0},{255,255,0},{0,255,0},{0,255,255},{0,0,255},{255,0,255}};
            std::memcpy(palette, rainbow, sizeof(palette));
            for(unsigned int i = 0; i < frames; i++)
            {
                times[i] = i * (120 - speed);
                indices[i] = i % 6;
            }
        }
        for(unsigned int i = 0; i < frames; i++)
        {
            program.push_back(times[i] >> 8);
            program.push_back(times[i] & 0xFF);
            for(unsigned int led = 0; led < leds; led++)
            {
                for(unsigned int component = 0; component < 3; component++)
                {
                    unsigned int value = (palette[indices[i]][component] * brightness + 50) / 100;
                    program.push_back(dim[i] ? (value + 19) / 20 : value);
                }
            }
        }
        unsigned char checksum = 0;
        for(unsigned int i = 0; i < program.size(); i++)
        {
            checksum += program[i];
        }
        header.assign(FRACTAL_REPORT_SIZE, 0);
        header[0] = 2;
        header[1] = 0xA4;
        header[2] = 0x11;
        header[3] = 1; // Regular lighting slot; never touch startup slot 0.
        header[4] = checksum;
        header[5] = program.size() >> 8;
        header[6] = program.size() & 0xFF;
        header[7] = 1; // Preview template; transport uses normal Apply (0) except for Saved cancellation.
        header[8] = mode == FRACTAL_SAVED ? 0 : 1;
        header[9] = 153; // Vendor custom/preview preset.
        header[10] = kind;
        header[11] = mode == FRACTAL_STATIC || mode == FRACTAL_OFF ? 100 : speed;
        std::memcpy(header.data() + 12, palette, palette_size * 3);
        header[12 + palette_size * 3] = brightness;
        if(mode == FRACTAL_SAVED)
        {
            program.clear(); // Target count zero cancels preview; no transfer.
        }
        return true;
    }
};
