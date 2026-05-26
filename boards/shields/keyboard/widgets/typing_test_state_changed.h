/*
 * PRD §11 step 9 — Event raised when typing-test state changes.
 *
 * Fired by behavior_typing_test.c on start AND stop.
 * Consumed by typing_test.c (display widget for mode 4).
 */
#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

enum zmk_typing_test_phase {
    ZMK_TYPING_TEST_IDLE    = 0, /* not running; shows "FN+T to start" */
    ZMK_TYPING_TEST_RUNNING = 1, /* in progress */
    ZMK_TYPING_TEST_DONE    = 2, /* finished; results available         */
};

struct zmk_typing_test_state_changed {
    enum zmk_typing_test_phase phase;
    uint32_t wpm;       /* words per minute  (meaningful when DONE) */
    uint32_t cpm;       /* chars per minute  (meaningful when DONE) */
    uint32_t elapsed_s; /* test duration, seconds (meaningful when DONE) */
    uint32_t key_count; /* total keystrokes  (meaningful when DONE) */
};

ZMK_EVENT_DECLARE(zmk_typing_test_state_changed);
