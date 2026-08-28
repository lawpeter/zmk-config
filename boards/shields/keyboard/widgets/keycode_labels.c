/*
 * PRD v2 §3 — see keycode_labels.h.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/modifiers.h>

#include "keycode_labels.h"

/* ── Modifier prefix ─────────────────────────────────────────────────────────
 * Collapse left/right of the same type to one letter: C(trl) S(hift) A(lt)
 * G(ui). "S-", "CG-", "CSAG-". Returns the number of bytes written (excl NUL). */
static size_t mod_prefix(char *buf, size_t buf_len, uint8_t mods) {
    if (mods == 0 || buf_len < 2) {
        buf[0] = '\0';
        return 0;
    }
    char tmp[5];
    size_t n = 0;
    if (mods & (MOD_LCTL | MOD_RCTL)) tmp[n++] = 'C';
    if (mods & (MOD_LSFT | MOD_RSFT)) tmp[n++] = 'S';
    if (mods & (MOD_LALT | MOD_RALT)) tmp[n++] = 'A';
    if (mods & (MOD_LGUI | MOD_RGUI)) tmp[n++] = 'G';

    size_t out = 0;
    for (size_t i = 0; i < n && out + 1 < buf_len; i++) {
        buf[out++] = tmp[i];
    }
    if (out + 1 < buf_len) {
        buf[out++] = '-';
    }
    buf[out] = '\0';
    return out;
}

/* ── Bare key label (no modifiers) ─────────────────────────────────────────── */

static void key_page_label(char *buf, size_t buf_len, uint32_t kc) {
    /* Letters A-Z */
    if (kc >= 0x04 && kc <= 0x1D) {
        snprintf(buf, buf_len, "%c", (char)('A' + (kc - 0x04)));
        return;
    }
    /* Digits: 0x1E..0x26 = 1..9, 0x27 = 0 */
    if (kc >= 0x1E && kc <= 0x26) {
        snprintf(buf, buf_len, "%c", (char)('1' + (kc - 0x1E)));
        return;
    }
    if (kc == 0x27) {
        snprintf(buf, buf_len, "0");
        return;
    }
    /* F1-F12 (0x3A..0x45), F13-F24 (0x68..0x73) */
    if (kc >= 0x3A && kc <= 0x45) {
        snprintf(buf, buf_len, "F%u", (unsigned)(kc - 0x3A + 1));
        return;
    }
    if (kc >= 0x68 && kc <= 0x73) {
        snprintf(buf, buf_len, "F%u", (unsigned)(kc - 0x68 + 13));
        return;
    }
    /* Modifiers 0xE0..0xE7 */
    static const char *const mods[] = {"LCTRL", "LSHIFT", "LALT", "LGUI",
                                       "RCTRL", "RSHIFT", "RALT", "RGUI"};
    if (kc >= 0xE0 && kc <= 0xE7) {
        snprintf(buf, buf_len, "%s", mods[kc - 0xE0]);
        return;
    }

    const char *s = NULL;
    switch (kc) {
    case 0x28: s = "ENTER"; break;
    case 0x29: s = "ESC"; break;
    case 0x2A: s = "BSPC"; break;
    case 0x2B: s = "TAB"; break;
    case 0x2C: s = "SPACE"; break;
    case 0x2D: s = "-"; break;
    case 0x2E: s = "="; break;
    case 0x2F: s = "["; break;
    case 0x30: s = "]"; break;
    case 0x31: s = "\\"; break;
    case 0x32: s = "#"; break; /* non-US */
    case 0x33: s = ";"; break;
    case 0x34: s = "'"; break;
    case 0x35: s = "`"; break;
    case 0x36: s = ","; break;
    case 0x37: s = "."; break;
    case 0x38: s = "/"; break;
    case 0x39: s = "CAPS"; break;
    case 0x46: s = "PRSCR"; break;
    case 0x47: s = "SCRLK"; break;
    case 0x48: s = "PAUSE"; break;
    case 0x49: s = "INS"; break;
    case 0x4A: s = "HOME"; break;
    case 0x4B: s = "PGUP"; break;
    case 0x4C: s = "DEL"; break;
    case 0x4D: s = "END"; break;
    case 0x4E: s = "PGDN"; break;
    case 0x4F: s = "RIGHT"; break;
    case 0x50: s = "LEFT"; break;
    case 0x51: s = "DOWN"; break;
    case 0x52: s = "UP"; break;
    case 0x64: s = "\\"; break; /* non-US backslash */
    case 0x65: s = "MENU"; break;
    default: break;
    }
    if (s) {
        snprintf(buf, buf_len, "%s", s);
    } else {
        snprintf(buf, buf_len, "0x%X", (unsigned)kc);
    }
}

static void consumer_page_label(char *buf, size_t buf_len, uint32_t kc) {
    const char *s = NULL;
    switch (kc) {
    case 0x6F: s = "BRI+"; break;
    case 0x70: s = "BRI-"; break;
    case 0xB5: s = "NEXT"; break;
    case 0xB6: s = "PREV"; break;
    case 0xCD: s = "PLAY"; break;
    case 0xE2: s = "MUTE"; break;
    case 0xE9: s = "VOL+"; break;
    case 0xEA: s = "VOL-"; break;
    default: break;
    }
    if (s) {
        snprintf(buf, buf_len, "%s", s);
    } else {
        snprintf(buf, buf_len, "C:0x%X", (unsigned)kc);
    }
}

void kc_label(char *buf, size_t buf_len, uint16_t usage_page, uint32_t keycode,
              uint8_t implicit_mods, uint8_t explicit_mods) {
    if (buf_len == 0) {
        return;
    }

    /* A held modifier key carries itself in explicit_modifiers; don't prefix it
     * with its own letter (e.g. LSHIFT would come out "S-LSHIFT"). */
    uint8_t mods = implicit_mods;
    bool is_mod_key = (usage_page == HID_USAGE_KEY && keycode >= 0xE0 && keycode <= 0xE7);
    if (!is_mod_key) {
        mods |= explicit_mods;
    }

    size_t off = mod_prefix(buf, buf_len, mods);

    switch (usage_page) {
    case HID_USAGE_KEY:
        key_page_label(buf + off, buf_len - off, keycode);
        break;
    case HID_USAGE_CONSUMER:
        consumer_page_label(buf + off, buf_len - off, keycode);
        break;
    default:
        snprintf(buf + off, buf_len - off, "%u:0x%X", (unsigned)usage_page, (unsigned)keycode);
        break;
    }
}

void kc_fmt_hold(char *buf, size_t buf_len, int64_t held_ms) {
    if (held_ms < 0) {
        held_ms = 0;
    }
    if (held_ms < 1000) {
        snprintf(buf, buf_len, "%d ms", (int)held_ms);
    } else if (held_ms < 100000) {
        /* Truncate to hundredths — the timer is ms-resolution, so anything
         * finer would be interpolated, not measured (PRD v2 §3). */
        int whole = (int)(held_ms / 1000);
        int hundredths = (int)((held_ms % 1000) / 10);
        snprintf(buf, buf_len, "%d.%02d s", whole, hundredths);
    } else {
        snprintf(buf, buf_len, "99+ s");
    }
}
