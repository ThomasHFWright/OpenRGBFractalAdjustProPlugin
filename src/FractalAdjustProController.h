/*---------------------------------------------------------*\
| FractalAdjustProController.h                              |
| Experimental RGB HID transport, AI-generated              |
| 11 Sep 2026                                              |
| SPDX-License-Identifier: GPL-2.0-or-later                 |
\*---------------------------------------------------------*/
#pragma once

#include <hidapi.h>
#include <mutex>
#include "FractalAdjustProProtocol.h"
#include "FractalAdjustProStartup.h"

class FractalAdjustProController
{
public:
    FractalAdjustProController(hid_device* dev, const char* path);
    ~FractalAdjustProController();
    bool Initialize();
    bool ReadStartup(unsigned int target, FractalStartupEffect& effect);
    bool ApplyStartup(unsigned int target, const FractalStartupEffect& effect);
    bool Apply(unsigned int target, unsigned int mode, unsigned int brightness, unsigned int speed,
               const unsigned char* colors, unsigned int color_count, FractalThemes::Wave wave = {});
    std::string location;
    std::string firmware;
    std::string serial;
    std::vector<FractalAdjustProTarget> targets;
    std::vector<FractalAdjustProEffect> effects;

private:
    bool Upload(const std::vector<unsigned char>& header, const std::vector<unsigned char>& program, bool commit);
    bool ReadEffects();
    bool Exchange(const unsigned char* request, unsigned char* reply);
    bool Query(unsigned char family, unsigned char command, unsigned char argument, unsigned char* reply);
    hid_device* device;
    std::mutex mutex;
};
