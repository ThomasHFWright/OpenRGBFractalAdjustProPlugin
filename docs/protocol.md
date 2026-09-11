# Protocol and validation evidence

Hardware: `36bc:1001`, firmware `1.1.17`, interface 0, vendor usage page `ff00`, usage 1. HID descriptor: `0600ff0901a100150025ff26ff0085020600ff953f75080901910209018102c0`.

Reference: https://adjust.fractal-design.com/main.e2372c083cc6b94f.js, SHA256 `1916df59569691fef1981e3b68c53186857804ba7e85c44bcc72251bee2478a4`. Freshly fetched reference matches the portable app bundle. The implementation independently encodes the observed protocol; the plugin does not distribute or execute vendor JavaScript. An optional reference test executes the pinned local encoder module without HID access. AI-generated implementation.

Upstream base: `728846f66861dd1cb7dc04835f651830d6ef13ce`. Original built-in driver source: `c308a122095d137a3fd0c27278ea42d80e5b6d96`.

## Framing and discovery

All requests and replies are 64 bytes including report ID `02`; unused bytes are zero. Reply bytes 1–2 echo family/command, byte 3 is status (zero succeeds). HIDAPI read/write lengths, report identity, command identity and status are checked. Replies unrelated to the pending command are discarded within a one-second deadline. Failed operations are not blindly retried.

| Payload excluding report ID | Purpose | Reply data |
| --- | --- | --- |
| `f1 01` | Firmware query | ASCII at report offset 4 |
| `a4 08 00` | Unmerged topology | Count at 4, then ID/LED-count/generation triples |
| `a4 0a 01 ID` | Select an accessory | Zero-status acknowledgment |
| `a4 0e 00` | Name of selected accessory | Byte length at 4, UTF-8 bytes at 5 |
| `a4 05 01 00 00` | Regular lighting metadata | Metadata at report offset 11 |
| `a4 05 00 00 00` | Startup metadata | Metadata at report offset 11 |
| `a4 1a` | Dynamic Lighting query | Boolean at 4; active/unreadable refuses writes |
| `a4 16`, `a4 18` | Rotation/mirror queries | Value at 4 |
| `a4 14`, `a4 1b` | ARGB generation/compatibility (validation only) | Port data following status |

Selection is a transport cursor. No getter was found; enumeration leaves the last accessory selected as the vendor app does. Every driver update selects its own target while holding the shared HID mutex. No cooling, firmware upgrade/reset, persistent ownership, orientation, or ARGB configuration operation is used. Startup writes are explicitly separate from regular lighting.

Topology is 11 selectors / 265 LEDs: `01/02/03/04/11/12/13` have 20 LEDs, `21` has 11 ARGB1 LEDs, `31/32/33` have 3/35/76 ARGB2 LEDs. Zero generation byte means ARGB2. Physical port is high nibble + 1. Vendor UI groups the three last targets as Meshify 3 XL. The legacy chain is not assumed independently addressable per accessory. [Initial captures](baseline-queries.json) and [name/effect captures](effect-queries.json) contain actual device replies, not generated fixtures.

## Normal Apply versus hover preview

The vendor `setAnimation -> sendTimestampEffect -> applyTimestampEffect` path uses:

1. Select target: `02 a4 0a 01 ID`.
2. Declare program: `02 a4 11 01 SUM LEN_HI LEN_LO 00 01 METADATA...`.
3. Transfer chunks: `02 a4 12 COUNT DATA...`, maximum 60 data bytes per report.
4. Commit selected accessory: `02 a4 13`.
5. End loading: `02 a4 78`.

Here `01` at report offset 3 selects regular lighting (startup slot 0 is untouched). Offset 7 is preview=0. Offset 8 is target-count=1. Checksum is the sum of program bytes modulo 256. Length is big-endian. Programs are sequences of a big-endian 16-bit timestamp and RGB triples for every LED on the selected accessory. Firmware interpolates the uploaded keyframes. Timestamp units have not been independently measured.

**Normal Apply is an automatic save operation**, matching the vendor app. Independent red and green targets survived subsequent preview cancellation and profile reload, with correct per-target metadata. Commits occur once per effect update, not per frame. Flash/power-cycle retention was not tested; do not claim that result.

The original RAM-only design failed live testing. With preview=1 and target-count=1, setting Top Front green cleared Top Middle red (user confirmed). Grouping two previews made both metadata readbacks take the last uploaded color. Therefore ordinary control uses normal Apply; the rejected grouped-preview implementation is not included in the final driver.

**Saved** cancels hover preview via this exact zero-padded report prefix:

`02 a4 11 01 9c 00 06 01 00 99 01 64 00 00 00 32`

The vendor cancellation function calculates checksum/length from a zero-LED Still program (`00 00 02 32 04 64`) but sends no chunks because target count is zero. This resumes currently stored lighting; it does not undo earlier normal Apply operations. Live validation used a temporary red hover preview over stored animations, then Saved restored all eleven stored target states.

Read-effect replies show active preview metadata while hovering, not exclusively saved flash metadata. The driver recognizes its supported custom metadata for profiles. Unsupported vendor presets are represented as Saved; unknown custom previews are rejected rather than falsely reconstructed. This is not a full vendor-effect export.

## Encoder expectations

Metadata starts with preset, kind, speed, colors, brightness. Custom preset is 153; kinds 1/10/2/0 represent Static/Off/Breathing/Color Cycle in this driver. The original uniform cycle is identified by its exact six-color palette; other custom kind-0 palettes are Shift.

Three-LED full-red Static uses timestamps `0000`, `0232`, `0464`, each followed by `ff0000` three times. Length is 33, checksum `93`. Normal Apply header prefix is `02 a4 11 01 93 00 21 00 01 99 01 64 ff 00 00 64`. These independently derived literal bytes are regression expectations, not hardware captures. The regression also exercises the same header with preview=1 for cancellation/preview validation.

Breathing uses nine keyframes and two colors, with speed scaling traced from the vendor encoder. Color Cycle is an independently encoded uniform six-color cycle, not the vendor's spatial rainbow theme. Brightness scales RGB components with rounding. LED counts, metadata bounds, speed/brightness, response lengths/status, target bounds and 60-byte chunk boundaries are covered by the runnable regression.

## Validation boundaries

`./check.sh` passes against the actual controller transport with mocked HID, including timeout/disconnect, wrong command, malformed/status-error replies, failure at each upload stage, single commit and no cooling commands. The original Linux baseline and built-in driver builds passed. The plugin uses the same encoder and transport (with standalone logging). Exact CLI commands and acknowledgment counts are in [the validation record](validation.md).

Real hardware: restricted discovery, independent Static readback, all-target brightness 0/25/75, targeted Off, Breathing speed 25/75, mixed-animation save/load, and Saved preview cancellation pass packet/readback checks. Startup metadata, rotation and mirroring match all pre-test snapshots. User visually confirmed single red Static, the original preview-targeting failure, and the final independent red/blue Breathing and rainbow Color Cycle continuing after CLI exit while the remaining accessories stayed blue. Brightness 0/25/75 was verified by acknowledged packets and readback. Finally, the Off profile was loaded and all eleven states matched the saved Off snapshot exactly. No unplug, reboot, power-cycle or cooling experiment occurred.


## v0.3 hardware themes and custom lighting

`FractalAdjustProThemes.h` holds the factual palettes and defaults from the pinned
vendor application: ten named themes and eighteen additional regular starting
presets. Kinds 0/1/2/7/8/6 correspond to Shift/Still/Breathe/Waves/Two color fade/Lava
lamp. Custom edits use preset 153. Named presets retain their vendor preset ID.
The original five OpenRGB mode indices remain stable; ten named and four custom
modes append to them, for nineteen total.

Shift uploads spatial gradients with interpolated boundary frames. Lava lamp uses
14 generated frames plus the initial frame, with a deterministic phase seed for
repeatable profiles. Two color fade uploads a static spatial gradient twice at
0 and 20000; the firmware drives movement. A single-LED two-color gradient uses
the first color explicitly (the vendor's degenerate formula is undefined).
All spatial programs read and honor existing rotation/mirroring without writing
those settings. Three-LED and 76-LED case components follow the vendor's special
layout handling. RGB rounding is checked against the reference encoder.

Wave metadata includes two colors, ramp-up, width, ramp-down, `100-frequency`,
brightness, and the brightness-scaled second color. Ramp/width values are 0–25;
frequency is 0–100. The API 4 mode structure has no custom parameter extension,
so the Waves mode packs those four values into its serialized `direction` word,
low byte first. It does not advertise the direction flag; the plugin editor exposes
the four named controls. Native `.orp` files and SDK mode packets retain this word.

API 4 profile loading checks the number of modes. The plugin upgrades matching
five-mode v0.2 profiles by appending the new modes using the host's native profile
serializer, with an original backup and atomic replacement. Existing mode settings
are retained. API 4 SDK profile loads notify through `UpdateLEDs`; the adapter
applies changed mode settings there, avoiding a second save after `UpdateMode`.
Per-LED color writes remain unsupported.

## Startup slot

Startup uploads use the same selected-accessory transport and framing, with
header offset 3 set to **0**, preview 0 and target count 1. Metadata is preset,
kind, fixed speed 100, RGB color and brightness. Supported hub kinds are Meshify
3, Fade in 4, Instant 5 and Off 10. Other device-specific startup families are not
advertised for this hub.

Meshify has six frames at 0/138/178/218/258/318; Fade in has four at 0/80/130/150.
Off has three black frames. Instant is special: the vendor sends a zero-length
header and end-loading command, with **no chunks or commit**. Live readback confirms
this path saves the startup metadata. Other kinds use chunks, one commit and
end-loading. Startup saves never overwrite regular slot 1. Loading regular profiles
does not write startup settings.

All four kinds were written to every accessory and read back successfully, then
the original Meshify/white/50% startup metadata was restored exactly. Regular
metadata and orientation remained unchanged. This proves the command/save/readback
path, not the animation's appearance or power-cycle retention.


## Direct streaming (firmware 1.1.17)

The hub accepts standard LampArray HID **feature** reports on interface 0 even
when its active descriptor advertises only vendor report 2 and the persistent
Windows Dynamic Lighting preference is disabled. These are HIDAPI
`hid_get_feature_report` / `hid_send_feature_report`, not the 64-byte vendor
request/reply exchange. No preference toggle or descriptor change is needed.

| Report | Bytes, including report ID | Meaning |
| --- | --- | --- |
| Get `03` | 23 | LampArray attributes; bytes 1–2 are the little-endian lamp count |
| Set `08 00` | 2 | Suspend autonomous lighting; accept streamed frames |
| Set `06` | 51 | Update up to eight lamps; layout below |
| Set `08 01` | 2 | Resume autonomous hardware lighting |

Report 6: byte 1 is lamp count (1–8), byte 2 is the completion flag (1 only on
the last batch in a frame), bytes 3–18 contain eight little-endian uint16 lamp
indices, and bytes 19–50 contain eight RGB-intensity quads. Unused entries are
zero. This firmware copies RGB and ignores intensity; the plugin sends 255.
No vendor acknowledgement follows a feature write: HIDAPI must return its exact
length. A failed batch aborts the frame and attempts autonomous restoration.

The firmware uses one RGB buffer for all four outputs. The returned lamp count
is the maximum accessory LED count. The plugin checks this against topology and
allows 1–255 lamps; missing or inconsistent attributes disable Direct while
retaining hardware lighting support. On the tested hub, attributes were:
`03 4c00 400d0300 400d0300 204e0000 07000000 e02e0000`
(76 lamps; advertised minimum update interval 12,000 microseconds).
Ten feature reports are needed for 76 lamps; actual USB throughput was roughly
61 ms per complete frame. The encoder deliberately uses synchronous full frames.

Direct changes volatile autonomous control only. Discovery is read-only. All
feature and vendor traffic uses the same hub mutex. A normal hardware Apply,
explicit Hardware Effects selection, or plugin unload releases an owned stream.
External Dynamic Lighting ownership detected before starting is still refused.
No startup data, rotation, mirror, cooling setting or firmware is changed.

The vendor JavaScript's `A5 F0` / `A5 01` / `A5 F1` streaming path belongs to
another product. Although the hub firmware logger recognizes those command
names, the hub's dispatcher does not implement that path. The plugin does not
send them. Transport findings were checked against the official 1.1.17 firmware
image (SHA-256 `068a32ab0dfb3119f51f0254fc9a36e4c48bbb6143a057d6d56648f16fafbf20`)
and live volatile feature reports. No firmware was flashed or redistributed.
