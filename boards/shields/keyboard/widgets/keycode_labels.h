/*
 * PRD v2 §3 — HID-usage → human-readable label, and hold-duration formatting,
 * for the live keypress display mode.
 *
 * ZMK has no built-in keycode-to-name function. This is a deliberately small
 * table scoped to what this keyboard actually emits (keyboard page + the few
 * consumer-page media keys on the encoder / Esc layers). Anything unmapped
 * renders as a hex usage id rather than nothing.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * Write a short label for (usage_page, keycode) into buf.
 * implicit_mods / explicit_mods are the modifier bitmasks from
 * zmk_keycode_state_changed; when non-zero a compact prefix is prepended,
 * e.g. "S-A", "CG-Q". Pass 0 for both to get the bare key label.
 *
 * buf should be >= 12 bytes. Always NUL-terminates.
 */
void kc_label(char *buf, size_t buf_len, uint16_t usage_page, uint32_t keycode,
              uint8_t implicit_mods, uint8_t explicit_mods);

/*
 * Format a hold duration per PRD v2 §3:
 *   < 1000 ms          -> "847 ms"      (integer ms)
 *   1000 ms .. <100 s  -> "1.25 s"      (seconds, 2 dp, truncated not rounded)
 *   >= 100 s           -> "99+ s"       (clamped)
 * Maximum rendered width is 7 characters. buf should be >= 8 bytes.
 */
void kc_fmt_hold(char *buf, size_t buf_len, int64_t held_ms);
