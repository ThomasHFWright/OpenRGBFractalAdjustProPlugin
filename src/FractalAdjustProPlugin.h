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
    void MigrateProfiles();
    struct Direct : RGBController
    {
        explicit Direct(std::shared_ptr<FractalAdjustProController> hub_ptr);
        void SetupZones() override {}
        void ResizeZone(int, int) override {}
        void DeviceUpdateLEDs() override;
        void UpdateLEDs() override { DeviceUpdateLEDs(); }
        void UpdateZoneLEDs(int zone) override { if(zone == 0) DeviceUpdateLEDs(); }
        void UpdateSingleLED(int led) override { if(led >= 0 && led < (int)leds.size()) DeviceUpdateLEDs(); }
        void DeviceUpdateMode() override { DeviceUpdateLEDs(); }
        void UpdateMode() override { DeviceUpdateMode(); }
        std::shared_ptr<FractalAdjustProController> hub;
    };
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
        void UpdateLEDs() override; // SDK profile loads notify through this hook.
        std::vector<unsigned int> ModeSettings() const;
        std::vector<unsigned int> last_applied;
        std::shared_ptr<FractalAdjustProController> hub;
        unsigned int index;
        bool last_update_ok = true;
    };
    ResourceManagerInterface* api = nullptr;
    std::vector<std::unique_ptr<Accessory>> accessories;
    std::vector<std::unique_ptr<Direct>> streams;
};
