# Custom 65% ZMK Keyboard — Firmware Guide

A 68-switch + rotary-encoder split-format 65%, built on `nice_nano_v2` + `nice!view`,
running [ZMK](https://zmk.dev) `v0.3`. This document describes what the firmware does
today and how to use it — it's a feature/usage reference, not a build log (see git
history and `ZMK_Firmware_PRD.md` for the "why").

## Contents

- [Layer overview](#layer-overview)
- [Base layer](#base-layer)
- [Symbol layer (⊃)](#symbol-layer-⊃)
- [Espanso setup (required for symbols)](#espanso-setup-required-for-symbols)
- [Function layer (f)](#function-layer-f)
- [Device layer (⊃ + f)](#device-layer--f)
- [Esc key — 4-way behavior](#esc-key--4-way-behavior)
- [Esc-hold brightness/sleep layer](#esc-hold-brightnesssleep-layer)
- [Mac / PC mode switch](#mac--pc-mode-switch)
- [Caps Word vs. Caps Lock](#caps-word-vs-caps-lock)
- [Rotary encoder — full behavior table](#rotary-encoder--full-behavior-table)
- [Display — 7 modes](#display--7-modes)
- [Safety combos (bootloader / reset)](#safety-combos-bootloader--reset)
- [Bluetooth](#bluetooth)
- [Typing test](#typing-test)
- [ZMK Studio](#zmk-studio)
- [Power](#power)
- [Dependencies](#dependencies)
- [Known limitations](#known-limitations)
- [Build / flash](#build--flash)

---

## Layer overview

| # | Name | Activation |
|---|------|------------|
| 0 | `default_layer` | always active (base QWERTY) |
| 1 | `sym_layer` | hold `⊃` (momentary) |
| 2 | `fn_layer` | hold `f` (momentary) |
| 3 | `device_layer` | hold `⊃` **and** `f` together (conditional layer — neither key alone triggers it) |
| 4 | `esc_layer` | hold `Esc` |
| 5 | `mac_mods` | toggled on/off from the device layer (`0`) — see [Mac / PC mode switch](#mac--pc-mode-switch) |
| 6-8 | `extra1`–`extra3` | reserved, empty — for ad-hoc ZMK Studio customization only |

Layers 1, 2, and 4 are momentary (active only while held). Layer 3 requires both
`⊃` and `f` held simultaneously. Layer 5 is a persistent on/off state that survives
independently of what else you're holding, until explicitly switched or the board
reboots (it does **not** persist across reboot — see [Known limitations](#known-limitations)).

---

## Base layer

Standard QWERTY, with these keyboard-specific points:

- **Esc / `` ` ``** (top-left key) — see [Esc key](#esc-key--4-way-behavior), not a plain key.
- **Caps Lock** — the labeled Caps Lock key is plain `&kp CAPS` (standard OS toggle
  behavior: one press/release edge per physical press).
- **Caps Word** — the key immediately right of the Up arrow (bottom-right cluster) is
  `&caps_word`, not Delete. See [Caps Word vs. Caps Lock](#caps-word-vs-caps-lock).
- **Delete** lives on `f` + Backspace (see [Function layer](#function-layer-f)), not
  on the base layer.
- **Modifier row** (left to right): `<>` (Ctrl) · `⊞` (Win) · Alt · `⌘` (Ctrl, PC
  default) · Space · `✦` (Right Alt, unassigned/reserved — see
  [Known limitations](#known-limitations)) · Space · `⊃` · `f` · `<>` (Ctrl) · arrows.
  In PC mode (default) all three `<>`/`<>`/`⌘` keys send Ctrl. In Mac mode this
  changes — see [Mac / PC mode switch](#mac--pc-mode-switch).

---

## Symbol layer (⊃)

Hold `⊃` to type Greek letters, math/logic symbols, and Unicode arrows. Nothing else
lives on this layer — no F-keys, no Bluetooth actions.

**How this actually works**: the firmware does *not* send Unicode codepoints
directly (an earlier revision tried that via `urob/zmk-unicode` and macOS's Option-hex
input — see git history; it required manually switching macOS's active input source
per symbol, which was unworkable). Instead, each symbol key types a short, plain
**ASCII trigger string** (e.g. `;;pi`), and a host-side text expander,
**[Espanso](https://espanso.org)**, watches for that string and replaces it with the
real glyph — identically configured on macOS and Windows, no OS-specific firmware
logic at all. **You must install and configure Espanso for symbols to work at all**
— see [Espanso setup](#espanso-setup-required-for-symbols) below.

All 45 symbols below are wired up and confirmed working on real hardware (macOS and
Windows, both via Espanso).

**Greek letters:**

| Key | Symbol | Trigger | Key | Symbol | Trigger | Key | Symbol | Trigger |
|---|---|---|---|---|---|---|---|---|
| W | ω | `;;omega` | A | α | `;;alpha` | Z | ζ | `;;zeta` |
| E | ε | `;;eps` | D | δ | `;;delta` | X | χ | `;;chi` |
| R | ρ | `;;rho` | F | φ | `;;phi` | V | ν | `;;nu` |
| T | τ | `;;tau` | G | γ | `;;gamma` | B | β | `;;beta` |
| Y | ψ | `;;psi` | H | θ | `;;theta` | N | η | `;;eta` |
| U | μ | `;;mu` | K | κ | `;;kappa` | | | |
| I | ι | `;;iota` | L | λ | `;;lambda` | | | |
| O | σ | `;;sigma` | | | | | | |
| P | π | `;;pi` | | | | | | |

`Q`, `S`, `C`, `J`, `M` are deliberately unmapped (`&trans`) — not a gap to fill in later.

**Number row (math/logic):**

| Key | Symbol | Trigger | Key | Symbol | Trigger |
|---|---|---|---|---|---|
| 1 | ± | `;;pm` | 7 | ∇ | `;;nabla` |
| 2 | ⊆ | `;;sub` | 8 | ∞ | `;;inf` |
| 3 | ∈ | `;;isin` | 9 | √ | `;;sqrt` |
| 4 | ¥ | `;;yen` | 0 | ° | `;;deg` |
| 5 | ∂ | `;;pd` | `-` | ≠ | `;;ne` |
| 6 | ∫ | `;;int` | `=` | ≈ | `;;approx` |

**Punctuation:**

| Key | Symbol | Trigger | Key | Symbol | Trigger |
|---|---|---|---|---|---|
| `[` | ∀ | `;;fa` | `,` | ≤ | `;;le` |
| `]` | ∃ | `;;te` | `.` | ≥ | `;;ge` |
| `\` | ⊥ | `;;perp` | `/` | ‽ (interrobang) | `;;irb` |
| `;` | ∴ | `;;tf` | Esc | ~ | *(plain `&kp LS(GRAVE)`, not a trigger)* |
| `'` | — (em dash) | `;;emd` | | | |

**Arrows** (literal glyphs, not navigation — navigation stays on the base layer):

| Key | Symbol | Trigger |
|---|---|---|
| Up | ↑ | `;;up` |
| Down | ↓ | `;;dn` |
| Left | ← | `;;lt` |
| Right | → | `;;rt` |

The canonical, always-up-to-date trigger table (with codepoints) is
[`espanso/symbols.yml`](espanso/symbols.yml) — copy it into your Espanso config rather
than retyping from this table.

---

## Espanso setup (required for symbols)

The [symbol layer](#symbol-layer-⊃) does nothing on its own — it only types plain
ASCII trigger strings. Without Espanso installed, running, and configured with this
board's match file, those triggers just sit there as literal text (`;;pi` stays
`;;pi`, it never becomes π).

1. Install [Espanso](https://espanso.org/install/) on every machine you use this
   board with (macOS **and** Windows — same tool, same config, on both).
2. Copy [`espanso/symbols.yml`](espanso/symbols.yml) from this repo into your
   Espanso match directory:
   - macOS: `~/Library/Application Support/espanso/match/symbols.yml`
   - Windows: `%APPDATA%\espanso\match\symbols.yml`
   - (Run `espanso path config` if unsure where your config actually lives.)
3. Restart Espanso (`espanso restart`) after adding or editing the match file.
4. Make sure Espanso is set to start automatically (its installer usually offers
   this) — if it's not running, triggers just do nothing.

**Trigger scheme**: every trigger is `;;` + a short mnemonic (`;;pi`, `;;isin`,
`;;omega`, …). No trigger is ever a prefix of another — Espanso fires on the first
exact match it sees, so a shorter trigger sharing a prefix with a longer one would
permanently shadow it. This is deliberate and preserved by design; if you ever add a
new symbol, keep that constraint (see the comment at the top of
[`espanso/symbols.yml`](espanso/symbols.yml)).

**If triggers get missed** (the firmware types `;;pi` but nothing happens, or only
part of it registers): the sym_layer macros inject keystrokes with 5ms timing
(`wait-ms`/`tap-ms` in `keyboard.keymap`), which is much faster than human typing —
Espanso needs to actually see each individual keystroke to detect the trigger. Try
raising those to 10-15ms before assuming anything else is wrong.

---

## Function layer (f)

Hold `f` for F-keys, extended navigation, and utility actions.

| Key(s) | Action |
|---|---|
| `1`–`9` | F1–F9 |
| `0` | F10 |
| `-` | F11 |
| `=` | F12 |
| Backspace | Delete |
| Up | Page Up |
| Left | Home |
| Down | Page Down |
| Right | End |
| Right Shift | Unlock ZMK Studio (`&studio_unlock`) |
| `T` | Start/stop the [typing test](#typing-test) |

Encoder while held: scroll down (CW) / scroll up (CCW), push = middle-click.

---

## Device layer (⊃ + f)

Hold **both** `⊃` and `f` together (neither alone) to reach Bluetooth management and
the Mac/PC mode switch.

| Key(s) | Action |
|---|---|
| `1`–`5` | Select BT profile 0–4 |
| `6` | Clear current BT profile |
| `7` | Next BT profile |
| `8` | Previous BT profile |
| `0` | **Toggle Mac/PC mode** — see below |
| `\` | Toggle USB / BLE output (`OUT_TOG`) |

Encoder rotation is intentionally unbound here (falls through to `f`'s scroll
behavior) — BT-profile cycling via the encoder was dropped since a single overshoot
detent risks an unwanted disconnect; use the discrete `1`–`8` keys instead. Encoder
push is a no-op.

---

## Esc key — 4-way behavior

The top-left key (labeled `Esc`/`` ` ``) is a hold-tap, not a plain key:

| Input | Result |
|---|---|
| Tap | `Esc` |
| Shift + tap | `` ` `` (backtick) |
| `⊃` held + tap | `~` (tilde — lives on the [symbol layer](#symbol-layer-⊃)) |
| **Hold** | Activates the [brightness/sleep layer](#esc-hold-brightnesssleep-layer) |

A quick tap always produces `Esc` — the hold threshold (200 ms) is tuned so normal
typing never accidentally triggers the hold behavior.

## Esc-hold brightness/sleep layer

Hold `Esc` (don't tap) to reach display brightness and sleep on the encoder — every
regular key is inert on this layer.

| Encoder action | Result |
|---|---|
| Rotate CW | Brightness up |
| Rotate CCW | Brightness down |
| Push | Put the host system to sleep |

See [Known limitations](#known-limitations) — brightness control is unreliable on
Windows for external keyboards; this is a host-OS limitation, not a firmware bug.

---

## Mac / PC mode switch

The board has two "modes" for how the modifier keys behave, switched from the
[device layer](#device-layer--f):

- **`0`** toggles between PC mode (**boot default**) and Mac mode.

This is a plain flip toggle (not two separate keys) — safe here because it's the
*only* thing tied to it (an earlier revision also kept a Unicode input mode in sync
with this switch and needed two explicit "set" keys to avoid desyncing the two; that
requirement went away once [symbols moved to Espanso](#espanso-setup-required-for-symbols),
which needs no OS-mode awareness at all). It only exists on the momentary device
layer, so it can never fire during normal typing — `0` always types a plain zero
unless you're holding `⊃`+`f`.

**What changes between modes:**

| Key | PC mode (default) | Mac mode |
|---|---|---|
| Left `<>` (modifier row, leftmost) | Ctrl | **Cmd** |
| `⊞` (Windows key) | Windows key | **Mission Control** (`Ctrl+Up`) |
| `⌘` (modifier row, 4th key) | Ctrl | **Cmd** |
| Right `<>` (modifier row, right of `f`) | Ctrl | **Ctrl (unchanged)** |

The right `<>` key **deliberately stays Control in Mac mode** — this is not a bug.
macOS genuinely needs a real Control key: `Ctrl+C` to interrupt a running process in
a terminal (there's no Cmd equivalent for SIGINT), `Ctrl+←/→` for Spaces switching,
`Ctrl+Tab` in browsers, and emacs-style `Ctrl+A`/`Ctrl+E` text bindings. If every
`<>` key sent Cmd in Mac mode, the board would have no way to send Control at all.
So: left `<>` and `⌘` both become Cmd (whichever your muscle memory reaches for),
right `<>` stays Control.

Unlike the modifier profile, [symbol-layer](#symbol-layer-⊃) output doesn't depend
on this switch at all anymore — Espanso triggers work identically regardless of
Mac/PC mode.

---

## Caps Word vs. Caps Lock

Two different keys, two different behaviors — don't confuse them:

- **Caps Lock** key (labeled, home-row-adjacent) — plain, standard OS Caps Lock.
  Stays on until pressed again.
- **Caps Word** key (bottom-right, right of Up arrow) — capitalizes the next word
  only, then automatically turns itself off. Continues through `_`, Backspace, and
  Delete (so `SOME_VAR_NAME` types correctly), but turns off at a space or most
  other punctuation.

---

## Rotary encoder — full behavior table

The encoder's rotation and push behavior depends on which layer (if any) is held:

| Context | Rotate CW | Rotate CCW | Push |
|---|---|---|---|
| Default (nothing held) | Volume up | Volume down | Cycle [display mode](#display--7-modes) (long-press resets to mode 1) |
| `⊃` held | Next track | Previous track | Play/Pause |
| `f` held | Scroll down | Scroll up | Middle-click |
| `⊃` + `f` held (device layer) | *(falls through to `f`'s scroll — see [device layer](#device-layer--f))* | | No-op |
| `Esc` held | Brightness up | Brightness down | Sleep |

---

## Display — 7 modes

Press the encoder (on the base layer, nothing else held) to cycle through display
modes; long-press to jump back to mode 1.

1. **Status** — layer name, BLE status, battery %, USB/BLE output
2. **WPM** — live words-per-minute counter
3. **Battery** — large battery percentage + charge bar
4. **Volume** — ⚠️ static placeholder only (ZMK has no volume state to read; see
   [Known limitations](#known-limitations))
5. **Typing Test** — results screen for the [typing test](#typing-test)
6. **Uptime** — ⚠️ static placeholder only, doesn't show a live clock
7. **Animation** — ⚠️ static placeholder only, no actual animation implemented

---

## Safety combos (bootloader / reset)

Two chorded combos, each requiring all three keys within 50 ms (accidental
activation during normal typing is essentially impossible):

| Combo | Action |
|---|---|
| Left Shift + Right Shift + Backspace | Enter UF2 bootloader mode (for flashing) |
| Left Shift + Right Shift + Caps Word key | Soft reboot |

---

## Bluetooth

5 BT profiles (0–4), managed from the [device layer](#device-layer--f):
`1`–`5` select a profile, `6` clears the current one, `7`/`8` cycle next/previous.
`\` toggles between USB and BLE output entirely.

---

## Typing test

Press `f` + `T` to start a typing test; press it again to stop and see results.
Switch the display to mode 5 (see [Display](#display--7-modes)) to watch it live or
review the result.

---

## ZMK Studio

Studio is unlocked via `f` + Right Shift. Layers 6–8 (`extra1`–`extra3`) are reserved
empty layers meant for ad-hoc Studio customization without needing a firmware
rebuild — everything else in this document is defined in the firmware itself and
Studio changes to those layers won't persist across a reflash.

> If you're flashing after the layer-restructure commit that introduced the device
> layer, Esc layer, and Mac/PC mode layer: those three new layers pushed
> `extra1`–`extra3` from indices 3–5 to 6–8. Any Studio customization made before
> that change is keyed to the old indices — run **Restore Stock Settings** in Studio
> after flashing, then re-customize.

---

## Power

- Deep sleep after 15 minutes of inactivity (~15 µA idle current). Any key or the
  encoder wakes it.
- BLE connection interval is tuned for a balance of typing latency and radio power
  draw — no user-facing setting.

---

## Dependencies

Declared in `config/west.yml`:

| Module | Purpose | Pinned to |
|---|---|---|
| [`zmkfirmware/zmk`](https://github.com/zmkfirmware/zmk) | ZMK firmware core | `v0.3` |

**External to the firmware — required on the host machine, not installed by
flashing:**

- **[Espanso](https://espanso.org)** must be installed, running, and configured with
  this board's match file on every machine you use this board with (macOS **and**
  Windows) — see [Espanso setup](#espanso-setup-required-for-symbols). Without it,
  the [symbol layer](#symbol-layer-⊃) types literal `;;pi`-style trigger strings
  instead of glyphs.

Build is CI-only via GitHub Actions (`.github/workflows/build-user-config.yml`) — no
local Zephyr toolchain is required or expected for normal use of this repo.

---

## Known limitations

- **Windows brightness control is unreliable.** The [Esc-hold layer](#esc-hold-brightnesssleep-layer)
  sends standard HID Consumer-page brightness commands, which macOS handles well but
  Windows frequently ignores for *external* keyboards (as opposed to a laptop's
  built-in keyboard). This is a Windows/host-OS limitation, not something the
  firmware can work around.
- **Espanso is a required install on every host** (macOS and Windows both), not
  bundled with the firmware — see [Dependencies](#dependencies). Without it running,
  the symbol layer just types literal `;;pi`-style text.
- **Espanso has to be running for symbols to work at all**, and some apps (certain
  games, some terminal emulators, apps that grab exclusive keyboard input) don't
  respect system-wide text expansion — symbol triggers may not expand there even
  with Espanso running correctly everywhere else.
- **Mac/PC mode resets to PC (default) on every reboot** — it isn't saved to flash.
  If you reboot while in Mac mode, press `0` (holding `⊃`+`f`) again once the board
  comes back up.
- **Display modes 4 (Volume), 6 (Uptime), and 7 (Animation) are static placeholders**
  — they show fixed text, not live data. ZMK has no volume state to read, and the
  uptime/animation modes were never built out beyond a label.
- **The `✦` key (Right Alt, between the two spacebars) has no assigned function**
  beyond sending plain Right Alt — a "gaming profile" or other use for it is
  undecided and deliberately unassigned for now.
- **Symbol-layer rollout is currently partial** — see the status note in
  [Symbol layer](#symbol-layer-⊃).

---

## Build / flash

This repo builds via GitHub Actions on every push to `main` — there is no local
Zephyr toolchain in normal use. After a push:

1. Wait for the `build-user-config.yml` workflow to finish (Actions tab).
2. Download the `.uf2` artifact from the completed run.
3. Put the board in bootloader mode (double-tap reset, or the
   [safety combo](#safety-combos-bootloader--reset)) and drag the `.uf2` onto the
   mounted drive.
