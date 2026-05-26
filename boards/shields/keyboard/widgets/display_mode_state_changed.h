/*
 * PRD §11 step 7 — event raised by the &disp_cycle behavior when the
 * active display mode changes. display_modes.c subscribes and calls
 * lv_scr_load() to show the appropriate LVGL screen.
 */
#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_display_mode_state_changed {
    uint8_t mode; /* 0-indexed: 0 = status screen (PRD mode 1), … */
};

ZMK_EVENT_DECLARE(zmk_display_mode_state_changed);
