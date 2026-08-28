/*
 * PRD v2 §3 — Live keypress readout widget (display mode 5).
 *
 * Renders into a 68×68 canvas and calls rotate_canvas (270° CW), same as the
 * other mode widgets. Shows the label(s) of the currently-pressed key(s), up
 * to three stacked with a "+" for more; on full release it holds the last key
 * and shows how long it was held.
 */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

/* Must match CANVAS_SIZE in util.h (68); util.h has no include guard so it
 * can't be pulled in here — same pattern as wpm.h. */
#define KEYPRESS_CANVAS_PX 68

struct zmk_widget_keypress {
    sys_snode_t node;
    lv_obj_t   *obj;
    lv_obj_t   *canvas;
    lv_color_t  cbuf[KEYPRESS_CANVAS_PX * KEYPRESS_CANVAS_PX];
};

int zmk_widget_keypress_init(struct zmk_widget_keypress *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_keypress_obj(struct zmk_widget_keypress *widget);
