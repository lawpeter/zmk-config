/*
 * PRD §11 step 7 — display mode constants shared between display_modes.c
 * and behavior_disp_cycle.c.
 */
#pragma once

/* Total number of display modes; indices 0 .. DISPLAY_MODE_COUNT-1.
 *   0 Status  1 WPM  2 Battery  3 TypingTest  4 Keypress
 * REVISION: was 7 (Status/WPM/Battery/Volume/TypingTest/Uptime/Animation) —
 * Volume, Uptime, and Animation were static-text placeholders with no real
 * data behind them (ZMK has no volume state, and the other two were never
 * built out), so they were removed rather than kept as dead screens.
 * PRD v2 §3: Keypress (mode 4) added — live post-layer key readout, real data. */
#define DISPLAY_MODE_COUNT 5

/* Returns the currently active mode index (0-based). */
uint8_t zmk_display_mode_get(void);
