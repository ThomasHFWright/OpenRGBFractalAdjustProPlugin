# v0.3.0 — Themes, custom lighting and startup

Host: official downloaded OpenRGB **1.0rc3.1** Linux x86-64 AppImage, plugin API 4,
SDK protocol 5, unchanged from [v0.2 validation](validation-v0.2.md).
Firmware **1.1.17**, 11 accessories / 265 LEDs. No OpenRGB host patches.

| Check | Result |
| --- | --- |
| `./check.sh`: packet, transport and failure regression | PASS |
| Pinned vendor comparison (`tests/verify_vendor.cjs`) | PASS: 1,800 complete header/program comparisons |
| All ten named themes, all eleven accessories | PASS: acknowledged hardware uploads |
| Six custom families and native profile round trips | PASS: exact Apply metadata reproduced |
| Wave shape through SDK and native profiles | PASS: exact Apply metadata reproduced |
| v0.2 profile migration | PASS: all original fields preserved; all three profiles load |
| Startup Meshify, Fade in, Instant and Off | PASS: all eleven accessories, exact metadata readback |
| Startup writes preserve regular lighting and orientation | PASS: exact before/after snapshots |
| Restore original startup settings | PASS: exact snapshot match |
| Scene filename validation | PASS: new save, duplicate and invalid-name refusal; no HID writes |
| Plugin scene/editor/startup GUI | PASS: target selection, Waves shape, scene save/load, startup save/read |
| Final installation/restoration | PASS: Campfire and original startup/orientation restored exactly; installed SDK sees 11 accessories / 19 modes |

The reference check covers LED counts 3, 11, 20, 35, 76 and 255, brightness and
speed endpoints, rotations/mirroring, custom palettes, wave controls and all four
startup kinds. Lava lamp random input is seeded identically in the comparison.
No vendor source is distributed with the plugin.

Live tests use a copied configuration and the official host. The HID trace checks
64-byte request/reply identity, success status and an RGB/discovery command
allowlist. The release CLI can exit while the server is still processing uploads;
the test waits for the trace to settle before checking all selected targets.
Startup readback is separate from regular lighting readback. No cooling, reset,
firmware, ownership or orientation writes were made.

The older physical confirmation covers Static, Breathing and Color Cycle only.
The new spatial animations have packet/reference validation; no new visual
confirmation or full power cycle is claimed. Other firmware, other operating
systems and physical unplug during upload remain untested.

Detailed local evidence is in the parent workspace's `plans/evidence/themes-*`,
`startup-*` and `validate-themes.py`. Device serials and local profiles are excluded
from this public repository.
