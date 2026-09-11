# Protocol and validation evidence

Hardware: `36bc:1001`, firmware `1.1.17`, interface 0, vendor usage page `ff00`, usage 1. HID descriptor: `0600ff0901a100150025ff26ff0085020600ff953f75080901910209018102c0`.

Reference: https://adjust.fractal-design.com/main.e2372c083cc6b94f.js, SHA256 `1916df59569691fef1981e3b68c53186857804ba7e85c44bcc72251bee2478a4`. Freshly fetched reference matches the portable app bundle. The implementation independently encodes the observed protocol; it does not distribute or execute vendor JavaScript. AI-generated implementation.

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
| `a4 05 00 00 00` | Startup metadata (validation only) | Metadata at report offset 11 |
| `a4 1a` | Dynamic Lighting query | Boolean at 4; active/unreadable refuses writes |
| `a4 16`, `a4 18` | Rotation/mirror queries (validation only) | Value at 4 |
| `a4 14`, `a4 1b` | ARGB generation/compatibility (validation only) | Port data following status |

Selection is a transport cursor. No getter was found; enumeration leaves the last accessory selected as the vendor app does. Every driver update selects its own target while holding the shared HID mutex. No cooling, firmware upgrade/reset, ownership, orientation, ARGB configuration or startup-writing operation is used.

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

Metadata starts with preset, kind, speed, colors, brightness. Custom preset is 153; kinds 1/10/2/0 represent Static/Off/Breathing/Color Cycle in this driver. Only the exact six-color palette of this driver's cycle is recognized on readback.

Three-LED full-red Static uses timestamps `0000`, `0232`, `0464`, each followed by `ff0000` three times. Length is 33, checksum `93`. Normal Apply header prefix is `02 a4 11 01 93 00 21 00 01 99 01 64 ff 00 00 64`. These independently derived literal bytes are regression expectations, not hardware captures. The regression also exercises the same header with preview=1 for cancellation/preview validation.

Breathing uses nine keyframes and two colors, with speed scaling traced from the vendor encoder. Color Cycle is an independently encoded uniform six-color cycle, not the vendor's spatial rainbow theme. Brightness scales RGB components with rounding. LED counts, metadata bounds, speed/brightness, response lengths/status, target bounds and 60-byte chunk boundaries are covered by the runnable regression.

## Validation boundaries

`./check.sh` passes against the actual controller transport with mocked HID, including timeout/disconnect, wrong command, malformed/status-error replies, failure at each upload stage, single commit and no cooling commands. The original Linux baseline and built-in driver builds passed. The plugin uses the same encoder and transport (with standalone logging). Exact CLI commands and acknowledgment counts are in [the validation record](validation.md).

Real hardware: restricted discovery, independent Static readback, all-target brightness 0/25/75, targeted Off, Breathing speed 25/75, mixed-animation save/load, and Saved preview cancellation pass packet/readback checks. Startup metadata, rotation and mirroring match all pre-test snapshots. User visually confirmed single red Static, the original preview-targeting failure, and the final independent red/blue Breathing and rainbow Color Cycle continuing after CLI exit while the remaining accessories stayed blue. Brightness 0/25/75 was verified by acknowledged packets and readback. Finally, the Off profile was loaded and all eleven states matched the saved Off snapshot exactly. No unplug, reboot, power-cycle or cooling experiment occurred.
