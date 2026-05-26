/*
 * PRD §11 step 8 — Live WPM counter widget for display mode 1.
 *
 * Layout (160 × 68 nice!view Sharp Memory LCD, 1-bit):
 *
 *   ┌──────────────────────────┐
 *   │                          │
 *   │           WPM            │  ← title_label (MONTSERRAT_14, top-center)
 *   │                          │
 *   │           123            │  ← wpm_label   (MONTSERRAT_18, center)
 *   │                          │
 *   └──────────────────────────┘
 *
 * The widget subscribes to zmk_wpm_state_changed via ZMK_DISPLAY_WIDGET_LISTENER
 * so all LVGL calls run on the ZMK display work queue.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

#include "wpm.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── Internal state type passed from event thread to display thread ───────── */

struct wpm_state {
    uint8_t wpm;
};

/* ── Linked-list of all live widget instances ─────────────────────────────── */

static sys_slist_t widgets;

/* ── Display-thread callback ─────────────────────────────────────────────── */

static void wpm_update_cb(struct zmk_widget_wpm *widget, struct wpm_state state) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", state.wpm);
    lv_label_set_text(widget->wpm_label, buf);
    LOG_DBG("wpm widget: %u WPM", state.wpm);
}

/* ── Iterate the widget list on every event ──────────────────────────────── */

static void set_wpm_state(struct wpm_state state) {
    struct zmk_widget_wpm *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        wpm_update_cb(widget, state);
    }
}

/* ── ZMK_DISPLAY_WIDGET_LISTENER plumbing ────────────────────────────────── */

static struct wpm_state wpm_get_state(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    return (struct wpm_state){
        .wpm = ev ? ev->wpm : zmk_wpm_get_state(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm, struct wpm_state, set_wpm_state, wpm_get_state)
ZMK_SUBSCRIPTION(widget_wpm, zmk_wpm_state_changed);

/* ── Public API ──────────────────────────────────────────────────────────── */

int zmk_widget_wpm_init(struct zmk_widget_wpm *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);

    /* Fill the parent screen; remove default LVGL padding / border. */
    lv_obj_set_size(widget->obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);

    /* "WPM" title — top-center. */
    lv_obj_t *title = lv_label_create(widget->obj);
    lv_label_set_text(title, "WPM");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Live counter — center of screen. */
    widget->wpm_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->wpm_label, "0");
    lv_obj_set_style_text_font(widget->wpm_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(widget->wpm_label, LV_ALIGN_CENTER, 0, 6);

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_init();
    return 0;
}

lv_obj_t *zmk_widget_wpm_obj(struct zmk_widget_wpm *widget) { return widget->obj; }
