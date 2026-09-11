// Experimental RGB-only plugin. AI-generated. SPDX-License-Identifier: GPL-2.0-or-later
#include <cstdio>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include "ProfileManager.h"
#include "FractalAdjustProPlugin.h"

static_assert(OPENRGB_PLUGIN_API_VERSION == 4, "Build against OpenRGB 1.0rc3.1 (plugin API 4)");

OpenRGBPluginInfo FractalAdjustProPlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info{};
    info.Name = "Fractal Adjust Pro";
    info.Description = "Experimental RGB-only Adjust Pro support (firmware 1.1.17)";
    info.Version = "0.3.0";
    info.URL = "https://github.com/ThomasHFWright/OpenRGBFractalAdjustProPlugin";
    info.Location = OPENRGB_PLUGIN_LOCATION_TOP;
    info.Label = "Fractal Adjust Pro";
    return info;
}

FractalAdjustProPlugin::Accessory::Accessory(std::shared_ptr<FractalAdjustProController> hub_ptr, unsigned int target)
    : hub(std::move(hub_ptr)), index(target)
{
    const FractalAdjustProTarget& accessory = hub->targets[index];
    std::string address = "Port " + std::to_string((accessory.id >> 4) + 1) + " Device " + std::to_string(accessory.id & 0x0F);
    name = "Fractal Adjust Pro " + (accessory.name.empty() ? address : accessory.name);
    vendor = "Fractal Design";
    description = "Experimental RGB-only accessory, ARGB" + std::to_string(accessory.generation) + "; " + address;
    type = DEVICE_TYPE_LEDSTRIP;
    version = hub->firmware;
    location = hub->location + " " + address;
    serial = hub->serial + ":" + std::to_string(accessory.id);

    const char* names[] = {"Saved", "Static", "Off", "Breathing", "Color Cycle"};
    const char* custom_names[] = {"Shift", "Waves", "Two Color Fade", "Lava Lamp"};
    for(unsigned int i = 0; i < FractalThemes::ModeCount; i++)
    {
        mode m;
        m.name = i < FractalThemes::FirstMode ? names[i] : i < FractalThemes::Shift ?
            FractalThemes::themes[i - FractalThemes::FirstMode].name : custom_names[i - FractalThemes::Shift];
        m.value = i;
        m.color_mode = MODE_COLORS_NONE;
        if(i != FRACTAL_SAVED)
        {
            m.flags |= MODE_FLAG_AUTOMATIC_SAVE;
        }
        if(i != FRACTAL_SAVED && i != FRACTAL_OFF)
        {
            m.flags |= MODE_FLAG_HAS_BRIGHTNESS;
            m.brightness_min = 0;
            m.brightness_max = 100;
            m.brightness = 100;
        }
        if(i == FRACTAL_STATIC || i == FRACTAL_BREATHING || i >= FractalThemes::Shift)
        {
            m.flags |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
            m.color_mode = MODE_COLORS_MODE_SPECIFIC;
            m.colors_min = m.colors_max = i == FRACTAL_STATIC ? 1 : i == FractalThemes::Shift || i == FractalThemes::LavaLamp ? 6 : 2;
            m.colors.resize(m.colors_min, ToRGBColor(255, 0, 0));
            if(i == FRACTAL_BREATHING)
            {
                m.colors[1] = ToRGBColor(0, 0, 255);
            }
        }
        if(i >= FractalThemes::Shift)
        {
            unsigned int preset = i == FractalThemes::Shift ? 0 : i == FractalThemes::Waves ? 3 : i == FractalThemes::TwoColorFade ? 5 : 8;
            const auto& theme = FractalThemes::themes[preset];
            for(unsigned int c = 0; c < m.colors.size(); c++)
                m.colors[c] = ToRGBColor(theme.colors[c * 3], theme.colors[c * 3 + 1], theme.colors[c * 3 + 2]);
        }
        if(i == FractalThemes::Waves) m.direction = FractalThemes::Wave{}.Pack();
        if(i == FRACTAL_BREATHING || i == FRACTAL_CYCLE || i >= FractalThemes::FirstMode)
        {
            m.flags |= MODE_FLAG_HAS_SPEED;
            m.speed_min = 0;
            m.speed_max = 100;
            m.speed = i >= FractalThemes::FirstMode && i < FractalThemes::Shift ? FractalThemes::themes[i - FractalThemes::FirstMode].speed : i == FractalThemes::Waves ? 3 : 50;
        }
        modes.push_back(m);
    }
    const FractalAdjustProEffect& current = hub->effects[index];
    active_mode = current.mode;
    modes[active_mode].brightness = current.brightness;
    modes[active_mode].speed = current.speed;
    if(active_mode == (int)FractalThemes::Waves) modes[active_mode].direction = current.wave.Pack();
    for(unsigned int i = 0; i < modes[active_mode].colors.size(); i++)
    {
        modes[active_mode].colors[i] = ToRGBColor(current.colors[i * 3], current.colors[i * 3 + 1], current.colors[i * 3 + 2]);
    }

    zone z;
    z.name = name;
    z.type = ZONE_TYPE_LINEAR;
    z.leds_min = z.leds_max = z.leds_count = accessory.leds;
    zones.push_back(z);
    for(unsigned int i = 0; i < accessory.leds; i++)
    {
        led l{};
        l.name = "LED " + std::to_string(i + 1);
        leds.push_back(l);
    }
    SetupColors();
    last_applied = ModeSettings();
}

std::vector<unsigned int> FractalAdjustProPlugin::Accessory::ModeSettings() const
{
    if(active_mode < 0 || active_mode >= (int)modes.size()) return {};
    const mode& m = modes[active_mode];
    std::vector<unsigned int> settings = {(unsigned int)active_mode,
        m.flags & MODE_FLAG_HAS_BRIGHTNESS ? m.brightness : 100,
        m.flags & MODE_FLAG_HAS_SPEED ? m.speed : 100,
        active_mode == (int)FractalThemes::Waves ? m.direction : 0};
    settings.insert(settings.end(), m.colors.begin(), m.colors.end());
    return settings;
}

void FractalAdjustProPlugin::Accessory::UpdateLEDs()
{
    // API 4 SDK profile loads only call UpdateLEDs. Apply changed mode settings once;
    // per-LED notifications and the extra notification after UpdateMode do not save again.
    if(ModeSettings() != last_applied) DeviceUpdateMode();
}

void FractalAdjustProPlugin::Accessory::DeviceUpdateMode()
{
    last_update_ok = false;
    if(active_mode < FRACTAL_SAVED || active_mode >= (int)FractalThemes::ModeCount) return;
    const mode m = modes[active_mode];
    const unsigned int count = m.colors.size();
    if((active_mode == FRACTAL_STATIC && count != 1) || (active_mode == FRACTAL_BREATHING && count != 2) ||
       (active_mode >= (int)FractalThemes::Shift && count != (active_mode == (int)FractalThemes::Shift || active_mode == (int)FractalThemes::LavaLamp ? 6U : 2U)) || count > 6)
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Invalid mode color count\n");
        return;
    }
    unsigned char colors[18] = {};
    for(unsigned int i = 0; i < count; i++)
    {
        colors[i * 3] = RGBGetRValue(m.colors[i]);
        colors[i * 3 + 1] = RGBGetGValue(m.colors[i]);
        colors[i * 3 + 2] = RGBGetBValue(m.colors[i]);
    }
    last_update_ok = hub->Apply(index, active_mode,
                   m.flags & MODE_FLAG_HAS_BRIGHTNESS ? m.brightness : 100,
                   m.flags & MODE_FLAG_HAS_SPEED ? m.speed : 100, colors, count,
                   active_mode == (int)FractalThemes::Waves ? FractalThemes::Wave::Unpack(m.direction) : FractalThemes::Wave{});
    if(last_update_ok) last_applied = ModeSettings();
    if(!last_update_ok)
    {
        std::fprintf(stderr, "[Fractal Adjust Pro] Failed to apply mode to %s\n", name.c_str());
    }
}

void FractalAdjustProPlugin::Load(ResourceManagerInterface* api_ptr)
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
            auto device = std::make_unique<Accessory>(hub, i);
            api->RegisterRGBController(device.get());
            accessories.push_back(std::move(device));
        }
    }
    hid_free_enumeration(devices);
    MigrateProfiles();
}

void FractalAdjustProPlugin::Unload()
{
    if(!api) return;
    for(auto& device : accessories)
    {
        api->UnregisterRGBController(device.get());
    }
    accessories.clear();
    api = nullptr;
}


void FractalAdjustProPlugin::MigrateProfiles()
{
    // The release host only restores mode settings when mode counts match.
    // Extend v0.2 profiles without applying them or losing their existing settings.
    auto* manager = api->GetProfileManager();
    for(const auto& name : manager->profile_list)
    {
        const QString path = QString::fromStdString((api->GetConfigurationDirectory() / (name + ".orp")).string());
        QFile original(path);
        if(!original.open(QIODevice::ReadOnly)) continue;
        const QByteArray prefix = original.read(20);
        const QByteArray expected("OPENRGB_PROFILE\0\5\0\0\0", 20);
        original.close();
        if(prefix != expected) continue;
        auto controllers = manager->LoadProfileToList(name);
        bool changed = false;
        for(auto* saved : controllers)
        {
            if(saved->modes.size() != 5 || saved->active_mode < 0 || saved->active_mode >= 5) continue;
            for(const auto& live : accessories)
            {
                if(saved->serial != live->serial || saved->name != live->name || saved->vendor != live->vendor) continue;
                bool compatible = true;
                for(unsigned int i = 0; i < 5; i++)
                    compatible &= saved->modes[i].name == live->modes[i].name && saved->modes[i].value == live->modes[i].value;
                if(!compatible) continue;
                saved->modes.insert(saved->modes.end(), live->modes.begin() + 5, live->modes.end());
                changed = true;
                break;
            }
        }
        if(changed)
        {
            const QString backup_dir = QString::fromStdString((api->GetConfigurationDirectory() / "before-fractal-v0.3").string());
            QDir().mkpath(backup_dir);
            const QString backup = backup_dir + "/" + QFileInfo(path).fileName();
            QSaveFile output(path);
            bool ok = !QFile::exists(backup) && QFile::copy(path, backup) && output.open(QIODevice::WriteOnly);
            if(ok) ok = output.write(prefix) == prefix.size();
            for(auto* saved : controllers)
            {
                if(!ok) break;
                std::unique_ptr<unsigned char[]> data(saved->GetDeviceDescription(5));
                unsigned int size;
                std::memcpy(&size, data.get(), sizeof(size));
                ok = output.write(reinterpret_cast<char*>(data.get()), size) == size;
            }
            if(ok) ok = output.commit();
            if(!ok) std::fprintf(stderr, "[Fractal Adjust Pro] Could not migrate profile %s; original retained\n", name.c_str());
        }
        for(auto* saved : controllers) delete saved;
    }
}
