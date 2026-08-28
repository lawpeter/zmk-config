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
- [Esc-hold brightness/lock layer](#esc-hold-brightnesslock-layer)
- [Mac / PC mode switch](#mac--pc-mode-switch)
- [Caps Word vs. Caps Lock](#caps-word-vs-caps-lock)
- [Rotary encoder — full behavior table](#rotary-encoder--full-behavior-table)
- [Display — 4 modes](#display--4-modes)
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
| 5 | `pc_mods` | toggled on/off from the device layer (`0`); **active by default at boot**, persisted to flash — see [Mac / PC mode switch](#mac--pc-mode-switch) |
| 6 | `pc_lock_layer` | conditional — active only while `esc_layer`(4) **and** `pc_mods`(5) are both active (holding `Esc` while already in PC mode); overrides just the lock shortcut, see [Esc-hold layer](#esc-hold-brightnesslock-layer) |
| 7-9 | `extra1`–`extra3` | reserved, empty — for ad-hoc ZMK Studio customization only |

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
- **Backspace / Delete** — the Backspace key is a mod-morph: plain tap is Backspace,
  **Shift + tap is Delete**. (Also still reachable via `f` + Backspace, see
  [Function layer](#function-layer-f) — either works.)
- **Modifier row** (left to right): `<>` (Cmd) · `⊞` (Mission Control) · Alt ·
  `⌘` (Cmd, Mac default) · Space · `✦` (Right Alt, unassigned/reserved — see
  [Known limitations](#known-limitations)) · Space · `⊃` · `f` · `<>` (Ctrl) · arrows.
  In Mac mode left `<>` and `⌘` send Cmd, `⊞` triggers Mission Control, right
  `<>` stays Ctrl. PC mode (the boot default) sends Ctrl / Windows key instead —
  see [Mac / PC mode switch](#mac--pc-mode-switch).

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

**Greek letters** — plain tap = lowercase, **Shift + tap = uppercase** (still while
holding `⊃`):

| Key | Lower | Trigger | Upper | Trigger | Key | Lower | Trigger | Upper | Trigger |
|---|---|---|---|---|---|---|---|---|---|
| W | ω | `;;omega` | Ω | `;;;omega` | Z | ζ | `;;zeta` | Ζ | `;;;zeta` |
| E | ε | `;;eps` | Ε | `;;;eps` | X | χ | `;;chi` | Χ | `;;;chi` |
| R | ρ | `;;rho` | Ρ | `;;;rho` | V | ν | `;;nu` | Ν | `;;;nu` |
| T | τ | `;;tau` | Τ | `;;;tau` | B | β | `;;beta` | Β | `;;;beta` |
| Y | ψ | `;;psi` | Ψ | `;;;psi` | N | η | `;;eta` | Η | `;;;eta` |
| U | μ | `;;mu` | Μ | `;;;mu` | A | α | `;;alpha` | Α | `;;;alpha` |
| I | ι | `;;iota` | Ι | `;;;iota` | D | δ | `;;delta` | Δ | `;;;delta` |
| O | σ | `;;sigma` | Σ | `;;;sigma` | F | φ | `;;phi` | Φ | `;;;phi` |
| P | π | `;;pi` | Π | `;;;pi` | G | γ | `;;gamma` | Γ | `;;;gamma` |
| | | | | | H | θ | `;;theta` | Θ | `;;;theta` |
| | | | | | K | κ | `;;kappa` | Κ | `;;;kappa` |
| | | | | | L | λ | `;;lambda` | Λ | `;;;lambda` |

`Q`, `S`, `C`, `J`, `M` are deliberately unmapped (`&trans`) — not a gap to fill in later.

The uppercase/shift trigger uses a 3-semicolon prefix (`;;;`) rather than relying on
case — the firmware masks Shift while typing the trigger text (a small custom
behavior, `&shift_mask`), so what actually gets typed is lowercase-shaped either way;
only the number of leading semicolons tells Espanso which glyph you mean. This
masking is scoped to each symbol's own macro, not to how long you hold any physical
key — so holding Shift down across several symbols in a row (the normal "hold shift,
type multiple capitals" gesture) works exactly like typing multiple regular capital
letters does.

**Number row (math/logic)** — every key also has a Shift variant, a second, unrelated
symbol (not a case pair — see note below):

| Key | Base | Trigger | Shift | Trigger |
|---|---|---|---|---|
| 1 | ± | `;;pm` | ⇔ | `;;;iff` |
| 2 | ⊆ | `;;sub` | ⊂ | `;;;psub` |
| 3 | ∈ | `;;isin` | ∉ | `;;;nisin` |
| 4 | ¥ | `;;yen` | € | `;;;eur` |
| 5 | ∂ | `;;pd` | ⨀ | `;;;odot` |
| 6 | ∫ | `;;int` | ∮ | `;;;cint` |
| 7 | ∇ | `;;nabla` | ⊗ | `;;;otimes` |
| 8 | ∞ | `;;inf` | ∝ | `;;;prop` |
| 9 | √ | `;;sqrt` | ✓ | `;;;check` |
| 0 | ° | `;;deg` | ∅ | `;;;empty` |
| `-` | ≠ | `;;ne` | ¬ | `;;;not` |
| `=` | ≈ | `;;approx` | ≡ | `;;;equiv` |

**Punctuation** — same pattern:

| Key | Base | Trigger | Shift | Trigger |
|---|---|---|---|---|
| `[` | ∀ | `;;fa` | ∧ | `;;;and` |
| `]` | ∃ | `;;te` | ∨ | `;;;or` |
| `\` | ⊥ | `;;perp` | ∥ | `;;;par` |
| `;` | ∴ | `;;tf` | ∵ | `;;;bcs` |
| `'` | — (em dash) | `;;emd` | – (en dash) | `;;;endash` |
| `,` | ≤ | `;;le` | ≪ | `;;;ll` |
| `.` | ≥ | `;;ge` | ≫ | `;;;gg` |
| `/` | ‽ (interrobang) | `;;irb` | ¿ | `;;;iq` |
| Esc | ~ | *(plain `&kp LS(GRAVE)`, not a trigger — no Shift variant)* | | |

**Arrows** (literal glyphs, not navigation — navigation stays on the base layer):

| Key | Base | Trigger | Shift | Trigger |
|---|---|---|---|---|
| Up | ↑ | `;;up` | ∩ | `;;;cap` |
| Down | ↓ | `;;dn` | ∪ | `;;;cup` |
| Left | ← | `;;lt` | ⇐ | `;;;bimp` |
| Right | → | `;;rt` | ⇒ | `;;;imp` |

All 24 non-Greek keys work the same way as uppercase Greek: hold `⊃` + Shift, and the
firmware masks Shift while typing a distinct `;;;`-prefixed trigger, so it's safe to
hold Shift across several symbols in a row (not just tap it briefly) — see the note
below.

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
| **Hold** | Activates the [brightness/lock layer](#esc-hold-brightnesslock-layer) |

A quick tap always produces `Esc` — the hold threshold (200 ms) is tuned so normal
typing never accidentally triggers the hold behavior.

## Esc-hold brightness/lock layer

Hold `Esc` (don't tap) to reach display brightness and screen lock on the encoder —
every regular key is inert on this layer.

| Encoder action | Result |
|---|---|
| Rotate CW | Brightness up |
| Rotate CCW | Brightness down |
| Push | **Lock the screen** — `Win+L` in PC mode (boot default), `Ctrl+Cmd+Q` in
Mac mode (see [Mac / PC mode switch](#mac--pc-mode-switch); this is the one place the
Mac/PC setting affects something outside the modifier row itself) |

Both are each OS's own default shortcut, not something this firmware invents — if
you've customized or disabled either shortcut in your OS settings, this won't work
until you restore it (or update the codepoint here to match your custom one).

See [Known limitations](#known-limitations) — brightness control is unreliable on
Windows for external keyboards; this is a host-OS limitation, not a firmware bug.

---

## Mac / PC mode switch

The board has two "modes" for how the modifier keys behave, switched from the
[device layer](#device-layer--f):

- **`0`** toggles between PC mode (**boot default**) and Mac mode.

This is a two-state flip toggle (not two separate keys) — safe here because it's the
*only* thing tied to it (an earlier revision also kept a Unicode input mode in sync
with this switch and needed two explicit "set" keys to avoid desyncing the two; that
requirement went away once [symbols moved to Espanso](#espanso-setup-required-for-symbols),
which needs no OS-mode awareness at all). It only exists on the momentary device
layer, so it can never fire during normal typing — `0` always types a plain zero
unless you're holding `⊃`+`f`.

**The choice is saved to flash.** Toggling to Mac and power-cycling brings the
board back up in Mac mode; the setting survives a firmware reflash too. Only a
fresh flash with no prior settings (or a settings erase) starts in the
Windows/PC default. Internally the mode is the `pc_mods` layer (index 5) being
active — a small custom behaviour (`&os_mode_toggle`) writes that state to the
same settings/NVS store ZMK uses for BT profiles and restores it at boot,
before the first keypress is processed.

**What changes between modes:**

| Key | Mac mode | PC mode (boot default) |
|---|---|---|
| Left `<>` (modifier row, leftmost) | **Cmd** | Ctrl |
| `⊞` (Windows key) | **Mission Control** (`Ctrl+Up`) | Windows key |
| `⌘` (modifier row, 4th key) | **Cmd** | Ctrl |
| Right `<>` (modifier row, right of `f`) | **Ctrl (unchanged)** | Ctrl |

The right `<>` key **deliberately stays Control in Mac mode too** — this is not a
bug. macOS genuinely needs a real Control key: `Ctrl+C` to interrupt a running
process in a terminal (there's no Cmd equivalent for SIGINT), `Ctrl+←/→` for Spaces
switching, `Ctrl+Tab` in browsers, and emacs-style `Ctrl+A`/`Ctrl+E` text bindings.
If every `<>` key sent Cmd in Mac mode, the board would have no way to send Control
at all. So: left `<>` and `⌘` both send Cmd (whichever your muscle memory reaches
for), right `<>` always sends Control.

Unlike the modifier profile, [symbol-layer](#symbol-layer-⊃) output doesn't depend
on this switch at all — Espanso triggers work identically regardless of Mac/PC mode.

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
| Default (nothing held) | Volume up | Volume down | Cycle [display mode](#display--4-modes) (long-press resets to mode 1) |
| `⊃` held | Next track | Previous track | Play/Pause |
| `f` held | Scroll up (4x speed) | Scroll down (4x speed) | Middle-click |
| `⊃` + `f` held (device layer) | *(falls through to `f`'s scroll — see [device layer](#device-layer--f))* | | No-op |
| `Esc` held | Brightness up | Brightness down | Lock screen (`Ctrl+Cmd+Q` Mac / `Win+L` PC) |

---

## Display — 4 modes

Press the encoder (on the base layer, nothing else held) to cycle through display
modes; long-press to jump back to mode 1.

1. **Status** — layer name, BLE status, battery %, USB/BLE output, plus a rolling
   CPM (characters-per-minute) graph and current value
2. **WPM** — live words-per-minute counter (a separate, dedicated screen — unrelated
   to mode 1's CPM graph)
3. **Battery** — large battery percentage + charge bar
4. **Typing Test** — results screen for the [typing test](#typing-test)

Volume, Uptime, and Animation modes were removed — they were static-text
placeholders with no real data behind them (ZMK has no volume state to read, and
the other two were never actually built out), so rather than leave dead screens in
the cycle they were dropped entirely.

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

### Profile assignments

| Key | Index | Device | Status |
|---|---|---|---|
| `1` | 0 | MacBook | Working |
| `2` | 1 | — | Unassigned |
| `3` | 2 | Windows PC | Working |
| `4` | 3 | — | Unassigned |
| `5` | 4 | — | Unassigned |

Notes:

- **Key label vs. index.** The keys are labeled `1`–`5` but select profile
  indices `0`–`4`. Anything reading raw ZMK settings (or ZMK Studio) reports the
  0-indexed number, so `1` = profile 0, `3` = profile 2, and so on.
- **Profile ≠ output channel.** Selecting a profile does *not* switch output to
  BLE — `\` (`OUT_TOG`) does that independently. A host can show "connected"
  while receiving no keystrokes because output is still set to USB. (This cost a
  real debugging session on the Mac.)
- **One device per profile.** Trying to pair a second host into an occupied
  profile fails rather than replacing the bond. Clear the profile with `6`
  first, then pair.
- **Linux/BlueZ does not work.** See [Known limitations](#known-limitations).

---

## Typing test

Press `f` + `T` to start a typing test; press it again to stop and see results.
Switch the display to mode 4 (see [Display](#display--4-modes)) to watch it live or
review the result. Screen reads, top to bottom: `READY`/`TYPING`/`DONE` (phase),
then once done, one stat per line — `WPM`, `CPM`, `TIME`, `KEYS`.

---

## ZMK Studio

Studio is unlocked via `f` + Right Shift. Layers 7–9 (`extra1`–`extra3`) are reserved
empty layers meant for ad-hoc Studio customization without needing a firmware
rebuild — everything else in this document is defined in the firmware itself and
Studio changes to those layers won't persist across a reflash.

> If you're flashing after a layer-count change (the original restructure that added
> device/Esc/mode-switch layers, or the later addition of `pc_lock_layer`):
> `extra1`–`extra3`'s indices shift every time. Any Studio customization made before
> such a change is keyed to the old indices — run **Restore Stock Settings** in
> Studio after flashing, then re-customize.

---

## Power

- Deep sleep after 15 minutes of inactivity (~15 µA idle current). **Press the
  encoder button to wake it** — that pin (P0.20) is a native nRF GPIO and is
  interrupt-capable. Matrix keys cannot wake it: the columns run through the
  MCP23017 I²C expander, which forces polled scanning, and polling only runs
  while the CPU is awake. Rotating the encoder does not wake it either; press only.
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

- **Windows brightness control is unreliable.** The [Esc-hold layer](#esc-hold-brightnesslock-layer)
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
- **The `✦` key (Right Alt, between the two spacebars) has no assigned function**
  beyond sending plain Right Alt — a "gaming profile" or other use for it is
  undecided and deliberately unassigned for now.
- **Symbol-layer rollout is currently partial** — see the status note in
  [Symbol layer](#symbol-layer-⊃).
- **Linux/BlueZ pairing fails.** Ubuntu pairing fails with
  `org.bluez.Error.AuthenticationFailed` on every profile. This matches a
  documented ZMK/BlueZ interaction, not a board fault — the same board pairs
  fine on macOS and Windows. Linux use is wired-only by decision; don't
  re-debug this.

---

## Build / flash

This repo builds via GitHub Actions on every push to `main` — there is no local
Zephyr toolchain in normal use. After a push:

1. Wait for the `build-user-config.yml` workflow to finish (Actions tab).
2. Download the `.uf2` artifact from the completed run.
3. Put the board in bootloader mode (double-tap reset, or the
   [safety combo](#safety-combos-bootloader--reset)) and drag the `.uf2` onto the
   mounted drive.
