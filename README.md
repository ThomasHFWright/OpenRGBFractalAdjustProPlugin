# Fractal Adjust Pro plugin for OpenRGB

Experimental, AI-generated **RGB-only** support for the Fractal Adjust Pro hub,
loaded through OpenRGB's plugin manager. No patched OpenRGB controller or upstream
merge request is required. GPL-2.0-or-later.

The plugin exposes each connected accessory in the normal **Devices** tab, with
Off, Static, two-color Breathing, Color Cycle, and Saved modes. Brightness, effect
speed, independent accessory control, and OpenRGB profiles are supported.

## Compatibility

- Linux, HID `36bc:1001`, firmware **1.1.17**. Other firmware is refused.
- Official OpenRGB development revision
  [`728846f66861dd1cb7dc04835f651830d6ef13ce`](https://gitlab.com/CalcProgrammer1/OpenRGB/-/commit/728846f66861dd1cb7dc04835f651830d6ef13ce),
  **plugin API 5**, Qt 5. The release binary targets Linux x86-64 with Qt 5.
- The binary was built with GCC 16 and glibc 2.44. Build from source on older
  distributions; this is not a universal AppImage-compatible binary.
- OpenRGB 0.9 and 1.0rc releases using plugin API 3/4 cannot load this plugin.
  API 5 is under development; rebuild and retest before changing the host revision.
  See the [tested host limitations](docs/validation.md#host-limitations).
- Tested hardware: seven 20-LED fans, an 11-LED legacy chain, and three case
  components (3, 35, and 76 LEDs): **11 accessories / 265 LEDs**.

## Build and install

Requires Git, a C++17 compiler, make, Qt 5 development tools (`qmake`), and
`hidapi-hidraw` development headers/library. OpenRGB itself needs its documented
build dependencies, including libusb and Qt translation tools.

```sh
git clone https://gitlab.com/CalcProgrammer1/OpenRGB.git
git -C OpenRGB checkout 728846f66861dd1cb7dc04835f651830d6ef13ce
mkdir OpenRGB/build
cd OpenRGB/build
qmake ../OpenRGB.pro
make -j4
cd ../..

git clone https://github.com/ThomasHFWright/OpenRGBFractalAdjustProPlugin.git
cd OpenRGBFractalAdjustProPlugin
./check.sh
./build.sh ../OpenRGB
```

Open the official OpenRGB GUI. Choose **Settings → Plugins → Install Plugin**,
select `build/libFractalAdjustProPlugin.so`, and enable it. The accessories appear
in **Devices**. Alternatively, with OpenRGB closed, copy the library into
`~/.config/OpenRGB/plugins/` and start OpenRGB.

Close the Fractal browser app before using the plugin. The current user must have
access to the hub's HID device. An existing Fractal udev rule may already provide
that access even when OpenRGB displays its generic missing-rules warning.

Disable and enable the plugin after reconnecting the hub or changing its physical
topology. Loading reads device state without changing lighting. Unloading releases
the connection and leaves the last hardware effect running.

## Controls and profiles

| Mode | Controls | Behavior |
| --- | --- | --- |
| Off | None | Turns the selected accessory off |
| Static | One color, brightness | Uniform color on that accessory |
| Breathing | Two colors, brightness, speed | Hardware-executed two-color breathing |
| Color Cycle | Brightness, speed | Uniform six-color cycle; no spatial rainbow |
| Saved | None | Cancels a vendor hover preview and resumes current saved lighting |

Ordinary mode changes save the selected accessory's **regular lighting**, matching
the vendor app's Apply action. They do not change startup lighting. Saved does not
undo those changes. Profiles store supported mode parameters, not arbitrary vendor
programs or a backup of hub flash. Unimplemented vendor presets are shown as Saved.

Use the normal OpenRGB **Save Profile** and **Active Profile** controls. For command
line control, leave the GUI running, start its SDK server on `127.0.0.1`, and enable
**All controllers** in the SDK server settings so plugin devices are exported.
Standalone CLI detection does not load plugins in this host revision.

```sh
openrgb --client 127.0.0.1:6742 --noautoconnect \
  --device 'Fractal Adjust Pro Top Middle' --mode Static --color FF0000 --brightness 25
openrgb --client 127.0.0.1:6742 --noautoconnect \
  --device 'Fractal Adjust Pro' --mode Off
```

There is no Direct/per-LED streaming mode, so the Effects plugin's Direct-mode
effects are unsupported. No fan control, telemetry, firmware updates, resets,
ownership switches, or startup-effect editing is included. Power-cycle retention
and other operating systems remain untested.

## Validation and provenance

`./check.sh` checks the real transport with fake HID: independent packet examples,
topology bounds, malformed replies, failed writes, timeouts/disconnects, chunk
boundaries, early failure, and the RGB command allowlist.

See [live validation](docs/validation.md), [protocol notes](docs/protocol.md), and
[captured normal Apply packets](docs/normal-apply-capture.json).
The protocol and transport originated in the
[experimental built-in driver](https://github.com/ThomasHFWright/OpenRGB/tree/fractal-adjust-pro-rgb/Controllers/FractalAdjustProController);
the plugin reuses that tested encoder and transport with a new API 5 adapter.
Vendor JavaScript is neither distributed nor executed.
