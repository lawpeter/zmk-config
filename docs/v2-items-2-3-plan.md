# Keyrambit v2 — implementation plan for items 2 and 3

Companion to `ZMK_Firmware_PRD.md` (v2). Items 1, 2, 4, 5 and **3** are all
implemented. Item 3 still needs the on-hardware two-round drop test and a
legibility check of the 3-line stacked layout (see §3 acceptance criteria).

**Item 2 — implemented** (Option A). New files:
`dts/bindings/zmk,behavior-os-mode.yaml`,
`boards/shields/keyboard/behaviors/behavior_os_mode.c`. `&tog 5` on
`device_layer` `0` is now `&os_mode_toggle`; `CONFIG_SETTINGS=y` made explicit
in `keyboard.conf`; boot default is PC/Windows via a SYS_INIT that activates
layer 5, real value restored in the settings-load callback. Comments + README
updated. Not yet built in CI or flashed.

---

## Item 5 result (feeds item 3 layout decision)

**Question:** does the nice!view fit three stacked key labels legibly?
**Answer: yes — go with 3-plus-`+`, not 2-plus-`+`.**

Evidence from the existing display code (no hardware measurement needed):

- All mode screens render into a **68×68 canvas** (`CANVAS_SIZE`) that is then
  rotated 270° CW and centred in the 160×68 LVGL space
  (`widgets/util.c:rotate_canvas`, `widgets/display_modes.c:77`).
- The typing-test **DONE** screen already stacks **four** lines of
  `lv_font_unscii_8` (8 px) at y = 18/30/42/54 inside that 68 px canvas
  dimension and they render legibly (`widgets/typing_test.c:87-96`).
- Three lines therefore fit with margin. At `lv_font_unscii_8` a 6-char label
  (`LSHIFT`) is ~30 px wide — well inside 68 px. At `lv_font_montserrat_14`
  it is ~47 px — still fits, and is the more legible choice for ≤3 lines.

**Layout recommendation for item 3:**
- 1–3 keys: one `montserrat_14` line each at y = 4 / 24 / 44.
- 4+ keys held: show the first 3, then a `+` on a 4th `unscii_8` line at y ~46
  (or replace line 3 with `+` if `montserrat_14` line 3 + `+` overflow — verify
  once on hardware, but the numbers say both fit).
- Released state: key label on line 1, hold-duration string on line 2.

---

## Item 2 — Windows default + OS-mode persistence

### 2a — flip boot default to Windows (low risk, do first)

The toggle is `&tog 5` → `pc_mods` (layer 5). Today the **base layer is
Mac-correct** and `pc_mods` overrides four positions to PC. To make Windows the
boot default without inverting all the override logic, the cleanest approach is:

**Option A (recommended): make `pc_mods` active at boot.**
Add a `zmk,keymap`-level default or a startup behavior that enables layer 5 on
init. ZMK has no "default layer set" keymap property for arbitrary layers, so
this needs the same custom init hook that 2b requires anyway — fold it in:
init code calls `zmk_keymap_layer_activate(5)` when the stored/default mode is
PC.

**Option B: rebuild the base layer as Windows-correct and invert `pc_mods` into
a `mac_mods` layer.** More invasive — touches the base layer, the
`pc_lock_layer` conditional (`esc_layer` + layer 5), and every comment/doc that
references the Mac/PC split. Not recommended; 2b makes Option A nearly free.

Decision needed from Peter: **A or B.** Plan below assumes **A**.

### 2b — persist OS mode across reboot

Layer state is not persisted by ZMK and `&tog` has no persistence option. Need a
small custom module.

**New behavior: `&os_mode_toggle`** (replaces `&tog 5` on `device_layer` key `0`)

Files:
- `boards/shields/keyboard/behaviors/behavior_os_mode.c`
- `dts/bindings/zmk,behavior-os-mode.yaml`
- node in `keyboard.keymap` behaviors block
- `CMakeLists.txt` — add the source
- `keyboard.keymap` — swap `&tog 5` → `&os_mode_toggle` at
  `device_layer` row 0, col 10 (`keyboard.keymap:968`)

Mechanism:
- **Storage:** Zephyr settings subsystem (`settings_save_one` /
  `SETTINGS_STATIC_HANDLER` or a `settings_load_subtree`), key
  `"os_mode/pc"`, one `uint8_t` (0 = Mac, 1 = PC). Same NVS backend ZMK
  already uses for BT — `CONFIG_SETTINGS=y` / `CONFIG_ZMK_SETTINGS` is
  already on (BT profiles persist), so no new Kconfig subsystem, but verify
  `CONFIG_SETTINGS` is explicitly present in `keyboard.conf` and add if not.
- **On press:** flip in-RAM state, `zmk_keymap_layer_activate/deactivate(5)`,
  then `settings_save_one("os_mode/pc", &val, 1)`. Write **only on actual
  change** (guard on `old != new`) — NVS endurance. Raise an event if a future
  display mode wants to show OS state (optional, skip for now).
- **On boot:** a settings handler `h_set` populates the RAM value during
  `settings_load()`. Then an init fn at `POST_KERNEL` /
  `APPLICATION` priority (after keymap init, before first scan) calls
  `zmk_keymap_layer_activate(5)` if value == PC. If no stored key exists,
  default to **PC (1)** per 2a.
- **Ordering guarantee for AC "before first keypress":** ZMK runs
  `settings_load()` early in `main()` before the kscan devices start
  reporting. Put the layer-activate in a `SYS_INIT` at
  `APPLICATION`/`CONFIG_APPLICATION_INIT_PRIORITY` or hook
  `zmk_keymap`'s init. Confirm against ZMK v0.3 `main.c` ordering during
  implementation — this is the one real risk in the item.

Edge cases:
- Settings not yet loaded when behavior first invoked → guard with an
  `initialized` flag; ignore/queue writes until load completes (in practice
  the user can't reach `device_layer` in the first few ms).
- Firmware reset / re-flash keeps NVS unless the settings partition is
  erased → AC "survives firmware reset" holds. A full flash-erase falls back
  to PC default, which is correct.

### Item 2 acceptance-criteria mapping

| AC | Covered by |
|---|---|
| Boot = Windows on first flash | boot handler default = 1 |
| Toggle to Mac + power cycle → Mac | `settings_save_one` on change + boot restore |
| Device-layer `0` toggles exactly two states | behavior is a boolean flip |
| Right `<>` = Control both modes | unchanged — `pc_mods` leaves it `&trans` |
| `0` still types zero when device layer not held | unchanged — binding only on `device_layer` |

### Docs to update for item 2
- `keyboard.keymap:13,59-65,873-878,952-957,1014-1021` — comments say "boot
  default = Mac".
- `README.md:457` known-limitation bullet ("resets to Mac on every reboot") →
  rewrite as "persists across reboot; boot default is Windows".
- `README.md` device-layer / modifier-profile section.

---

## Item 3 — live keypress readout (display mode 5)

### Data source

Subscribe to `zmk_keycode_state_changed` (already used by
`behavior_typing_test.c:45`). This is **post-layer resolution** — it carries the
final HID usage the keyboard sends, satisfying the "reflect what the key
actually sends" requirement. It is raised on the **ZMK event thread, not the
scan loop**, so subscribing cannot slow the I²C scan — satisfies the hard
constraint in the PRD. The display redraw is further deferred to the LVGL
display thread via `ZMK_DISPLAY_WIDGET_LISTENER` (same pattern as
`battery_mode_listener`).

### New files
- `boards/shields/keyboard/widgets/keypress.c` / `.h` — the widget
  (68×68 canvas, `rotate_canvas`, like `wpm.c`).
- `boards/shields/keyboard/widgets/keycode_labels.c` / `.h` — usage→string
  lookup. ZMK has **no** built-in keycode-to-name function, so we need a table.
  Scope it to what this keyboard actually emits: letters, digits, F-keys,
  mods (`LSHIFT`…`RGUI`), nav/edit cluster, symbols/punctuation, `SPACE`,
  `ENTER`, `TAB`, `ESC`, `BSPC`, `DEL`, plus consumer-page entries for the
  media keys on the encoder/Esc layers. Fallback: render `0xNNN` hex for an
  unmapped usage rather than nothing.
- Wire into `CMakeLists.txt` and `display_modes.c` as `screens[4]`.

### State machine (in the widget)

```
held[]  : ring/array of up to N currently-down usages, in press order
          (push on state==1, remove on state==0)
last    : { usage, press_ms }  — for the released view
```

- **On press (state=1):** add usage to `held[]`, record `press_ms` for it,
  redraw "pressed" view (stacked labels, up to 3 + `+`).
- **On release (state=0):** remove from `held[]`. If `held[]` now empty, set
  `last = { usage, now - press_ms }` and redraw "released" view (label +
  duration). If not empty, redraw "pressed" view for the remainder.
- Multiple keys down: track per-usage press timestamps so the duration shown on
  release is that key's own hold time.

### Hold-duration formatting (`keycode_labels.c` helper, ≤7 chars)

```c
// buf must be >= 8 bytes
void fmt_hold(char *buf, size_t n, int64_t ms) {
    if (ms < 1000)        snprintf(buf, n, "%lld ms", ms);        // "847 ms"
    else if (ms < 100000) snprintf(buf, n, "%lld.%02lld s",       // "1.25 s" / "12.34 s"
                                   ms / 1000, (ms % 1000) / 10);
    else                  snprintf(buf, n, "99+ s");              // clamp
}
```
- 2 decimal places only — the timer is ms-resolution, more digits would be
  interpolated (PRD note).
- `(ms % 1000) / 10` gives hundredths without rounding; acceptable, matches
  "at the limit of real precision".
- Max width check: `"12.34 s"` = 7 chars ✓, `"99+ s"` = 5 ✓.

### Integration points
- `DISPLAY_MODE_COUNT` 4 → 5 (`widgets/display_modes.h`).
- `display_modes.c:zmk_display_status_screen()` — add `screens[4]` init.
- Encoder cycle already does `(current_mode + 1) % DISPLAY_MODE_COUNT` and
  long-press → 0 (`behavior_disp_cycle.c:55-58`) — no change needed, mode 5
  falls out for free.
- `CONFIG_LV_Z_MEM_POOL_SIZE` currently 8192 for 4 screens — bump to ~10240
  for the 5th (the comment at `keyboard.conf` still says "7 LVGL screens", so
  headroom may already exist; measure with a build).

### Acceptance-criteria mapping

| AC | Covered by |
|---|---|
| Reachable in encoder cycle; long-press → mode 1 | `DISPLAY_MODE_COUNT`++ , existing behavior |
| Updates within one scan interval, no lag | event-thread listener + display-thread redraw |
| No new dropped keystrokes (2-round drop test) | no scan-loop work; verify on hardware both rounds |
| Modifier combos render correctly | `held[]` stacking + mod labels in table |
| Human-readable label not raw keycode | `keycode_labels.c` table + hex fallback |
| Up to 3 stacked + `+` beyond | item 5 result above |
| Released: persist last key + hold duration | `last` state + `fmt_hold` |

### Docs to update for item 3
- `README.md:349` heading "Display — 4 modes" → 5, add list entry 5.
- `README.md:361` "Volume, Uptime, Animation were removed" paragraph — add a
  line that mode 5 (keypress) has live data behind it, unlike those.
- `keyboard.keymap:37` encoder behavior-table comment (mode count).

### Suggested build/test order
1. 2a+2b together (Option A) — one behavior, one boot handler. Flash, verify
   AC table, verify `pc_lock_layer` still triggers.
2. Item 3 — labels table first (unit-testable logic), then widget, then wire in.
   Run the two-round drop test with mode 5 active before calling it done.
