/*
 * ============================ TEMPORARY — DELETE ME ============================
 *
 * Step 0 instrumentation for the dropped-keystroke investigation. NOT part of
 * the shipping firmware: remove this file, its zephyr_library_sources() line in
 * ../CMakeLists.txt, and CONFIG_ZMK_USB_LOGGING from ../keyboard.conf once the
 * scan duration has been measured.
 *
 * Why this exists
 * ---------------
 * Every matrix column strobe is an I2C register write to the MCP23017, and
 * kscan_gpio_matrix.c issues TWO of them per column (assert at line 226,
 * deassert at line 251) across 14 columns — 28 transactions per full scan pass.
 * ZMK's debouncer counts scans rather than measuring elapsed time
 * (kscan_gpio_matrix.c:247 passes the *configured* debounce-scan-period-ms as
 * its elapsed_ms argument), so the real debounce window is
 *
 *     scans_to_latch x actual_scan_duration
 *
 * which makes the true cost of one I2C write the number the whole fix depends
 * on. Measure it rather than trusting arithmetic.
 *
 * Why it is safe to run here
 * --------------------------
 * Registered at POST_KERNEL. The kscan drivers' own init only configures pins
 * and their work item — scanning does not begin until kscan_enable_callback(),
 * which ZMK invokes from zmk_physical_layouts_init(), registered at
 * src/physical_layouts.c:490 as SYS_INIT(..., APPLICATION, ...). Every
 * POST_KERNEL entry runs before every APPLICATION entry, so the bus is
 * guaranteed idle while we measure — this holds regardless of the relative
 * init priorities of the MCP230XX and KSCAN drivers.
 *
 * It toggles GPA0 (pin 0), which is unused: PRD SS2.4 lists GPA0/GPA1 as spare,
 * and the PCB netlist confirms U2 pad 21 as unconnected-(U2-GPA0-Pad21). The
 * key matrix is untouched.
 *
 * Bus integrity check
 * -------------------
 * 400 kHz is out of I2C fast-mode rise-time spec on this board (internal ~13k
 * pull-ups only, no external resistors — PRD SS2.3 and the BOM confirm none).
 * A marginal bus NACKs, and a NACK *aborts early*, so a broken bus can report a
 * deceptively FAST time. Timing alone therefore cannot tell "fast" from
 * "broken". So we also count failed writes and do a write/read-back pass: drive
 * the pin, read the port back, confirm the level survived the round trip.
 * Zero errors and zero mismatches is what makes a speed bump trustworthy.
 *
 * Why the result is reported on a repeating timer
 * -----------------------------------------------
 * The measurement has to happen at POST_KERNEL to get an idle bus, but the USB
 * CDC ACM console does not exist until USB has enumerated and the host has
 * opened the port — seconds later. A single log line emitted at POST_KERNEL
 * would be gone before anyone could read it. So we measure once, cache the
 * result, and re-log it every few seconds for a couple of minutes: plug in,
 * open a serial monitor, and the line will come round again.
 * ==============================================================================
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mcp23017), okay)

/* Spare expander pin — not wired to the matrix. */
#define PROBE_PIN 0

/* Each iteration is two port writes, matching one column's strobe/unstrobe. */
#define PROBE_ITERATIONS 500
#define PROBE_WRITES (PROBE_ITERATIONS * 2)

/* kscan_gpio_matrix.c does 2 writes per column across 14 col-gpios. */
#define WRITES_PER_SCAN 28

/* Round trips for the integrity pass (write, read back, compare). */
#define VERIFY_ITERATIONS 200

/* Cached so the repeating reporter can re-emit it once the USB console exists. */
static uint32_t measured_us_per_write;
static int measured_elapsed_ms;
static bool measurement_valid;
static uint32_t write_errors;
static uint32_t verify_errors;
static uint32_t verify_mismatches;

/* Re-log every REPORT_PERIOD until REPORT_COUNT is exhausted, so the line is
 * catchable whenever the host opens the port. Stops on its own rather than
 * spamming the console forever. */
#define REPORT_PERIOD K_SECONDS(3)
#define REPORT_COUNT 40

static void i2c_scan_timing_report(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(report_work, i2c_scan_timing_report);

static void i2c_scan_timing_report(struct k_work *work) {
    ARG_UNUSED(work);

    static int reports_left = REPORT_COUNT;

    if (measurement_valid) {
        const uint32_t scan_us = measured_us_per_write * WRITES_PER_SCAN;

        LOG_ERR("I2C TIMING: %u writes in %d ms => %u us/write; "
                "est full 14-col scan = %u us (%u.%02u ms)",
                (unsigned)PROBE_WRITES, measured_elapsed_ms, measured_us_per_write, scan_us,
                scan_us / 1000, (scan_us % 1000) / 10);

        /* Integrity verdict is what licenses an out-of-spec clock, so make it
         * unmissable and state the pass/fail rather than leaving raw counts to
         * be interpreted. */
        if (write_errors == 0 && verify_errors == 0 && verify_mismatches == 0) {
            LOG_ERR("I2C BUS OK: 0 write errors, %u/%u read-backs matched",
                    (unsigned)VERIFY_ITERATIONS * 2, (unsigned)VERIFY_ITERATIONS * 2);
        } else {
            LOG_ERR("I2C BUS SUSPECT: %u write errors, %u read errors, %u mismatches "
                    "-- bus is marginal, do NOT trust the timing above",
                    write_errors, verify_errors, verify_mismatches);
        }
    } else {
        LOG_ERR("I2C TIMING: measurement failed - see earlier error");
    }

    if (--reports_left > 0) {
        k_work_schedule(&report_work, REPORT_PERIOD);
    }
}

static int i2c_scan_timing_probe(void) {
    const struct device *mcp = DEVICE_DT_GET(DT_NODELABEL(mcp23017));

    /* Report regardless of outcome, so a failure is visible on the console
     * rather than looking like the shim was never compiled in. */
    k_work_schedule(&report_work, REPORT_PERIOD);

    if (!device_is_ready(mcp)) {
        /* Loud on purpose: a mis-ordered probe must not report a wrong number. */
        LOG_ERR("I2C TIMING: mcp23017 not ready - measurement skipped");
        return 0;
    }

    int err = gpio_pin_configure(mcp, PROBE_PIN, GPIO_OUTPUT_INACTIVE);
    if (err) {
        LOG_ERR("I2C TIMING: failed to configure probe pin: %d", err);
        return 0;
    }

    const int64_t start = k_uptime_get();

    for (int i = 0; i < PROBE_ITERATIONS; i++) {
        write_errors += (gpio_pin_set(mcp, PROBE_PIN, 1) != 0);
        write_errors += (gpio_pin_set(mcp, PROBE_PIN, 0) != 0);
    }

    const int64_t elapsed_ms = k_uptime_get() - start;

    if (elapsed_ms <= 0) {
        LOG_ERR("I2C TIMING: elapsed %d ms - too fast to resolve", (int)elapsed_ms);
        return 0;
    }

    measured_us_per_write = (uint32_t)((elapsed_ms * 1000) / PROBE_WRITES);
    measured_elapsed_ms = (int)elapsed_ms;

    /* Integrity pass, deliberately outside the timed loop so the read-backs do
     * not pollute the per-write figure. */
    for (int i = 0; i < VERIFY_ITERATIONS; i++) {
        for (int level = 1; level >= 0; level--) {
            int werr = gpio_pin_set(mcp, PROBE_PIN, level);
            if (werr) {
                write_errors++;
                continue;
            }

            int readback = gpio_pin_get(mcp, PROBE_PIN);
            if (readback < 0) {
                verify_errors++;
            } else if (readback != level) {
                verify_mismatches++;
            }
        }
    }

    measurement_valid = true;

    return 0;
}

SYS_INIT(i2c_scan_timing_probe, POST_KERNEL, 99);

#endif /* DT_NODE_HAS_STATUS(mcp23017, okay) */
