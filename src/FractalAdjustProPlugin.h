// Experimental RGB-only plugin. AI-generated. SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <memory>
#include "OpenRGBPluginInterface.h"
#include "FractalAdjustProController.h"
#include "RGBController.h"

class FractalAdjustProPlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID FILE "plugin.json")
    Q_INTERFACES(OpenRGBPluginInterface)
public:
    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override { return OPENRGB_PLUGIN_API_VERSION; }
    void Load(ResourceManagerInterface* api_ptr) override;
    void Unload() override;
    QWidget* GetWidget() override;
    QMenu* GetTrayMenu() override { return nullptr; }

private:
    struct Accessory : RGBController
    {
        Accessory(std::shared_ptr<FractalAdjustProController> hub_ptr, unsigned int target);
        void SetupZones() override {} // Fixed topology, initialized in the constructor.
        void ResizeZone(int, int) override {}
        void DeviceUpdateLEDs() override {}
        void UpdateZoneLEDs(int) override {}
        void UpdateSingleLED(int) override {}
        void DeviceUpdateMode() override;
        // Configuration uploads are synchronous: no queued HID work can outlive this plugin.
        void UpdateMode() override { DeviceUpdateMode(); }
        void UpdateLEDs() override {} // No per-LED/Direct mode; profiles also call this hook.
        std::shared_ptr<FractalAdjustProController> hub;
        unsigned int index;
    };
    ResourceManagerInterface* api = nullptr;
    std::vector<std::unique_ptr<Accessory>> accessories;
};
