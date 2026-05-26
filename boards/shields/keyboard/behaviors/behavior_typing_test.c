/*
 * PRD §11 step 9 — &typing_test_toggle behavior.
 *
 * First press (key-down): starts the typing test.
 *   - Records start timestamp and resets counters.
 *   - Raises zmk_typing_test_state_changed{phase=RUNNING}.
 *
 * Second press (key-down): stops the typing test.
 *   - Computes WPM (words / elapsed_minutes) and CPM (chars / elapsed_minutes).
 *   - Raises zmk_typing_test_state_changed{phase=DONE, wpm, cpm, elapsed_s, key_count}.
 *
 * While a test is active this module also subscribes to zmk_keycode_state_changed
 * (via ZMK_LISTENER / ZMK_SUBSCRIPTION) to count keystrokes and word-boundaries.
 *
 * Thread safety: both the behavior callbacks and the keycode listener run on
 * the ZMK event thread, so no extra locking is needed.
 */

#define DT_DRV_COMPAT zmk_behavior_typing_test

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

#include "widgets/typing_test_state_changed.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── Shared typing-test state ────────────────────────────────────────────── */

static struct {
    bool    active;
    int64_t start_time_ms;
    uint32_t keystroke_count; /* every key-down on the keyboard page */
    uint32_t word_count;      /* space or enter key presses           */
} tt;

/* ── Keycode listener — counts while test is active ─────────────────────── */

static int tt_keycode_listener(const zmk_event_t *eh) {
    if (!tt.active) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (!ev || !ev->state) {
        /* Ignore key-release events. */
        return ZMK_EV_EVENT_BUBBLE;
    }
    if (ev->usage_page != HID_USAGE_KEY) {
        /* Ignore consumer / mouse / other pages. */
        return ZMK_EV_EVENT_BUBBLE;
    }

    tt.keystroke_count++;

    /* Word boundary: space or enter. */
    if (ev->keycode == HID_USAGE_KEY_KEYBOARD_SPACEBAR ||
        ev->keycode == HID_USAGE_KEY_KEYBOARD_RETURN_ENTER) {
        tt.word_count++;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(tt_keycode_listener_inst, tt_keycode_listener);
ZMK_SUBSCRIPTION(tt_keycode_listener_inst, zmk_keycode_state_changed);

/* ── Behavior callbacks ───────────────────────────────────────────────────── */

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    if (!tt.active) {
        /* ── Start ─────────────────────────────────────────────────────── */
        tt.active          = true;
        tt.start_time_ms   = event.timestamp;
        tt.keystroke_count = 0;
        tt.word_count      = 0;

        LOG_DBG("typing_test: started");

        return raise_zmk_typing_test_state_changed(
            (struct zmk_typing_test_state_changed){
                .phase = ZMK_TYPING_TEST_RUNNING,
            });
    } else {
        /* ── Stop ──────────────────────────────────────────────────────── */
        tt.active = false;

        int64_t elapsed_ms = event.timestamp - tt.start_time_ms;
        uint32_t elapsed_s = (uint32_t)(elapsed_ms / 1000);

        uint32_t wpm = 0, cpm = 0;
        if (elapsed_ms > 0) {
            /* words  / (elapsed_ms / 60 000) = words * 60 000 / elapsed_ms */
            wpm = (uint32_t)(((uint64_t)tt.word_count * 60000ULL)
                             / (uint64_t)elapsed_ms);
            cpm = (uint32_t)(((uint64_t)tt.keystroke_count * 60000ULL)
                             / (uint64_t)elapsed_ms);
        }

        LOG_DBG("typing_test: done  wpm=%u cpm=%u elapsed=%us keys=%u",
                wpm, cpm, elapsed_s, tt.keystroke_count);

        return raise_zmk_typing_test_state_changed(
            (struct zmk_typing_test_state_changed){
                .phase     = ZMK_TYPING_TEST_DONE,
                .wpm       = wpm,
                .cpm       = cpm,
                .elapsed_s = elapsed_s,
                .key_count = tt.keystroke_count,
            });
    }
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_typing_test_driver_api = {
    .binding_pressed  = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

/* ── Device instantiation ────────────────────────────────────────────────── */

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define TYPING_TEST_INST(n)                                                    \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,            \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,               \
                            &behavior_typing_test_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TYPING_TEST_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY */
