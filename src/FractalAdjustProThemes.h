// Fractal regular-lighting themes. Independently encoded; AI-generated.
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace FractalThemes
{
constexpr unsigned int FirstMode = 5;
struct Theme
{
    const char* name;
    unsigned char preset, kind, speed, color_count;
    unsigned char colors[18];
    unsigned char up = 6, width = 3, down = 6, frequency = 100;
};
inline constexpr Theme themes[] = {
    {"Northern lights", 0, 0, 40, 6, {0,116,197,110,0,250,0,250,235,201,64,189,149,253,228,0,240,60}},
    {"Summer sky", 1, 0, 35, 6, {255,244,168,250,154,0,250,21,0,176,64,201,149,158,253,255,255,255}},
    {"Sunset", 2, 0, 75, 6, {198,0,0,198,0,0,229,95,0,229,95,0,193,0,22,244,0,163}},
    {"Starfall", 13, 7, 3, 2, {0,14,7,119,255,34}},
    {"Glistening ice", 12, 6, 15, 6, {40,208,255,19,243,255,0,183,244,255,255,255,193,240,255,0,32,219}},
    {"Pink sapphire", 14, 8, 75, 2, {255,0,89,0,0,0}},
    {"Lunar mist", 15, 8, 75, 2, {155,155,155,0,0,0}},
    {"Mystic night", 3, 0, 60, 6, {86,0,85,154,0,250,250,0,204,67,20,71,172,3,210,60,0,66}},
    {"Campfire", 11, 6, 50, 6, {255,10,0,10,5,0,255,30,0,255,50,0,255,80,0,10,10,5}},
    {"Radiant dawn", 5, 0, 45, 6, {255,162,91,255,145,35,255,145,34,255,195,91,255,160,35,255,195,91}},
};
constexpr unsigned int Count = sizeof(themes) / sizeof(themes[0]);
inline constexpr Theme extra_presets[] = {
    {"Emerald lake", 4, 0, 75, 6, {0,96,14,0,86,10,0,122,40,84,206,108,2,125,30,0,76,16}},
    {"Fractal blue", 6, 1, 100, 1, {0,116,197}},
    {"Bonfire", 16, 1, 100, 1, {193,25,0}},
    {"Ocean", 17, 1, 100, 1, {172,203,230}},
    {"Pine", 18, 1, 100, 1, {0,87,17}},
    {"Fractal blue", 7, 2, 30, 2, {0,116,197,0,116,197}},
    {"Twilight Forest", 19, 2, 35, 2, {82,255,151,0,97,31}},
    {"Skyward", 20, 2, 73, 2, {193,24,0,0,0,0}},
    {"Misty Fade", 21, 2, 65, 2, {172,203,230,56,173,255}},
    {"Cedar Smoke", 23, 7, 4, 2, {0,0,0,255,130,35}, 4, 7, 2, 70},
    {"Maple", 24, 7, 8, 2, {142,30,0,255,124,25}, 6, 10, 7, 95},
    {"Deep Sea", 25, 7, 5, 2, {0,20,20,31,173,255}, 12, 2, 10, 100},
    {"Nautical", 26, 8, 92, 2, {28,8,128,255,255,255}},
    {"Shoreline", 27, 8, 92, 2, {255,99,15,198,56,0}},
    {"Endless Woodland", 28, 8, 100, 2, {66,255,101,0,112,20}},
    {"Azure Horizon", 29, 8, 100, 2, {238,124,62,219,219,219}},
    {"Flowing Spark", 30, 8, 100, 2, {229,0,0,0,183,70}},
    {"Rustling Leaves", 22, 6, 15, 6, {56,23,0,255,129,34,255,129,34,36,10,0,0,0,0,117,27,0}},
};
constexpr unsigned int PresetCount = Count + sizeof(extra_presets) / sizeof(extra_presets[0]);
inline const Theme& Preset(unsigned int i) { return i < Count ? themes[i] : extra_presets[i - Count]; }
constexpr unsigned int Shift = FirstMode + Count;
constexpr unsigned int Waves = Shift + 1;
constexpr unsigned int TwoColorFade = Shift + 2;
constexpr unsigned int LavaLamp = Shift + 3;
constexpr unsigned int ModeCount = Shift + 4;
struct Wave
{
    unsigned int up = 6, width = 3, down = 6, frequency = 100;
    bool Valid() const { return up <= 25 && width <= 25 && down <= 25 && frequency <= 100; }
    // API 4 has no custom parameter fields. Its unused direction word is serialized
    // by both native profiles and SDK mode updates. Waves advertises no direction flag.
    unsigned int Pack() const { return up | (width << 8) | (down << 16) | (frequency << 24); }
    static Wave Unpack(unsigned int value) { return {value & 255, (value >> 8) & 255, (value >> 16) & 255, value >> 24}; }
};
inline Theme Custom(unsigned int mode, const unsigned char* colors)
{
    unsigned char kind = mode == Shift ? 0 : mode == Waves ? 7 : mode == TwoColorFade ? 8 : 6;
    Theme theme = {"Custom", 153, kind, 50, (unsigned char)(kind == 0 || kind == 6 ? 6 : 2), {}};
    std::copy_n(colors, theme.color_count * 3, theme.colors);
    return theme;
}

inline std::vector<unsigned char> Metadata(const Theme& theme, unsigned int brightness, unsigned int speed, Wave wave = {})
{
    std::vector<unsigned char> result = {theme.preset, theme.kind, (unsigned char)speed};
    result.insert(result.end(), theme.colors, theme.colors + 3 * theme.color_count);
    if(theme.kind == 7)
        for(unsigned int value : {wave.up, wave.width, wave.down, 100 - wave.frequency}) result.push_back(value);
    if(theme.kind == 8) result.push_back(speed); // Firmware movement speed, separate from timestamps.
    result.push_back(brightness);
    if(theme.kind == 7)
        for(unsigned int c = 3; c < 6; c++) result.push_back(std::round(theme.colors[c] * (brightness / 100.0)));
    return result;
}

inline void Encode(const Theme& theme, unsigned int leds, unsigned int brightness, unsigned int speed,
                   unsigned int rotation, bool mirror, std::vector<unsigned char>& program)
{
    std::array<std::array<unsigned char, 3>, 6> palette{};
    for(unsigned int i = 0; i < theme.color_count; i++)
        for(unsigned int c = 0; c < 3; c++)
            palette[i][c] = std::round(theme.colors[i * 3 + c] * (brightness / 100.0));
    // Stable phase/noise keeps profile reloads reproducible; the vendor uses Math.random().
    uint32_t seed = 1 + (static_cast<unsigned int>(theme.preset) << 8) + leds;
    auto random = [&]() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed / 4294967296.0; };
    auto append = [&](unsigned int time, const std::vector<unsigned char>& frame)
    {
        program.push_back(time >> 8);
        program.push_back(time & 255);
        program.insert(program.end(), frame.begin(), frame.end());
    };
    auto transform = [&](const std::vector<unsigned char>& frame)
    {
        std::vector<unsigned char> result(frame.size());
        for(unsigned int i = 0; i < leds; i++)
        {
            unsigned int dest = (i + (unsigned int)std::round(leds * rotation / 8.0)) % leds;
            if(mirror) dest = (leds - dest) % leds;
            std::copy_n(frame.begin() + 3 * i, 3, result.begin() + 3 * dest);
        }
        return result;
    };
    if(theme.kind == 0) // Shift: six spatial palettes and an interpolated loop boundary.
    {
        std::array<unsigned int, 6> order{};
        const unsigned int start = (unsigned int)(random() * 6);
        for(unsigned int i = 0; i < 6; i++) order[(start + i) % 6] = i;
        std::vector<std::vector<unsigned char>> frames;
        constexpr unsigned int transition[] = {4,5,3,0,1,2};
        for(unsigned int f = 0; f < 6; f++)
        {
            std::vector<unsigned char> frame(3 * leds);
            unsigned int band = 0;
            for(unsigned int i = 0; i < leds; i++)
            {
                double fraction = i * (6.0 / leds) - band;
                if(fraction > 1) { band++; fraction = i * (6.0 / leds) - band; }
                for(unsigned int c = 0; c < 3; c++)
                {
                    double value = palette[order[band]][c] +
                        (palette[order[(band + 1) % 6]][c] - palette[order[band]][c]) * ((100 * fraction) / 100);
                    frame[3 * i + c] = std::round((std::clamp(value, 0.0, 255.0) / 255.0) * 255.0);
                }
            }
            frames.push_back(frame);
            const auto previous = order;
            for(unsigned int i = 0; i < 6; i++) order[i] = previous[transition[i]];
        }
        unsigned int shift = ((unsigned int)(random() * 6) * 3 * leds) % 6;
        std::rotate(frames.begin(), frames.end() - shift, frames.end());
        unsigned int step = 120 - speed;
        unsigned int offset = (unsigned int)(step * random());
        std::vector<unsigned char> boundary(3 * leds);
        for(unsigned int c = 0; c < boundary.size(); c++)
            boundary[c] = frames[0][c] + (frames[1][c] - frames[0][c]) * (double(offset) / step);
        append(0, transform(boundary));
        append(step - offset, transform(frames[1]));
        for(unsigned int f = 2; f < 6; f++) append(f * step, transform(frames[f]));
        append(6 * step, transform(frames[0]));
        append(6 * step + offset, transform(boundary));
    }
    else if(theme.kind == 6) // Lava Lamp: fourteen noisy keyframes, then the first frame again.
    {
        std::vector<unsigned char> first;
        unsigned int time = 0;
        for(unsigned int f = 0; f < 14; f++)
        {
            std::vector<unsigned char> frame(3 * leds);
            for(unsigned int i = 0; i < leds; i++)
            {
                unsigned int choice = (unsigned int)(12 * random());
                for(unsigned int c = 0; c < 3; c++)
                    frame[3 * i + c] = palette[choice % 6][c] / (choice < 6 ? 1 : 2);
            }
            if(leds > 15)
                for(unsigned int i = 0; i < (unsigned int)std::round(leds / 4.0); i++)
                {
                    unsigned int choice = (unsigned int)(12 * random());
                    unsigned int start = (unsigned int)(random() * (leds - 3));
                    for(unsigned int j = 0; j < 3; j++)
                        for(unsigned int c = 0; c < 3; c++)
                            frame[3 * (start + j) + c] = std::round(palette[choice % 6][c] /
                                double((choice < 6 ? 1 : 2) * (j == 1 ? 1 : 2)));
                }
            if(f == 0) first = frame;
            append(time, frame);
            time += 106 - speed + (unsigned int)(10 * random()) - 5;
        }
        append(time, first);
    }
    else if(theme.kind == 7) // Firmware renders the wave from metadata over a background program.
    {
        std::vector<unsigned char> frame(3 * leds);
        for(unsigned int i = 0; i < frame.size(); i++) frame[i] = palette[0][i % 3];
        for(unsigned int time : {0U, 562U, 1124U}) append(time, frame);
    }
    else if(theme.kind == 8) // Firmware moves the uploaded spatial gradient.
    {
        std::vector<unsigned char> frame(3 * leds);
        for(unsigned int i = 0; i < leds; i++)
        {
            double fraction = leds == 1 ? 0 : std::clamp(1.25 * std::sin(i / double(leds - 1) * 2 * std::acos(-1.0)), 0.0, 1.0);
            for(unsigned int c = 0; c < 3; c++)
                frame[3 * i + c] = std::round(palette[0][c] + (palette[1][c] - palette[0][c]) * fraction);
        }
        frame = transform(frame);
        unsigned int shift = leds == 76 ? 62 : (unsigned int)std::round(3 * leds / 8.0);
        std::rotate(frame.begin(), frame.end() - 3 * shift, frame.end());
        append(0, frame);
        append(20000, frame);
    }
}
}
