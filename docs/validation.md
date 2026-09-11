# v0.2.0 — Normal release validation

Host: the official downloaded **OpenRGB 1.0rc3.1 Linux x86-64 AppImage**, as linked
from [OpenRGB releases](https://openrgb.org/releases.html). No locally compiled or
modified OpenRGB binary was used for these tests.

- Release revision: `5e81e26fcc65d3dacfb76b0a30ec0142ec7bb131`
- Plugin API **4**, SDK protocol **5**
- AppImage SHA256: `42910311b364ae525ca593f53f5fadcf746b4de41e9e49302a5aa5dd614a608a`
- Plugin built with Qt 5.15.19, GCC 16, glibc 2.44 and hidapi-hidraw 0.15.0
- Hub firmware 1.1.17; 11 accessories / 265 LEDs

The API 4 wrapper registers ordinary local RGB controllers through
`ResourceManagerInterface`. It links the matching upstream RGBController
implementation into the plugin. Lighting uploads are synchronous configuration
changes, so no queued USB writes survive plugin unloading. The validated protocol
encoder and HID transport are unchanged from v0.1.0.

| Check | Result |
| --- | --- |
| Protocol/transport regression (`./check.sh`) | PASS |
| Plugin build and dynamic dependencies | PASS |
| Official AppImage loads plugin and discovers 11 accessories | PASS |
| GUI Install Plugin file picker | PASS; installed library matches built library |
| Initial detection | Read-only lighting state, no Apply writes |
| Independent red / green Static | PASS: exact metadata, 18 acknowledged HID exchanges |
| All-accessory Static blue, brightness 0 / 25 / 75 | PASS: exact metadata, 104 exchanges each |
| Breathing speed 0 / 100 | PASS: exact metadata, 15 exchanges each |
| Mixed Breathing / Color Cycle / Static profile | PASS: 114 exchanges and hardware readback |
| Three development JSON profiles migrated to native `.orp` | PASS: supported parameters preserved |
| Two-color, animation and Off `.orp` save/load round trips | PASS: exact Apply metadata reproduced |
| GUI Load Profile button | PASS: migrated two-color profile applied |
| Saved preview cancellation | PASS: 33 exchanges; saved animation state preserved |
| Disable / enable plugin | PASS: 0 / 11 SDK devices, no duplicates, USB handle released while disabled |
| Disable preserves lighting | PASS: exact hardware readback before/after |
| Graceful close / restart | PASS; no API 5 thread-teardown warnings |
| Startup effects, rotation, mirroring | Unchanged byte-for-byte |
| Physical visual check | User confirmed Top Middle red/blue Breathing, Top Front Color Cycle, nine other accessories steady blue |
| Final Off restoration | PASS: all 11 accessories match the initial Off hardware snapshot exactly; startup, rotation and mirroring unchanged |
| Installed normal release restarted after restoration | PASS: SDK discovers 11 accessories, all Off |

All traced HID requests/replies were 64 bytes, with matching report/family/command
and zero status. An allowlist permitted only the discovery and regular RGB
commands. Direct readback was performed only while the plugin had released the
hub. Detailed commands, traces, and profiles are retained locally in the parent
workspace's `plans/evidence/release-*` records and `validate-release-plugin.py`.
Profiles containing hardware serials are excluded from this public repository.

Profiles preserve supported modes and parameters. Saved means “resume current
saved lighting”; it cannot restore an arbitrary vendor program from the earlier
JSON profile. Off and the implemented custom animations/colors can be restored.

The release host logs a network receive error when an SDK client disconnects
normally; the listener remains available and subsequent clients succeed. Its CLI
prints the HID location in the Version field; the plugin's actual firmware string
and HID firmware query are 1.1.17. These are existing host behaviors.

No Direct/per-LED streaming, cooling, firmware, reset, ownership, or startup-effect
operations were added. Power-cycle retention, physical hot-unplug during upload,
other firmware and other operating systems remain untested. Error/timeout handling
is covered by the fake-HID regression check. The previous development-build result
is retained separately as [historical API 5 validation](validation-api5.md).
