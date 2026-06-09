/*
 * PRD §11 step 8 — Live WPM counter widget for display mode 1.
 *
 * Renders into a 68×68 canvas and calls rotate_canvas (270° CW) so the
 * content appears portrait-correct on the physical display.
 * Canvas is centred horizontally in the 160×68 LVGL space (x-offset 46).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

#include "util.h"   /* rotate_canvas, CANVAS_SIZE, LVGL_BACKGROUND/FOREGROUND */
#include "wpm.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct wpm_state { uint8_t wpm; };

static sys_slist_t widgets;

/* ── Canvas draw helper ─────────────────────────────────────────────────── */

static void draw_wpm_canvas(struct zmk_widget_wpm *widget, uint8_t wpm) {
    lv_draw_rect_dsc_t  rect_bg;
    lv_draw_label_dsc_t lbl_title, lbl_value;

    lv_draw_rect_dsc_init(&rect_bg);
    rect_bg.bg_color = LVGL_BACKGROUND;

    lv_draw_label_dsc_init(&lbl_title);
    lbl_title.color = LVGL_FOREGROUND;
    lbl_title.font  = &lv_font_montserrat_14;
    lbl_title.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_label_dsc_init(&lbl_value);
    lbl_value.color = LVGL_FOREGROUND;
    lbl_value.font  = &lv_font_montserrat_18;
    lbl_value.align = LV_TEXT_ALIGN_CENTER;

    /* Clear background */
    lv_canvas_draw_rect(widget->canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_bg);

    /* "WPM" title near top */
    lv_canvas_draw_text(widget->canvas, 0, 6, CANVAS_SIZE, &lbl_title, "WPM");

    /* Live counter centred vertically */
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", wpm);
    lv_canvas_draw_text(widget->canvas, 0, 28, CANVAS_SIZE, &lbl_value, buf);

    rotate_canvas(widget->canvas, widget->cbuf);
}

/* ── ZMK event plumbing ─────────────────────────────────────────────────── */

static void set_wpm_state(struct wpm_state state) {
    struct zmk_widget_wpm *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        draw_wpm_canvas(widget, state.wpm);
    }
}

static struct wpm_state wpm_get_state(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    return (struct wpm_state){.wpm = ev ? ev->state : zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm, struct wpm_state, set_wpm_state, wpm_get_state)
ZMK_SUBSCRIPTION(widget_wpm, zmk_wpm_state_changed);

/* ── Public API ─────────────────────────────────────────────────────────── */

int zmk_widget_wpm_init(struct zmk_widget_wpm *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);

    widget->canvas = lv_canvas_create(widget->obj);
    lv_obj_align(widget->canvas, LV_ALIGN_TOP_LEFT, 46, 0);
    lv_canvas_set_buffer(widget->canvas, widget->cbuf,
                         CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    widget_wpm_init();
    return 0;
}

lv_obj_t *zmk_widget_wpm_obj(struct zmk_widget_wpm *widget) { return widget->obj; }
