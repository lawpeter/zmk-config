/*
 * PRD §11 step 7 — custom multi-mode display screen.
 *
 * Overrides zmk_display_status_screen() from the nice_view shield.
 * CONFIG_NICE_VIEW_WIDGET_STATUS=n in keyboard.conf prevents nice_view from
 * compiling its own zmk_display_status_screen(), avoiding a linker conflict.
 * The nice_view widget files (util / bolt / status) are compiled via our
 * CMakeLists.txt so we can call zmk_widget_status_init() for mode 0.
 *
 * Mode layout (0-indexed, matching PRD §6.1 one-indexed modes):
 *   0  Status     — layer / BLE / battery / WPM  (nice_view status widget)
 *   1  WPM        — live WPM counter (zmk_widget_wpm)
 *   2  Battery    — placeholder
 *   3  Volume     — placeholder
 *   4  TypingTest — placeholder → Step 9
 *   5  Uptime     — placeholder
 *   6  Animation  — placeholder
 *
 * Mode switching:
 *   The &disp_cycle behavior raises zmk_display_mode_state_changed. Our
 *   ZMK_DISPLAY_WIDGET_LISTENER receives it on the display work queue and
 *   calls lv_scr_load() to swap the active LVGL screen. All 7 screens are
 *   created once at init and kept in memory; only the active one is shown.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>

#include "display_mode_state_changed.h"
#include "display_modes.h"
#include "widgets/status.h"   /* zmk_widget_status_init / zmk_widget_status_obj */
#include "widgets/wpm.h"      /* zmk_widget_wpm_init / zmk_widget_wpm_obj        */

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── State ─────────────────────────────────────────────────────────────────── */

static lv_obj_t *screens[DISPLAY_MODE_COUNT];
static uint8_t   active_mode;

static struct zmk_widget_status status_widget; /* mode 0 */
static struct zmk_widget_wpm    wpm_widget;    /* mode 1 */

/* Placeholder label text for modes that still use a placeholder (NULL = real widget). */
static const char *const mode_names[DISPLAY_MODE_COUNT] = {
    NULL,          /* 0: nice_view status widget */
    NULL,          /* 1: WPM widget (Step 8)     */
    "BATTERY",     /* 2 */
    "VOLUME",      /* 3 */
    "TYPING TEST", /* 4: Step 9 */
    "UPTIME",      /* 5 */
    "ANIMATION",   /* 6 */
};

/* ── Display event listener ─────────────────────────────────────────────────── */

struct display_mode_event_state {
    uint8_t mode;
};

static void mode_update_cb(struct display_mode_event_state state) {
    if (state.mode >= DISPLAY_MODE_COUNT) {
        return;
    }
    active_mode = state.mode;
    lv_scr_load(screens[active_mode]);
}

static struct display_mode_event_state mode_get_state(const zmk_event_t *eh) {
    const struct zmk_display_mode_state_changed *ev = as_zmk_display_mode_state_changed(eh);
    /* When called with eh=NULL (init), return current mode (0 at startup). */
    return (struct display_mode_event_state){.mode = ev ? ev->mode : active_mode};
}

ZMK_DISPLAY_WIDGET_LISTENER(display_mode_listener, struct display_mode_event_state,
                             mode_update_cb, mode_get_state)

ZMK_SUBSCRIPTION(display_mode_listener, zmk_display_mode_state_changed);

/* ── Screen factory (called once at init by the ZMK display subsystem) ─────── */

lv_obj_t *zmk_display_status_screen(void) {
    /* Mode 0: reuse the nice_view status widget (layer / BLE / battery / WPM). */
    screens[0] = lv_obj_create(NULL);
    zmk_widget_status_init(&status_widget, screens[0]);
    lv_obj_align(zmk_widget_status_obj(&status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    /* Mode 1: live WPM widget (Step 8). */
    screens[1] = lv_obj_create(NULL);
    zmk_widget_wpm_init(&wpm_widget, screens[1]);

    /* Modes 2-6: simple centered label placeholders, expanded in later steps. */
    for (int i = 2; i < DISPLAY_MODE_COUNT; i++) {
        screens[i] = lv_obj_create(NULL);
        lv_obj_t *lbl = lv_label_create(screens[i]);
        lv_obj_center(lbl);
        lv_label_set_text(lbl, mode_names[i]);
    }

    /* Subscribe to mode-change events (no-op on init since mode stays 0). */
    display_mode_listener_init();

    return screens[0]; /* ZMK loads this as the initial screen. */
}

uint8_t zmk_display_mode_get(void) { return active_mode; }
