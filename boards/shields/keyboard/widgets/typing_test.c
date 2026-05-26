/*
 * PRD §11 step 9 — Typing-test display widget for mode 4.
 *
 * Layout (160 × 68 nice!view Sharp Memory LCD):
 *
 *   ┌──────────────────────────┐
 *   │      TYPING TEST         │  ← status_label (MONTSERRAT_14, top-center)
 *   │                          │
 *   │     FN+T to start        │  ← result_label (MONTSERRAT_14, center)
 *   │   (or results, or …)     │
 *   └──────────────────────────┘
 *
 * Subscribes to zmk_typing_test_state_changed via ZMK_DISPLAY_WIDGET_LISTENER.
 * The behavior_typing_test.c module raises that event on start and stop.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>

#include "typing_test_state_changed.h"
#include "typing_test.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── Internal state type passed to the display thread ────────────────────── */

struct tt_display_state {
    enum zmk_typing_test_phase phase;
    uint32_t wpm;
    uint32_t cpm;
    uint32_t elapsed_s;
    uint32_t key_count;
};

/* ── Widget instance list ────────────────────────────────────────────────── */

static sys_slist_t widgets;

/* ── Display-thread update callback ─────────────────────────────────────── */

static void tt_update_cb(struct zmk_widget_typing_test *widget,
                         struct tt_display_state state) {
    char buf[48];

    switch (state.phase) {
    case ZMK_TYPING_TEST_IDLE:
        lv_label_set_text(widget->status_label, "TYPING TEST");
        lv_label_set_text(widget->result_label, "FN+T to start");
        break;

    case ZMK_TYPING_TEST_RUNNING:
        lv_label_set_text(widget->status_label, "IN PROGRESS");
        lv_label_set_text(widget->result_label, "Type now...");
        break;

    case ZMK_TYPING_TEST_DONE:
        lv_label_set_text(widget->status_label, "RESULTS");
        snprintf(buf, sizeof(buf), "WPM:%u CPM:%u\n%us %u keys",
                 state.wpm, state.cpm, state.elapsed_s, state.key_count);
        lv_label_set_text(widget->result_label, buf);
        break;
    }
}

/* ── Iterate widget list on every event ──────────────────────────────────── */

static void set_tt_state(struct tt_display_state state) {
    struct zmk_widget_typing_test *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        tt_update_cb(widget, state);
    }
}

/* ── ZMK_DISPLAY_WIDGET_LISTENER plumbing ────────────────────────────────── */

static struct tt_display_state tt_get_state(const zmk_event_t *eh) {
    const struct zmk_typing_test_state_changed *ev =
        as_zmk_typing_test_state_changed(eh);
    if (!ev) {
        /* Init call (eh == NULL): show idle state. */
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

/* ── Public API ──────────────────────────────────────────────────────────── */

int zmk_widget_typing_test_init(struct zmk_widget_typing_test *widget,
                                lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);

    /* Status header — top-center. */
    widget->status_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->status_label, "TYPING TEST");
    lv_obj_set_style_text_font(widget->status_label, &lv_font_montserrat_14,
                               LV_PART_MAIN);
    lv_obj_align(widget->status_label, LV_ALIGN_TOP_MID, 0, 4);

    /* Detail / result — center, wrapping. */
    widget->result_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->result_label, "FN+T to start");
    lv_obj_set_style_text_font(widget->result_label, &lv_font_montserrat_14,
                               LV_PART_MAIN);
    lv_label_set_long_mode(widget->result_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(widget->result_label, LV_PCT(90));
    lv_obj_align(widget->result_label, LV_ALIGN_CENTER, 0, 8);

    sys_slist_append(&widgets, &widget->node);

    widget_typing_test_init();
    return 0;
}

lv_obj_t *zmk_widget_typing_test_obj(struct zmk_widget_typing_test *widget) {
    return widget->obj;
}
