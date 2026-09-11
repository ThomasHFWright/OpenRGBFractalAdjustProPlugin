// Experimental RGB-only plugin. AI-generated. SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <memory>
#include "OpenRGBPluginInterface.h"
#include "FractalAdjustProController.h"

class FractalAdjustProPlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID FILE "plugin.json")
    Q_INTERFACES(OpenRGBPluginInterface)
public:
    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override { return OPENRGB_PLUGIN_API_VERSION; }
    void Load(OpenRGBPluginAPIInterface* api_ptr) override;
    void Unload() override;
    QWidget* GetWidget() override;
    QMenu* GetTrayMenu() override { return nullptr; }
    void OnProfileAboutToLoad() override {}
    void OnProfileLoad(nlohmann::json) override {}
    nlohmann::json OnProfileSave() override { return nlohmann::json::object(); }
    unsigned char* OnSDKCommand(unsigned int, unsigned char*, unsigned int* size) override { *size = 0; return nullptr; }
    void ProfileManagerUpdated(unsigned int) override {}
    void ResourceManagerUpdated(unsigned int) override {}
    void SettingsManagerUpdated(unsigned int) override {}

private:
    struct Accessory
    {
        std::shared_ptr<FractalAdjustProController> hub;
        unsigned int index;
        RGBControllerInterface* rgb = nullptr;
    };
    static RGBController_Setup Setup(Accessory& accessory);
    static void UpdateMode(void* object);
    OpenRGBPluginAPIInterface* api = nullptr;
    std::vector<std::unique_ptr<Accessory>> accessories;
};
