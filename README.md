# Fractal Adjust Pro plugin for OpenRGB

Experimental, AI-generated **RGB-only** support for the Fractal Adjust Pro hub,
loaded through OpenRGB's plugin manager. No patched OpenRGB controller or upstream
merge request is required. GPL-2.0-or-later.

The **Fractal Adjust Pro** tab provides one-click themes, custom hardware lighting,
saved scenes, and startup effects. Each accessory also appears in **Devices**.
Animations run on the hub and continue after OpenRGB closes.

## Install with the normal OpenRGB release

Use the official **OpenRGB 1.0rc3 Hotfix 1 (1.0rc3.1)** download from
[OpenRGB releases](https://openrgb.org/releases.html), which uses **plugin API 4**.
The downloaded Linux x86-64 AppImage was used for live validation. OpenRGB itself
needs no source changes or custom build.

Download `libFractalAdjustProPlugin.so` from the
[v0.3.0 release](https://github.com/ThomasHFWright/OpenRGBFractalAdjustProPlugin/releases/tag/v0.3.0).
Open OpenRGB, choose **Settings → Plugins → Install Plugin**, select the library,
and enable it. Accessories appear in **Devices**. With OpenRGB closed, copying the
library into `~/.config/OpenRGB/plugins/` is an alternative.

- Linux HID `36bc:1001`, firmware **1.1.17**. Other firmware is refused.
- Plugin v0.3.0 targets API 4 / Qt 5. It does not load into API 5 development builds
  or OpenRGB 0.9's API 3. The older v0.1.0 plugin targeted development API 5.
- The binary was built on Linux x86-64 with Qt 5.15.19, GCC 16 and glibc 2.44, and
  tested with the official AppImage on that machine. Build the plugin from source
  on distributions with older system libraries.
- Tested: seven 20-LED fans, an 11-LED legacy chain, and three case components
  (3, 35, 76 LEDs): **11 accessories / 265 LEDs**.

Close the Fractal browser app before using the plugin. The current user must have
access to the hub's HID device. An existing Fractal udev rule may already provide
that access even when OpenRGB displays its generic missing-rules warning.

Disable and enable the plugin after reconnecting the hub or changing its physical
topology. Loading reads device state without changing lighting. Unloading releases
the connection and leaves the last hardware effect running.

## Build the plugin

Requires Git, a C++17 compiler, make, Qt 5 development tools (`qmake`), and
`hidapi-hidraw` development headers/library. The OpenRGB checkout supplies the
release API headers and the upstream `RGBController.cpp` implementation linked
into the plugin. **You do not build OpenRGB.**

```sh
git clone --branch release_candidate_1.0rc3.1 --depth 1 https://gitlab.com/CalcProgrammer1/OpenRGB.git
git clone https://github.com/ThomasHFWright/OpenRGBFractalAdjustProPlugin.git
cd OpenRGBFractalAdjustProPlugin
./check.sh
./build.sh ../OpenRGB
```

The build script checks release revision
`5e81e26fcc65d3dacfb76b0a30ec0142ec7bb131` to prevent mixing incompatible headers.
The output is `build/libFractalAdjustProPlugin.so`.

## Controls and profiles

Select accessories in the **Fractal Adjust Pro** tab, then click a theme:
Northern lights, Summer sky, Sunset, Starfall, Glistening ice, Pink sapphire,
Lunar mist, Mystic night, Campfire, or Radiant dawn. Set brightness before applying.

**Custom lighting** offers Shift, Still, Breathe, Waves, Two color fade, and Lava
lamp, with all 28 regular starting presets from the pinned vendor app. Edit RGB
colors, brightness and animation speed; Waves also exposes ramp-up, wave LED count,
ramp-down and frequency. Changes are sent when you click **Apply custom lighting**.
**Read highlighted accessory** copies its current regular settings into the editor.

The normal Devices tab retains Saved, Static, Off, Breathing and Color Cycle,
and adds the ten themes plus Shift, Waves, Two Color Fade and Lava Lamp. Static
corresponds to Still and Breathing to Breathe. Color Cycle is the original uniform
six-color cycle. Saved cancels a vendor hover preview and resumes currently saved
lighting; it does not undo ordinary Apply operations.

**Save current scene** and **Load scene** use native OpenRGB profiles, including
all OpenRGB devices. Wave shape is preserved in profiles. Existing v0.2 `.orp`
profiles are upgraded automatically, with originals retained in
`~/.config/OpenRGB/before-fractal-v0.3/`. Profiles store supported regular mode
parameters, not arbitrary vendor programs or a backup of hub flash. Unknown vendor
presets appear as Saved.

**Startup effect** independently saves Meshify effect, Fade in, No effect (instant
color), or RGB Off to the selected accessories, with color and brightness controls.
Use **Read highlighted accessory** to inspect the existing startup setting.
Regular scenes do not include or overwrite startup lighting. Startup writes and
readback have been tested; animation appearance and retention across a complete
power cycle still require physical verification.

Ordinary changes save the selected accessory's regular lighting, matching the
vendor app's Apply action. These are configuration uploads, not streamed frames.

Use the normal OpenRGB **Save Profile** and **Load Profile** controls. Release profiles
use `.orp`; development-build JSON profiles are not directly compatible. For command
line control, leave the GUI running and start its SDK server on `127.0.0.1`.
The API 4 plugin registers its accessories as local devices, so the release SDK
server includes them without enabling **All controllers**.
Standalone CLI detection does not load plugins in this host revision.

```sh
openrgb --client 127.0.0.1:6742 --noautoconnect \
  --device 'Fractal Adjust Pro Top Middle' --mode Static --color FF0000 --brightness 25
openrgb --client 127.0.0.1:6742 --noautoconnect \
  --device 'Fractal Adjust Pro' --mode Off
```

There is no Direct/per-LED streaming mode, so the Effects plugin's Direct-mode
effects are unsupported. No fan control, telemetry, firmware updates, resets,
or ownership switches are included. Power-cycle retention
and other operating systems remain untested.

## Validation and provenance

`./check.sh` checks the real transport with fake HID: independent packet examples,
topology bounds, malformed replies, failed writes, timeouts/disconnects, chunk
boundaries, early failure, and the RGB command allowlist.

See [live validation](docs/validation.md), [protocol notes](docs/protocol.md), and
[captured normal Apply packets](docs/normal-apply-capture.json).
The protocol and transport originated in the
[experimental built-in driver](https://github.com/ThomasHFWright/OpenRGB/tree/fractal-adjust-pro-rgb/Controllers/FractalAdjustProController);
the plugin reuses that tested encoder and transport with an API 4 adapter.
The plugin does not distribute or execute vendor JavaScript. Its independent C++
encoders are checked against 1,800 outputs from the pinned vendor bundle. For the
optional reference comparison, supply that bundle locally (the test executes its
encoder module without browser or HID access):

```sh
node tests/verify_vendor.cjs /path/to/main.e2372c083cc6b94f.js
```

Lava lamp uses a repeatable random phase so profiles replay consistently; the
vendor app chooses a fresh random phase. This can change the initial arrangement,
while retaining the same hardware animation and palette.
