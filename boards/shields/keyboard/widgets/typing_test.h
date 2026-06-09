/*
 * PRD §11 step 9 — Typing-test display widget for mode 4.
 */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

/* Canvas size must match CANVAS_SIZE in util.h (68).  util.h has no include
 * guard so we cannot include it here; use the literal instead.               */
#define TT_CANVAS_PX 68

struct zmk_widget_typing_test {
    sys_snode_t  node;
    lv_obj_t    *obj;
    lv_obj_t    *canvas;
    lv_color_t   cbuf[TT_CANVAS_PX * TT_CANVAS_PX];
};

int      zmk_widget_typing_test_init(struct zmk_widget_typing_test *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_typing_test_obj(struct zmk_widget_typing_test *widget);
