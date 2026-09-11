# Plugin validation — 2026-09-11

Host: unmodified official OpenRGB `728846f66861dd1cb7dc04835f651830d6ef13ce`,
plugin API 5, Qt 5.15.19, Linux x86-64, hidapi-hidraw 0.15.0. The host was built
from a clean upstream checkout, with no Fractal controller compiled in.
The independently loaded `.so` supplies all Fractal devices.

Hub firmware: 1.1.17. Exactly 11 accessories / 265 LEDs. Other OpenRGB hardware
detectors were disabled for these tests. The Fractal browser app was closed.

| Check | Result |
| --- | --- |
| Standalone packet/transport regression check (`./check.sh`) | PASS |
| Plugin compilation and dynamic dependencies | PASS |
| Settings → Plugins → Install Plugin file picker | PASS, copied final library byte-for-byte and loaded 11 devices |
| Official GUI loads API 5 plugin; SDK lists all 11 devices | PASS |
| Startup reads firmware, names, topology and current effects without lighting writes | PASS |
| Top Middle red + Top Front green independently | PASS, USB ACKs and user visual confirmation |
| Static blue on all accessories, brightness 0 / 25 / 75 | PASS, 104 acknowledged exchanges per update, exact metadata |
| Breathing speed 0 / 100 | PASS, 15 acknowledged exchanges per update, exact speed metadata |
| Independent red/blue Breathing + six-color Cycle | PASS, 28 acknowledged exchanges and hardware readback |
| SDK saves and restores two-color and animation profiles | PASS |
| Saved cancels preview without replacing the stored animations | PASS, 33 acknowledged exchanges and hardware readback |
| Previous built-in driver Off profile imported | PASS, all 11 accessories off |
| Disable plugin in Settings | PASS, zero SDK devices, USB handle released, hardware state unchanged |
| Enable plugin again | PASS, exactly 11 devices, no duplicates, current modes read back |
| Graceful GUI exit and restart | PASS, devices rediscovered, USB handle released on exit |
| Startup effects, rotation, mirror | Unchanged byte-for-byte |
| Final state | Off on all 11 accessories; exact original readback |

The live test controlled the real plugin through the official GUI's SDK server,
not a separate HID implementation. All captured HID exchanges had 64-byte requests
and replies with matching report/family/command and zero status. An allowlist
covered only the discovery and regular RGB commands. Direct readback took place
only after the plugin had released the hub.

Detailed local captures, test commands, and profiles are retained in the parent
workspace's `plans/evidence/plugin-*` files; profiles containing hardware serials
are excluded from this public repository. The public normal-Apply capture predates
the plugin and documents the unchanged transport protocol.

## Host limitations

This is a development version of OpenRGB. Its API 5 virtual-controller deletion
path logs `Device thread still active in base class destructor`; the base cleanup
then stops and joins the thread. Disable/re-enable and graceful exit completed in
live testing. The plugin uses the documented deletion API and keeps its callback
objects and HID connection alive until deletion returns. The host also logs a
network receive error when an SDK client disconnects normally; the listener remains
available and subsequent clients connect successfully. No host patches are included.

OpenRGB's missing-udev warning is generic: this machine already has a Fractal rule
that grants access. The development host does not persist its “don't show again”
checkbox through its settings schema. The warning's existing no-show preference
was stored directly in the local configuration while OpenRGB was closed; the next
GUI launch showed no dialog. No udev rules or permissions were changed.

Power-cycle retention, physical hot-unplug during an upload, other firmware, other
operating systems, and Direct/per-LED effects remain untested or unsupported.
Disconnect and timeout handling are covered by fake-HID regression checks. The new
plugin's animations were verified by ACKs and readback; the same hardware encoder's
animations had previously been visually confirmed using the built-in driver.
