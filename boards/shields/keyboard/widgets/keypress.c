/*
 * PRD v2 §3 — Live keypress readout widget (display mode 5).
 *
 * Data source: zmk_keycode_state_changed. That event is raised on the ZMK
 * dispatch thread *after* the behavior/layer stack has resolved the key, so
 * (a) the label reflects what the key actually sends, post-layer, and (b) none
 * of this runs in the kscan / I2C scan loop. The redraw is further deferred to
 * the LVGL display work queue by ZMK_DISPLAY_WIDGET_LISTENER.
 *
 * The held-key bookkeeping is done in the state_func (event context, one call
 * per event, lossless) rather than the display callback — the display listener
 * only keeps the latest snapshot and would drop intermediate press/release
 * events under load, desyncing the held list.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/events/keycode_state_changed.h>

#include "util.h" /* rotate_canvas, CANVAS_SIZE, LVGL_BACKGROUND/FOREGROUND */
#include "keypress.h"
#include "keycode_labels.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* How many simultaneous keys we track. Only 3 are shown (plus a "+"), but we
 * track a few more so releasing one still shows the rest correctly. */
#define KP_MAX_TRACK 6
#define KP_MAX_SHOWN 3
#define KP_LABEL_LEN 12
#define KP_DUR_LEN 8

struct kp_key {
    uint16_t page;
    uint32_t code;
    uint8_t  imods;
    uint8_t  emods;
    int64_t  ts; /* press timestamp */
};

/* ── Live model — touched only from the event-context state_func ───────────── */

static struct kp_key kp_held[KP_MAX_TRACK];
static uint8_t       kp_held_count;

static struct {
    bool valid;
    char label[KP_LABEL_LEN];
    char dur[KP_DUR_LEN];
} kp_last;

/* ── Render snapshot — copied to the display thread ───────────────────────── */

struct kp_state {
    uint8_t held_count;
    bool    overflow;
    char    lines[KP_MAX_SHOWN][KP_LABEL_LEN];
    bool    show_release;
    char    rel_label[KP_LABEL_LEN];
    char    rel_dur[KP_DUR_LEN];
};

static sys_slist_t widgets;

/* ── Model updates ────────────────────────────────────────────────────────── */

static int kp_find(uint16_t page, uint32_t code) {
    for (int i = 0; i < kp_held_count; i++) {
        if (kp_held[i].page == page && kp_held[i].code == code) {
            return i;
        }
    }
    return -1;
}

static void kp_on_press(const struct zmk_keycode_state_changed *ev) {
    if (kp_find(ev->usage_page, ev->keycode) >= 0) {
        return; /* auto-repeat / duplicate */
    }
    if (kp_held_count >= KP_MAX_TRACK) {
        /* Drop the oldest to make room — keeps the display responsive to the
         * most recent activity rather than freezing on a stuck entry. */
        memmove(&kp_held[0], &kp_held[1], sizeof(kp_held[0]) * (KP_MAX_TRACK - 1));
        kp_held_count--;
    }
    kp_held[kp_held_count++] = (struct kp_key){
        .page = ev->usage_page,
        .code = ev->keycode,
        .imods = ev->implicit_modifiers,
        .emods = ev->explicit_modifiers,
        .ts = ev->timestamp,
    };
}

static void kp_on_release(const struct zmk_keycode_state_changed *ev) {
    int idx = kp_find(ev->usage_page, ev->keycode);
    if (idx < 0) {
        return;
    }
    struct kp_key released = kp_held[idx];
    memmove(&kp_held[idx], &kp_held[idx + 1], sizeof(kp_held[0]) * (kp_held_count - idx - 1));
    kp_held_count--;

    if (kp_held_count == 0) {
        kc_label(kp_last.label, sizeof(kp_last.label), released.page, released.code,
                 released.imods, released.emods);
        kc_fmt_hold(kp_last.dur, sizeof(kp_last.dur), ev->timestamp - released.ts);
        kp_last.valid = true;
    }
}

static struct kp_state kp_build_snapshot(void) {
    struct kp_state s = {0};
    s.held_count = kp_held_count;

    if (kp_held_count > 0) {
        uint8_t shown = kp_held_count > KP_MAX_SHOWN ? KP_MAX_SHOWN : kp_held_count;
        s.overflow = kp_held_count > KP_MAX_SHOWN;
        for (uint8_t i = 0; i < shown; i++) {
            kc_label(s.lines[i], KP_LABEL_LEN, kp_held[i].page, kp_held[i].code,
                     kp_held[i].imods, kp_held[i].emods);
        }
    } else if (kp_last.valid) {
        s.show_release = true;
        strncpy(s.rel_label, kp_last.label, sizeof(s.rel_label) - 1);
        strncpy(s.rel_dur, kp_last.dur, sizeof(s.rel_dur) - 1);
    }
    return s;
}

/* ── Drawing ─────────────────────────────────────────────────────────────── */

static void kp_draw(struct zmk_widget_keypress *widget, const struct kp_state *s) {
    lv_draw_rect_dsc_t rect_bg;
    lv_draw_label_dsc_t lbl_sm, lbl_lg;

    lv_draw_rect_dsc_init(&rect_bg);
    rect_bg.bg_color = LVGL_BACKGROUND;

    lv_draw_label_dsc_init(&lbl_sm);
    lbl_sm.color = LVGL_FOREGROUND;
    lbl_sm.font = &lv_font_unscii_8;
    lbl_sm.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_label_dsc_init(&lbl_lg);
    lbl_lg.color = LVGL_FOREGROUND;
    lbl_lg.font = &lv_font_montserrat_14;
    lbl_lg.align = LV_TEXT_ALIGN_CENTER;

    lv_canvas_draw_rect(widget->canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_bg);

    if (s->held_count > 0) {
        /* Up to 3 stacked labels at ~17px pitch, then "+" if more. */
        static const lv_coord_t y[KP_MAX_SHOWN] = {4, 21, 38};
        uint8_t shown = s->overflow ? KP_MAX_SHOWN : s->held_count;
        for (uint8_t i = 0; i < shown; i++) {
            lv_canvas_draw_text(widget->canvas, 0, y[i], CANVAS_SIZE, &lbl_sm, s->lines[i]);
        }
        if (s->overflow) {
            lv_canvas_draw_text(widget->canvas, 0, 54, CANVAS_SIZE, &lbl_sm, "+");
        }
    } else if (s->show_release) {
        lv_canvas_draw_text(widget->canvas, 0, 10, CANVAS_SIZE, &lbl_lg, s->rel_label);
        lv_canvas_draw_text(widget->canvas, 0, 36, CANVAS_SIZE, &lbl_sm, s->rel_dur);
    } else {
        lv_canvas_draw_text(widget->canvas, 0, 22, CANVAS_SIZE, &lbl_lg, "PRESS");
    }

    rotate_canvas(widget->canvas, widget->cbuf);
}

/* ── ZMK event plumbing ──────────────────────────────────────────────────── */

static void kp_set_state(struct kp_state state) {
    struct zmk_widget_keypress *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        kp_draw(widget, &state);
    }
}

static struct kp_state kp_get_state(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = eh ? as_zmk_keycode_state_changed(eh) : NULL;
    if (ev) {
        if (ev->state) {
            kp_on_press(ev);
        } else {
            kp_on_release(ev);
        }
    }
    return kp_build_snapshot();
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_keypress, struct kp_state, kp_set_state, kp_get_state)
ZMK_SUBSCRIPTION(widget_keypress, zmk_keycode_state_changed);

/* ── Public API ─────────────────────────────────────────────────────────── */

int zmk_widget_keypress_init(struct zmk_widget_keypress *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);

    widget->canvas = lv_canvas_create(widget->obj);
    lv_obj_align(widget->canvas, LV_ALIGN_TOP_LEFT, 46, 0);
    lv_canvas_set_buffer(widget->canvas, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE,
                         LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    widget_keypress_init();
    return 0;
}

lv_obj_t *zmk_widget_keypress_obj(struct zmk_widget_keypress *widget) { return widget->obj; }
