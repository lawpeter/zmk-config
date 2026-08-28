# PRD §11 step 7: custom display modes + &disp_cycle behavior.
#
# CONFIG_NICE_VIEW_WIDGET_STATUS=n (set in keyboard.conf) prevents the
# nice_view shield from compiling custom_status_screen.c, which would
# produce a duplicate zmk_display_status_screen() symbol. We own that
# function in widgets/display_modes.c.
#
# The nice_view widget implementation files (util / bolt / status) are not
# compiled by the nice_view CMakeLists when NICE_VIEW_WIDGET_STATUS=n, so
# we add them here directly so display_modes.c can call
# zmk_widget_status_init() for mode 0.

# Always expose the shield root as an include path so that
# behavior_disp_cycle.c can find widgets/display_mode_state_changed.h.
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})

if(CONFIG_ZMK_DISPLAY)
    zephyr_library_include_directories(
        ${CMAKE_SOURCE_DIR}/boards/shields/nice_view          # "widgets/status.h" etc.
        ${CMAKE_SOURCE_DIR}/boards/shields/nice_view/widgets  # "util.h", "status.h" for local overrides
        ${CMAKE_SOURCE_DIR}/include                           # zmk core headers
    )

    zephyr_library_sources(
        # nice_view widget implementations (compiled here because
        # NICE_VIEW_WIDGET_STATUS=n skips them in the nice_view CMakeLists).
        # util.c and status.c are LOCAL OVERRIDES in widgets/ that apply the
        # 270° CW rotation and the adjusted bottom-canvas offset for the
        # connector-at-top portrait mounting.  bolt.c is unchanged upstream.
        widgets/util.c
        widgets/status.c
        ${CMAKE_SOURCE_DIR}/boards/shields/nice_view/widgets/bolt.c

        # Our multi-mode screen and mode-change event
        widgets/display_mode_state_changed.c
        widgets/display_modes.c

        # Step 8: live WPM counter widget (mode 1)
        widgets/wpm.c

        # Step 9: typing-test widget (mode 4) + state-change event
        widgets/typing_test_state_changed.c
        widgets/typing_test.c
    )
endif()

# disp_cycle behavior — guarded internally by DT_HAS_COMPAT_STATUS_OKAY
zephyr_library_sources(behaviors/behavior_disp_cycle.c)

# typing_test_toggle behavior — guarded internally by DT_HAS_COMPAT_STATUS_OKAY
# The ZMK_LISTENER/ZMK_SUBSCRIPTION for keycode counting is always compiled in
# but is a no-op while tt.active == false (i.e. at boot and when no test runs).
zephyr_library_sources(behaviors/behavior_typing_test.c)

# os_mode toggle behavior — guarded internally by DT_HAS_COMPAT_STATUS_OKAY.
# PRD v2 §2: Mac/PC modifier profile with a Windows boot default and
# persistence to the settings/NVS backend across reboot.
zephyr_library_sources(behaviors/behavior_os_mode.c)

# shift_mask behavior — guarded internally by DT_HAS_COMPAT_STATUS_OKAY.
# Wraps zmk_hid_masked_modifiers_set/clear for use inside macros (uppercase
# Greek sym_layer triggers need Shift suppressed for their own duration,
# independent of how long the physical Shift/letter keys are held).
zephyr_library_sources(behaviors/behavior_shift_mask.c)

# shift_branch behavior — guarded internally by DT_HAS_COMPAT_STATUS_OKAY.
# mod-morph-alike branching (pick a binding based on current Shift state)
# WITHOUT mod-morph's own built-in masking, which conflicts with &shift_mask
# used inside the invoked macros — see behavior_shift_branch.c for why.
zephyr_library_sources(behaviors/behavior_shift_branch.c)
