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

class FractalAdjustProController
{
public:
    FractalAdjustProController(hid_device* dev, const char* path);
    ~FractalAdjustProController();
    bool Initialize();
    bool Apply(unsigned int target, unsigned int mode, unsigned int brightness, unsigned int speed,
               const unsigned char* colors, unsigned int color_count);
    std::string location;
    std::string firmware;
    std::string serial;
    std::vector<FractalAdjustProTarget> targets;
    std::vector<FractalAdjustProEffect> effects;

private:
    bool ReadEffects();
    bool Exchange(const unsigned char* request, unsigned char* reply);
    bool Query(unsigned char family, unsigned char command, unsigned char argument, unsigned char* reply);
    hid_device* device;
    std::mutex mutex;
};
