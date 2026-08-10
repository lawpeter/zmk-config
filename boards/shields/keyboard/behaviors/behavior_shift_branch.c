/*
 * &shift_branch behavior — picks between two sub-bindings based on whether
 * Shift is currently held, exactly like zmk,behavior-mod-morph, but WITHOUT
 * mod-morph's built-in modifier-masking side effect.
 *
 * REVISION — why this exists instead of just using mod-morph directly:
 * mod-morph's own on_mod_morph_binding_released() ALWAYS calls
 * zmk_hid_masked_modifiers_clear() unconditionally, tied to the physical
 * LETTER key's own release. The sym_layer "shift variant" macros (see
 * keyboard.keymap) do their OWN masking internally via &shift_mask, scoped
 * to their own macro's start-to-finish window (which runs asynchronously
 * and can take longer to drain than the physical key is held). Both
 * mechanisms write the SAME global masked_modifiers variable (zmk/hid.c) —
 * it's a flat set/clear, not reference-counted — so mod-morph's clear could
 * fire in the middle of a macro's own masked window and wipe it out early,
 * letting the tail of that macro's keystrokes go out shifted again even
 * though the macro's own &shift_mask hadn't released yet. That was
 * confirmed as the reason "hold shift across multiple letters" kept
 * breaking even after &shift_mask was added.
 *
 * &shift_branch does ONLY the branching decision (which mod-morph also
 * does) and leaves modifier masking entirely to whatever the invoked
 * binding does on its own — for our macros, that's &shift_mask, and
 * nothing else competes with it anymore.
 */

#define DT_DRV_COMPAT zmk_behavior_shift_branch

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_shift_branch_config {
    struct zmk_behavior_binding normal_binding;
    struct zmk_behavior_binding shifted_binding;
    zmk_mod_flags_t mods;
};

struct behavior_shift_branch_data {
    struct zmk_behavior_binding *pressed_binding;
};

static int on_shift_branch_binding_pressed(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_shift_branch_config *cfg = dev->config;
    struct behavior_shift_branch_data *data = dev->data;

    if (data->pressed_binding != NULL) {
        LOG_ERR("Can't press the same shift-branch twice");
        return -ENOTSUP;
    }

    if (zmk_hid_get_explicit_mods() & cfg->mods) {
        data->pressed_binding = (struct zmk_behavior_binding *)&cfg->shifted_binding;
    } else {
        data->pressed_binding = (struct zmk_behavior_binding *)&cfg->normal_binding;
    }
    return zmk_behavior_invoke_binding(data->pressed_binding, event, true);
}

static int on_shift_branch_binding_released(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_shift_branch_data *data = dev->data;

    if (data->pressed_binding == NULL) {
        LOG_ERR("Shift-branch already released");
        return -ENOTSUP;
    }

    struct zmk_behavior_binding *pressed_binding = data->pressed_binding;
    data->pressed_binding = NULL;
    return zmk_behavior_invoke_binding(pressed_binding, event, false);
}

static const struct behavior_driver_api behavior_shift_branch_driver_api = {
    .binding_pressed = on_shift_branch_binding_pressed,
    .binding_released = on_shift_branch_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define _SB_TRANSFORM_ENTRY(idx, node)                                                            \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),               \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    }

#define SHIFT_BRANCH_INST(n)                                                                       \
    static struct behavior_shift_branch_config behavior_shift_branch_config_##n = {                \
        .normal_binding = _SB_TRANSFORM_ENTRY(0, n),                                               \
        .shifted_binding = _SB_TRANSFORM_ENTRY(1, n),                                              \
        .mods = DT_INST_PROP(n, mods),                                                             \
    };                                                                                             \
    static struct behavior_shift_branch_data behavior_shift_branch_data_##n = {};                  \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &behavior_shift_branch_data_##n,                        \
                            &behavior_shift_branch_config_##n, POST_KERNEL,                        \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_shift_branch_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SHIFT_BRANCH_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
