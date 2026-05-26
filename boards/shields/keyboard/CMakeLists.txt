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
        ${CMAKE_SOURCE_DIR}/boards/shields/nice_view   # "widgets/status.h" etc.
        ${CMAKE_SOURCE_DIR}/include                    # zmk core headers
    )

    zephyr_library_sources(
        # nice_view widget implementations (compiled here because
        # NICE_VIEW_WIDGET_STATUS=n skips them in the nice_view CMakeLists)
        ${CMAKE_SOURCE_DIR}/boards/shields/nice_view/widgets/util.c
        ${CMAKE_SOURCE_DIR}/boards/shields/nice_view/widgets/bolt.c
        ${CMAKE_SOURCE_DIR}/boards/shields/nice_view/widgets/status.c

        # Our multi-mode screen and mode-change event
        widgets/display_mode_state_changed.c
        widgets/display_modes.c

        # Step 8: live WPM counter widget (mode 1)
        widgets/wpm.c
    )
endif()

# disp_cycle behavior — guarded internally by DT_HAS_COMPAT_STATUS_OKAY
zephyr_library_sources(behaviors/behavior_disp_cycle.c)
