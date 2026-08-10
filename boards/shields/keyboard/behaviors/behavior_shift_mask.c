/*
 * &shift_mask behavior — suppresses Shift from the outgoing HID report
 * between press and release, without touching real key-press state.
 *
 * Used inside the uppercase-Greek sym_layer macros (see keyboard.keymap) to
 * type Espanso trigger text that must come out unshifted even while the
 * user is still physically holding Shift. zmk_hid_masked_modifiers_set/
 * clear (zmk/hid.h — the same mechanism zmk,behavior-mod-morph already uses
 * internally) is a pure real-time filter on the outgoing report; it never
 * touches explicit_modifiers itself, so unlike explicitly injecting &kp
 * LSHFT press/release there's no risk of a "stuck shift" if the user
 * releases the physical key mid-macro — the filter just stops applying the
 * moment &shift_mask releases, and whatever the real modifier state is at
 * that point is what shows through.
 */

#define DT_DRV_COMPAT zmk_behavior_shift_mask

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/keys.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    zmk_hid_masked_modifiers_set(MOD_LSFT | MOD_RSFT);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    zmk_hid_masked_modifiers_clear();
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_shift_mask_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define SHIFT_MASK_INST(n)                                                                         \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_shift_mask_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SHIFT_MASK_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
