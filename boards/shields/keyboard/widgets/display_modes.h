/*
 * PRD §11 step 7 — display mode constants shared between display_modes.c
 * and behavior_disp_cycle.c.
 */
#pragma once

/* Total number of display modes; indices 0 .. DISPLAY_MODE_COUNT-1. */
#define DISPLAY_MODE_COUNT 7

/* Returns the currently active mode index (0-based). */
uint8_t zmk_display_mode_get(void);
