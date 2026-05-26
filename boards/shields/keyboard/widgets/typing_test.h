/*
 * PRD §11 step 9 — Typing-test display widget for mode 4.
 */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_typing_test {
    sys_snode_t node;
    lv_obj_t   *obj;
    lv_obj_t   *status_label; /* "TYPING TEST" / "IN PROGRESS" / "RESULTS" */
    lv_obj_t   *result_label; /* detail line(s) — WPM, CPM, time, keys     */
};

int      zmk_widget_typing_test_init(struct zmk_widget_typing_test *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_typing_test_obj(struct zmk_widget_typing_test *widget);
