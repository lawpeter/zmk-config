/*
 * PRD §11 step 8 — Live WPM counter widget for display mode 1.
 */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_wpm {
    sys_snode_t node;
    lv_obj_t   *obj;
    lv_obj_t   *wpm_label; /* updated on every zmk_wpm_state_changed event */
};

int      zmk_widget_wpm_init(struct zmk_widget_wpm *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_wpm_obj(struct zmk_widget_wpm *widget);
