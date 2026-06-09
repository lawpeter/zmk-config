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
 *   2  Battery    — battery percentage + bar
 *   3  Volume     — placeholder (ZMK has no volume state)
 *   4  TypingTest — typing-test widget (zmk_widget_typing_test)
 *   5  Uptime     — placeholder
 *   6  Animation  — placeholder
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>
#endif

#include "display_mode_state_changed.h"
#include "display_modes.h"
#include "widgets/status.h"
#include "widgets/util.h"
#include "widgets/wpm.h"
#include "widgets/typing_test.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── State ─────────────────────────────────────────────────────────────────── */

static lv_obj_t *screens[DISPLAY_MODE_COUNT];
static uint8_t   active_mode;

static struct zmk_widget_status status_widget;
static struct zmk_widget_wpm    wpm_widget;
static struct zmk_widget_typing_test tt_widget;

/* Battery screen state */
static lv_obj_t  *battery_canvas;
static lv_color_t battery_cbuf[CANVAS_SIZE * CANVAS_SIZE];

static const char *const mode_names[DISPLAY_MODE_COUNT] = {
    NULL,      /* 0: nice_view status widget */
    NULL,      /* 1: WPM widget              */
    NULL,      /* 2: battery canvas          */
    "VOL",     /* 3 */
    NULL,      /* 4: typing-test widget      */
    "UPTIME",  /* 5 */
    "ANIM",    /* 6 */
};

/* ── Display mode listener ─────────────────────────────────────────────────── */

struct display_mode_event_state { uint8_t mode; };

static void mode_update_cb(struct display_mode_event_state state) {
    if (state.mode >= DISPLAY_MODE_COUNT) return;
    active_mode = state.mode;
    lv_scr_load(screens[active_mode]);
}

static struct display_mode_event_state mode_get_state(const zmk_event_t *eh) {
    const struct zmk_display_mode_state_changed *ev = as_zmk_display_mode_state_changed(eh);
    return (struct display_mode_event_state){.mode = ev ? ev->mode : active_mode};
}

ZMK_DISPLAY_WIDGET_LISTENER(display_mode_listener, struct display_mode_event_state,
                             mode_update_cb, mode_get_state)
ZMK_SUBSCRIPTION(display_mode_listener, zmk_display_mode_state_changed);

/* ── Battery screen (mode 2) ────────────────────────────────────────────────
 * Draws into a 68×68 canvas then calls rotate_canvas (90° CW) so the content
 * appears portrait-correct on the physical display.
 * Canvas is centred horizontally in the 160×68 LVGL space (x-offset 46).     */

struct battery_mode_state {
    uint8_t level;
    bool    charging;
};

static void draw_battery_screen(struct battery_mode_state state) {
    if (!battery_canvas) return;

    lv_draw_rect_dsc_t rect_bg, rect_fg;
    lv_draw_label_dsc_t lbl_large, lbl_small;

    lv_draw_rect_dsc_init(&rect_bg);
    rect_bg.bg_color = LVGL_BACKGROUND;

    lv_draw_rect_dsc_init(&rect_fg);
    rect_fg.bg_color = LVGL_FOREGROUND;

    lv_draw_label_dsc_init(&lbl_large);
    lbl_large.color = LVGL_FOREGROUND;
    lbl_large.font  = &lv_font_montserrat_16;
    lbl_large.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_label_dsc_init(&lbl_small);
    lbl_small.color = LVGL_FOREGROUND;
    lbl_small.font  = &lv_font_unscii_8;
    lbl_small.align = LV_TEXT_ALIGN_CENTER;

    /* Clear background */
    lv_canvas_draw_rect(battery_canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_bg);

    /* Battery outline (60×14 at y=4) + terminal nub */
    lv_canvas_draw_rect(battery_canvas, 3, 4, 60, 14, &rect_fg);
    lv_canvas_draw_rect(battery_canvas, 4, 5, 58, 12, &rect_bg);
    lv_canvas_draw_rect(battery_canvas, 63, 7, 4, 8,  &rect_fg);
    lv_canvas_draw_rect(battery_canvas, 64, 8, 2, 6,  &rect_bg);

    /* Fill bar proportional to level */
    uint8_t fill_px = (state.level * 56) / 100;
    if (fill_px > 0) {
        lv_canvas_draw_rect(battery_canvas, 4, 5, fill_px, 12, &rect_fg);
    }

    /* Percentage text centred below the bar */
    char pct_text[6];
    snprintf(pct_text, sizeof(pct_text), "%d%%", state.level);
    lv_canvas_draw_text(battery_canvas, 0, 22, CANVAS_SIZE, &lbl_large, pct_text);

    /* Charging indicator */
    if (state.charging) {
        lv_canvas_draw_text(battery_canvas, 0, 44, CANVAS_SIZE, &lbl_small, "CHARGING");
    }

    rotate_canvas(battery_canvas, battery_cbuf);
}

static void battery_mode_update_cb(struct battery_mode_state state) {
    draw_battery_screen(state);
}

static struct battery_mode_state battery_mode_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_mode_state){
        .level    = ev ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .charging = zmk_usb_is_powered(),
#else
        .charging = false,
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(battery_mode_listener, struct battery_mode_state,
                             battery_mode_update_cb, battery_mode_get_state)
ZMK_SUBSCRIPTION(battery_mode_listener, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(battery_mode_listener, zmk_usb_conn_state_changed);
#endif

/* ── Screen factory ─────────────────────────────────────────────────────────── */

lv_obj_t *zmk_display_status_screen(void) {
    /* Mode 0: nice_view status widget */
    screens[0] = lv_obj_create(NULL);
    zmk_widget_status_init(&status_widget, screens[0]);
    lv_obj_align(zmk_widget_status_obj(&status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    /* Mode 1: WPM widget */
    screens[1] = lv_obj_create(NULL);
    zmk_widget_wpm_init(&wpm_widget, screens[1]);

    /* Mode 2: battery canvas */
    screens[2] = lv_obj_create(NULL);
    battery_canvas = lv_canvas_create(screens[2]);
    lv_obj_align(battery_canvas, LV_ALIGN_TOP_LEFT, 46, 0);
    lv_canvas_set_buffer(battery_canvas, battery_cbuf, CANVAS_SIZE, CANVAS_SIZE,
                         LV_IMG_CF_TRUE_COLOR);

    /* Mode 4: typing-test widget */
    screens[4] = lv_obj_create(NULL);
    zmk_widget_typing_test_init(&tt_widget, screens[4]);

    /* Modes 3, 5, 6: simple centred label placeholders */
    for (int i = 3; i < DISPLAY_MODE_COUNT; i++) {
        if (i == 4) continue;
        screens[i] = lv_obj_create(NULL);
        lv_obj_t *lbl = lv_label_create(screens[i]);
        lv_obj_center(lbl);
        lv_label_set_text(lbl, mode_names[i]);
    }

    display_mode_listener_init();
    battery_mode_listener_init();

    return screens[0];
}

uint8_t zmk_display_mode_get(void) { return active_mode; }
