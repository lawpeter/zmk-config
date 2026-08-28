/*
 * PRD v2 §2 — &os_mode_toggle behavior.
 *
 * Replaces `&tog 5` on device_layer's `0` key. Two jobs:
 *
 *   §2a  Boot default is PC/Windows. The pc_mods layer is activated at init
 *        when no stored value exists (first boot after flashing).
 *
 *   §2b  The selected mode persists across reboot / firmware reset. On each
 *        real toggle the new state (1 = PC, 0 = Mac) is written to the Zephyr
 *        settings subsystem — the same NVS backend ZMK already uses for BT
 *        profiles. Restored during settings_load(), before any key event is
 *        processed, so the wrong modifier map is never briefly live to the user.
 *
 * Writes happen only on an actual change (the key is only reachable while
 * holding the momentary device layer, never during normal typing) and are
 * debounced through a delayable work item, so NVS flash endurance is a
 * non-issue.
 *
 * "PC mode" is exactly "pc_mods (layer 5) active", unchanged from the previous
 * &tog 5 design — the conditional pc_lock_layer wiring and the base layer are
 * untouched.
 */

#define DT_DRV_COMPAT zmk_behavior_os_mode

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

/* pc_mods layer id, from the (single) instance's `layer` property so this can
 * never drift from the keymap. */
#define OS_MODE_PC_LAYER DT_PROP(DT_DRV_INST(0), layer)

/* 1 = PC/Windows, 0 = Mac. The initializer is the PRD v2 §2a Windows default,
 * used verbatim when flash holds no stored value. */
static uint8_t os_mode_pc = 1;

static void os_mode_apply(void) {
    if (os_mode_pc) {
        zmk_keymap_layer_activate(OS_MODE_PC_LAYER);
    } else {
        zmk_keymap_layer_deactivate(OS_MODE_PC_LAYER);
    }
}

#if IS_ENABLED(CONFIG_SETTINGS)

static void os_mode_save_work_handler(struct k_work *work) {
    uint8_t val = os_mode_pc;
    int rc = settings_save_one("os_mode/pc", &val, sizeof(val));
    if (rc) {
        LOG_ERR("os_mode: settings_save_one failed (%d)", rc);
    }
}
static K_WORK_DELAYABLE_DEFINE(os_mode_save_work, os_mode_save_work_handler);

static void os_mode_save(void) {
    k_work_reschedule(&os_mode_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
}

static int os_mode_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                void *cb_arg) {
    const char *next;

    if (settings_name_steq(name, "pc", &next) && !next) {
        uint8_t val;
        if (len != sizeof(val)) {
            return -EINVAL;
        }
        int rc = read_cb(cb_arg, &val, sizeof(val));
        if (rc < 0) {
            return rc;
        }
        os_mode_pc = val ? 1 : 0;
        os_mode_apply();
        LOG_DBG("os_mode: restored from settings -> %s", os_mode_pc ? "PC" : "Mac");
        return 0;
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(os_mode, "os_mode", NULL, os_mode_settings_set, NULL, NULL);

#else /* !CONFIG_SETTINGS */

static void os_mode_save(void) {}

#endif /* CONFIG_SETTINGS */

/* Boot-time apply.
 *
 * Priority 99 in APPLICATION runs *after* keymap_init (APPLICATION,
 * CONFIG_APPLICATION_INIT_PRIORITY == 90) so layer state is set up, and
 * *before* main() reaches settings_load(). Sequence:
 *
 *   1. no stored value  -> os_mode_pc stays 1, layer activated here. Done.
 *   2. stored value     -> layer set here from the default, then
 *      settings_load() -> os_mode_settings_set() immediately re-applies the
 *      real value. Both happen before the kscan work queue delivers the first
 *      position event to the keymap, so there is no user-visible window with
 *      the wrong map.
 */
static int os_mode_boot_init(void) {
    os_mode_apply();
    LOG_DBG("os_mode: boot default applied (pc_mode=%d)", os_mode_pc);
    return 0;
}
SYS_INIT(os_mode_boot_init, APPLICATION, 99);

/* ── Behavior callbacks ───────────────────────────────────────────────────── */

static int os_mode_binding_pressed(struct zmk_behavior_binding *binding,
                                   struct zmk_behavior_binding_event event) {
    /* Flip relative to the actual layer state so this is always a clean
     * two-state toggle. */
    bool new_pc = !zmk_keymap_layer_active(OS_MODE_PC_LAYER);
    os_mode_pc = new_pc ? 1 : 0;
    os_mode_apply();
    os_mode_save();

    LOG_DBG("os_mode: toggled -> %s", new_pc ? "PC" : "Mac");
    return ZMK_BEHAVIOR_OPAQUE;
}

static int os_mode_binding_released(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api os_mode_driver_api = {
    .binding_pressed = os_mode_binding_pressed,
    .binding_released = os_mode_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define OS_MODE_INST(n)                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &os_mode_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OS_MODE_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
