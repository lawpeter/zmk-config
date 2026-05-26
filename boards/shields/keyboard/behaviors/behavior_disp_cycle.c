/*
 * PRD §11 step 7 — &disp_cycle behavior.
 *
 * Short press (< tapping-term-ms): advance to the next display mode.
 * Long press  (>= tapping-term-ms): reset to mode 0 (the status screen).
 *
 * On release, raises zmk_display_mode_state_changed with the new mode index.
 * display_modes.c subscribes and calls lv_scr_load() on the display thread.
 */

#define DT_DRV_COMPAT zmk_behavior_disp_cycle

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zephyr/logging/log.h>

#include "widgets/display_mode_state_changed.h"
#include "widgets/display_modes.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_disp_cycle_config {
    uint32_t tapping_term_ms;
};

struct behavior_disp_cycle_data {
    int64_t press_time; /* k_uptime_get() at the moment of key press */
};

/* Mode index, 0-based (0 = status screen). Shared across all layers since
 * there is exactly one disp_cycle instance per keyboard. */
static uint8_t current_mode;

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_disp_cycle_data *data = dev->data;
    data->press_time = event.timestamp;
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_disp_cycle_config *cfg = dev->config;
    struct behavior_disp_cycle_data *data = dev->data;

    int64_t held_ms = event.timestamp - data->press_time;

    if (held_ms >= (int64_t)cfg->tapping_term_ms) {
        /* Long press: reset to status screen (mode 0 / PRD "mode 1"). */
        current_mode = 0;
    } else {
        /* Short press: advance, wrapping back to 0 after the last mode. */
        current_mode = (current_mode + 1) % DISPLAY_MODE_COUNT;
    }

    LOG_DBG("disp_cycle: held %lld ms → mode %d", held_ms, current_mode);

    return raise_zmk_display_mode_state_changed(
        (struct zmk_display_mode_state_changed){.mode = current_mode});
}

static const struct behavior_driver_api behavior_disp_cycle_driver_api = {
    .binding_pressed  = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define DISP_CYCLE_INST(n)                                                                         \
    static struct behavior_disp_cycle_data disp_cycle_data_##n = {};                               \
    static const struct behavior_disp_cycle_config disp_cycle_config_##n = {                       \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &disp_cycle_data_##n, &disp_cycle_config_##n,           \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_disp_cycle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DISP_CYCLE_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
