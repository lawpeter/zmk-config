/*
 * PRD §11 step 8 — Live WPM counter widget for display mode 1.
 */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

/* Canvas size must match CANVAS_SIZE in util.h (68).  util.h has no include
 * guard so we cannot include it here; use the literal instead.               */
#define WPM_CANVAS_PX 68

struct zmk_widget_wpm {
    sys_snode_t  node;
    lv_obj_t    *obj;
    lv_obj_t    *canvas;
    lv_color_t   cbuf[WPM_CANVAS_PX * WPM_CANVAS_PX];
};

int      zmk_widget_wpm_init(struct zmk_widget_wpm *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_wpm_obj(struct zmk_widget_wpm *widget);
