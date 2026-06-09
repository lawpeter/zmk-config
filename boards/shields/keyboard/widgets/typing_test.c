/*
 * PRD §11 step 9 — Typing-test display widget for mode 4.
 *
 * Renders into a 68×68 canvas and calls rotate_canvas (270° CW) so the
 * content appears portrait-correct on the physical display.
 * Canvas is centred horizontally in the 160×68 LVGL space (x-offset 46).
 *
 * Canvas layout (pre-rotation coordinates):
 *   y= 2: status line  ("TYPING TEST" / "IN PROGRESS" / "RESULTS")
 *   y=20: detail line 1
 *   y=38: detail line 2 (results only)
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>

#include "util.h"   /* rotate_canvas, CANVAS_SIZE, LVGL_BACKGROUND/FOREGROUND */
#include "typing_test_state_changed.h"
#include "typing_test.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct tt_display_state {
    enum zmk_typing_test_phase phase;
    uint32_t wpm;
    uint32_t cpm;
    uint32_t elapsed_s;
    uint32_t key_count;
};

static sys_slist_t widgets;

/* ── Canvas draw helper ─────────────────────────────────────────────────── */

static void draw_tt_canvas(struct zmk_widget_typing_test *widget,
                            struct tt_display_state state) {
    lv_draw_rect_dsc_t  rect_bg;
    lv_draw_label_dsc_t lbl;

    lv_draw_rect_dsc_init(&rect_bg);
    rect_bg.bg_color = LVGL_BACKGROUND;

    lv_draw_label_dsc_init(&lbl);
    lbl.color = LVGL_FOREGROUND;
    lbl.font  = &lv_font_montserrat_14;
    lbl.align = LV_TEXT_ALIGN_CENTER;

    /* Clear background */
    lv_canvas_draw_rect(widget->canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_bg);

    char detail1[24] = {};
    char detail2[24] = {};

    switch (state.phase) {
    case ZMK_TYPING_TEST_IDLE:
        lv_canvas_draw_text(widget->canvas, 0, 2, CANVAS_SIZE, &lbl, "TYPING TEST");
        lv_canvas_draw_text(widget->canvas, 0, 24, CANVAS_SIZE, &lbl, "FN+T to start");
        break;

    case ZMK_TYPING_TEST_RUNNING:
        lv_canvas_draw_text(widget->canvas, 0, 2, CANVAS_SIZE, &lbl, "IN PROGRESS");
        lv_canvas_draw_text(widget->canvas, 0, 28, CANVAS_SIZE, &lbl, "Type now...");
        break;

    case ZMK_TYPING_TEST_DONE:
        lv_canvas_draw_text(widget->canvas, 0, 2, CANVAS_SIZE, &lbl, "RESULTS");
        snprintf(detail1, sizeof(detail1), "WPM:%u CPM:%u", state.wpm, state.cpm);
        snprintf(detail2, sizeof(detail2), "%us  %u keys", state.elapsed_s, state.key_count);
        lv_canvas_draw_text(widget->canvas, 0, 20, CANVAS_SIZE, &lbl, detail1);
        lv_canvas_draw_text(widget->canvas, 0, 44, CANVAS_SIZE, &lbl, detail2);
        break;
    }

    rotate_canvas(widget->canvas, widget->cbuf);
}

/* ── ZMK event plumbing ─────────────────────────────────────────────────── */

static void set_tt_state(struct tt_display_state state) {
    struct zmk_widget_typing_test *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        draw_tt_canvas(widget, state);
    }
}

static struct tt_display_state tt_get_state(const zmk_event_t *eh) {
    const struct zmk_typing_test_state_changed *ev = as_zmk_typing_test_state_changed(eh);
    if (!ev) {
        return (struct tt_display_state){.phase = ZMK_TYPING_TEST_IDLE};
    }
    return (struct tt_display_state){
        .phase     = ev->phase,
        .wpm       = ev->wpm,
        .cpm       = ev->cpm,
        .elapsed_s = ev->elapsed_s,
        .key_count = ev->key_count,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_typing_test, struct tt_display_state,
                             set_tt_state, tt_get_state)
ZMK_SUBSCRIPTION(widget_typing_test, zmk_typing_test_state_changed);

/* ── Public API ─────────────────────────────────────────────────────────── */

int zmk_widget_typing_test_init(struct zmk_widget_typing_test *widget,
                                lv_obj_t *parent) {
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
    widget_typing_test_init();
    return 0;
}

lv_obj_t *zmk_widget_typing_test_obj(struct zmk_widget_typing_test *widget) {
    return widget->obj;
}
