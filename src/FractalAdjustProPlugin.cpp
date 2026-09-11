// Experimental RGB-only plugin. AI-generated. SPDX-License-Identifier: GPL-2.0-or-later
#include <cstdio>
#include "FractalAdjustProPlugin.h"

static_assert(OPENRGB_PLUGIN_API_VERSION == 5, "Build against the documented OpenRGB API 5 revision");

OpenRGBPluginInfo FractalAdjustProPlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info{};
    info.Name = "Fractal Adjust Pro";
    info.Description = "Experimental RGB-only Adjust Pro support (firmware 1.1.17)";
    info.Version = "0.1.0";
    info.URL = "https://github.com/ThomasHFWright/OpenRGBFractalAdjustProPlugin";
    info.Location = OPENRGB_PLUGIN_LOCATION_INFORMATION;
    info.Label = "Fractal Adjust Pro";
    return info;
}

RGBController_Setup FractalAdjustProPlugin::Setup(Accessory& device)
{
    RGBController_Setup setup{};
    const FractalAdjustProTarget& accessory = device.hub->targets[device.index];
    std::string address = "Port " + std::to_string((accessory.id >> 4) + 1) + " Device " + std::to_string(accessory.id & 0x0F);
    setup.name = "Fractal Adjust Pro " + (accessory.name.empty() ? address : accessory.name);
    setup.vendor = "Fractal Design";
    setup.description = "Experimental RGB-only accessory, ARGB" + std::to_string(accessory.generation) + "; " + address;
    setup.type = DEVICE_TYPE_LEDSTRIP;
    setup.version = device.hub->firmware;
    setup.location = device.hub->location + " " + address;
    setup.serial = device.hub->serial + ":" + std::to_string(accessory.id);

    const char* names[] = {"Saved", "Static", "Off", "Breathing", "Color Cycle"};
    for(unsigned int i = 0; i <= FRACTAL_CYCLE; i++)
    {
        mode m;
        m.name = names[i];
        m.value = i;
        m.color_mode = MODE_COLORS_NONE;
        if(i != FRACTAL_SAVED)
        {
            m.flags |= MODE_FLAG_AUTOMATIC_SAVE;
        }
        if(i == FRACTAL_STATIC || i == FRACTAL_BREATHING || i == FRACTAL_CYCLE)
        {
            m.flags |= MODE_FLAG_HAS_BRIGHTNESS;
            m.brightness_min = 0;
            m.brightness_max = 100;
            m.brightness = 100;
        }
        if(i == FRACTAL_STATIC || i == FRACTAL_BREATHING)
        {
            m.flags |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
            m.color_mode = MODE_COLORS_MODE_SPECIFIC;
            m.colors_min = m.colors_max = i == FRACTAL_BREATHING ? 2 : 1;
            m.colors.resize(m.colors_min, ToRGBColor(255, 0, 0));
            if(i == FRACTAL_BREATHING)
            {
                m.colors[1] = ToRGBColor(0, 0, 255);
            }
        }
        if(i == FRACTAL_BREATHING || i == FRACTAL_CYCLE)
        {
            m.flags |= MODE_FLAG_HAS_SPEED;
            m.speed_min = 0;
            m.speed_max = 100;
            m.speed = 50;
        }
        setup.modes.push_back(m);
    }
    const FractalAdjustProEffect& current = device.hub->effects[device.index];
    setup.active_mode = current.mode;
    setup.modes[setup.active_mode].brightness = current.brightness;
    setup.modes[setup.active_mode].speed = current.speed;
    for(unsigned int i = 0; i < setup.modes[setup.active_mode].colors.size(); i++)
    {
        setup.modes[setup.active_mode].colors[i] = ToRGBColor(current.colors[i * 3], current.colors[i * 3 + 1], current.colors[i * 3 + 2]);
    }

    zone z;
    z.name = setup.name;
    z.type = ZONE_TYPE_LINEAR;
    z.leds_min = z.leds_max = z.leds_count = accessory.leds;
    setup.zones.push_back(z);
    for(unsigned int i = 0; i < accessory.leds; i++)
    {
        led l{};
        l.name = "LED " + std::to_string(i + 1);
        setup.leds.push_back(l);
    }
    setup.object_ptr = &device;
    setup.DeviceUpdateMode = UpdateMode;
    return setup;
}

void FractalAdjustProPlugin::UpdateMode(void* object)
{
    auto& device = *static_cast<Accessory*>(object);
    auto* rgb = device.rgb;
    const int mode = rgb->GetActiveMode();
    if(mode < FRACTAL_SAVED || mode > FRACTAL_CYCLE) return;
    const unsigned int count = rgb->GetModeColorsCount(mode);
    if((mode == FRACTAL_STATIC && count != 1) || (mode == FRACTAL_BREATHING && count != 2) || count > 2)
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Invalid mode color count\n");
        return;
    }
    unsigned char colors[6] = {};
    for(unsigned int i = 0; i < count; i++)
    {
        const RGBColor color = rgb->GetModeColor(mode, i);
        colors[i * 3] = RGBGetRValue(color);
        colors[i * 3 + 1] = RGBGetGValue(color);
        colors[i * 3 + 2] = RGBGetBValue(color);
    }
    const unsigned int flags = rgb->GetModeFlags(mode);
    if(!device.hub->Apply(device.index, mode,
                         flags & MODE_FLAG_HAS_BRIGHTNESS ? rgb->GetModeBrightness(mode) : 100,
                         flags & MODE_FLAG_HAS_SPEED ? rgb->GetModeSpeed(mode) : 100,
                         colors, count))
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Failed to apply mode to %s\n", rgb->GetName().c_str());
    }
}

void FractalAdjustProPlugin::Load(OpenRGBPluginAPIInterface* api_ptr)
{
    if(api) return;
    api = api_ptr;
    hid_device_info* devices = hid_enumerate(0x36BC, 0x1001);
    for(auto* info = devices; info; info = info->next)
    {
        if(info->interface_number != 0 || info->usage_page != 0xFF00 || info->usage != 1) continue;
        // Avoid opening the same hub alongside the earlier built-in experimental driver.
        bool duplicate = false;
        const std::string location = std::string("HID: ") + info->path + " ";
        for(auto* rgb : api->GetRGBControllers())
            if(rgb->GetLocation().compare(0, location.size(), location) == 0) duplicate = true;
        if(duplicate) continue;
        hid_device* dev = hid_open_path(info->path);
        if(!dev)
        {
            std::fprintf(stderr, "[Fractal Adjust Pro] Cannot open %s\n", info->path);
            continue;
        }
        auto hub = std::make_shared<FractalAdjustProController>(dev, info->path);
        if(!hub->Initialize()) continue;
        for(unsigned int i = 0; i < hub->targets.size(); i++)
        {
            auto device = std::make_unique<Accessory>();
            device->hub = hub;
            device->index = i;
            auto setup = Setup(*device);
            device->rgb = api->CreateVirtualRGBController(&setup);
            api->RegisterVirtualRGBController(device->rgb);
            accessories.push_back(std::move(device));
        }
    }
    hid_free_enumeration(devices);
}

void FractalAdjustProPlugin::Unload()
{
    if(!api) return;
    for(auto& device : accessories)
    {
        api->UnregisterVirtualRGBController(device->rgb);
        // Keep the callback object and shared HID handle alive until the host joins its worker.
        api->DeleteVirtualRGBController(device->rgb);
    }
    accessories.clear();
    api = nullptr;
}

QWidget* FractalAdjustProPlugin::GetWidget()
{
    auto* label = new QLabel(QString("%1 RGB accessories connected.\n\n"
        "Use the Devices tab for lighting and Profiles to save settings.\n"
        "Close the Fractal browser app before controlling the hub.\n"
        "After reconnecting the hub, disable and enable this plugin to detect it again.")
        .arg(accessories.size()));
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    label->setMargin(16);
    return label;
}
