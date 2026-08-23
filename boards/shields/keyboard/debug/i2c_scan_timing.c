/*
 * ============================ TEMPORARY — DELETE ME ============================
 *
 * Step 0/1 instrumentation for the dropped-keystroke investigation. NOT part of
 * the shipping firmware: remove this file, its zephyr_library_sources() line in
 * ../CMakeLists.txt, the USB console block at the bottom of ../keyboard.overlay
 * and CONFIG_ZMK_USB_LOGGING in ../keyboard.conf once the numbers are recorded.
 *
 * What it measures and why
 * ------------------------
 * Every matrix column strobe is an I2C register write to the MCP23017, and
 * kscan_gpio_matrix.c issues TWO per column (assert at line 226, deassert at
 * line 251) across 14 columns — 28 transactions per full scan. ZMK's debouncer
 * counts scans rather than measuring elapsed time (kscan_gpio_matrix.c:247
 * passes the *configured* debounce-scan-period-ms as its elapsed_ms argument),
 * so the real debounce window is
 *
 *     scans_to_latch x actual_scan_duration
 *
 * which makes the cost of one I2C write the number the whole fix depends on.
 *
 * Why it sweeps the bus speed at runtime
 * --------------------------------------
 * The first 400 kHz attempt (set via the devicetree clock-frequency) failed
 * outright, and booting at a speed the bus cannot sustain risks taking the
 * whole matrix down with it — every column lives on this expander. So the DT
 * now stays at the known-good 100 kHz and this shim changes the frequency at
 * runtime instead, measuring each speed in turn and restoring 100 kHz when
 * done. i2c_nrfx_twim_configure() maps I2C_SPEED_STANDARD/FAST onto
 * NRF_TWIM_FREQ_100K/400K and is safe to call after init, which does no bus
 * traffic of its own. The keyboard therefore always boots working, whatever
 * this finds. (250 kHz is reachable only from the devicetree — the Zephyr I2C
 * API has no constant for it — so it is not in the sweep.)
 *
 * Timing alone is not enough
 * --------------------------
 * A NACK aborts a transfer EARLY, so a marginal bus reports a deceptively FAST
 * time; "fast" and "broken" are indistinguishable from duration. Each speed is
 * therefore also checked for write errors and with a write/read-back pass.
 * A speed is only trustworthy at "BUS OK".
 *
 * It toggles GPA0 (pin 0), which is unused: PRD SS2.4 lists GPA0/GPA1 as spare
 * and the PCB netlist confirms U2 pad 21 as unconnected-(U2-GPA0-Pad21), so the
 * key matrix is untouched.
 *
 * Everything is reported on a repeating timer because the measurement must run
 * at POST_KERNEL to get an idle bus, but the USB CDC ACM console does not exist
 * until USB has enumerated and the host has opened the port — seconds later.
 * A line logged at POST_KERNEL is gone before anyone can read it. (The previous
 * revision got this right for results but not for errors, which is why a failure
 * showed up as an unhelpful "see earlier error".)
 * ==============================================================================
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mcp23017), okay)

#define MCP_NODE DT_NODELABEL(mcp23017)

/* Spare expander pin — not wired to the matrix. */
#define PROBE_PIN 0

/* Each iteration is two port writes, matching one column's strobe/unstrobe. */
#define PROBE_ITERATIONS 300
#define PROBE_WRITES (PROBE_ITERATIONS * 2)

/* Round trips for the integrity pass (write, read back, compare). */
#define VERIFY_ITERATIONS 150
#define VERIFY_ROUNDTRIPS (VERIFY_ITERATIONS * 2)

/* kscan_gpio_matrix.c does 2 writes per column across 14 col-gpios. */
#define WRITES_PER_SCAN 28

struct speed_result {
    const char *label;
    uint32_t speed;
    bool attempted;
    int cfg_err;
    int elapsed_ms;
    uint32_t us_per_write;
    uint32_t write_errors;
    uint32_t verify_errors;
    uint32_t verify_mismatches;
};

static struct speed_result results[] = {
    {.label = "100kHz", .speed = I2C_SPEED_STANDARD},
    {.label = "400kHz", .speed = I2C_SPEED_FAST},
};

/* Set when we could not get far enough to measure anything at all. Cached as a
 * string rather than logged on the spot, so it survives to the reporter. */
static const char *fatal_reason;
static int fatal_err;

#define REPORT_PERIOD K_SECONDS(3)
#define REPORT_COUNT 40

static void i2c_scan_timing_report(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(report_work, i2c_scan_timing_report);

static void i2c_scan_timing_report(struct k_work *work) {
    ARG_UNUSED(work);

    static int reports_left = REPORT_COUNT;

    if (fatal_reason) {
        LOG_ERR("I2C TIMING: FAILED - %s (err %d)", fatal_reason, fatal_err);
    } else {
        for (int i = 0; i < ARRAY_SIZE(results); i++) {
            const struct speed_result *r = &results[i];

            if (!r->attempted) {
                LOG_ERR("I2C %s: not attempted", r->label);
            } else if (r->cfg_err) {
                LOG_ERR("I2C %s: i2c_configure rejected it (err %d)", r->label, r->cfg_err);
            } else if (r->elapsed_ms <= 0) {
                LOG_ERR("I2C %s: %d ms for %u writes - implausibly fast, bus almost "
                        "certainly NACKing (%u write errors)",
                        r->label, r->elapsed_ms, (unsigned)PROBE_WRITES, r->write_errors);
            } else {
                const uint32_t scan_us = r->us_per_write * WRITES_PER_SCAN;
                const bool ok = (r->write_errors == 0 && r->verify_errors == 0 &&
                                 r->verify_mismatches == 0);

                LOG_ERR("I2C %s: %u us/write -> 14-col scan %u us (%u.%02u ms) | %s "
                        "(%u write err, %u read err, %u mismatch of %u)",
                        r->label, r->us_per_write, scan_us, scan_us / 1000,
                        (scan_us % 1000) / 10, ok ? "BUS OK" : "BUS SUSPECT", r->write_errors,
                        r->verify_errors, r->verify_mismatches, (unsigned)VERIFY_ROUNDTRIPS);
            }
        }
    }

    if (--reports_left > 0) {
        k_work_schedule(&report_work, REPORT_PERIOD);
    }
}

static void measure_at_speed(struct speed_result *r, const struct device *bus,
                             const struct device *mcp) {
    r->attempted = true;

    r->cfg_err = i2c_configure(bus, I2C_SPEED_SET(r->speed) | I2C_MODE_CONTROLLER);
    if (r->cfg_err) {
        return;
    }

    const int64_t start = k_uptime_get();

    for (int i = 0; i < PROBE_ITERATIONS; i++) {
        r->write_errors += (gpio_pin_set(mcp, PROBE_PIN, 1) != 0);
        r->write_errors += (gpio_pin_set(mcp, PROBE_PIN, 0) != 0);
    }

    r->elapsed_ms = (int)(k_uptime_get() - start);
    if (r->elapsed_ms > 0) {
        r->us_per_write = (uint32_t)(((int64_t)r->elapsed_ms * 1000) / PROBE_WRITES);
    }

    /* Integrity pass, deliberately outside the timed loop so the read-backs do
     * not pollute the per-write figure. */
    for (int i = 0; i < VERIFY_ITERATIONS; i++) {
        for (int level = 1; level >= 0; level--) {
            if (gpio_pin_set(mcp, PROBE_PIN, level) != 0) {
                r->write_errors++;
                continue;
            }

            int readback = gpio_pin_get(mcp, PROBE_PIN);
            if (readback < 0) {
                r->verify_errors++;
            } else if (readback != level) {
                r->verify_mismatches++;
            }
        }
    }
}

static int i2c_scan_timing_probe(void) {
    const struct device *mcp = DEVICE_DT_GET(MCP_NODE);
    const struct device *bus = DEVICE_DT_GET(DT_BUS(MCP_NODE));

    /* Schedule unconditionally: a failure must reach the console too. */
    k_work_schedule(&report_work, REPORT_PERIOD);

    if (!device_is_ready(bus)) {
        fatal_reason = "i2c0 bus not ready";
        return 0;
    }

    if (!device_is_ready(mcp)) {
        /* The expander driver failed to initialise — on this board that also
         * means every matrix column is dead, so say so plainly. */
        fatal_reason = "mcp23017 not ready (expander init failed; matrix will be dead)";
        return 0;
    }

    int err = gpio_pin_configure(mcp, PROBE_PIN, GPIO_OUTPUT_INACTIVE);
    if (err) {
        fatal_reason = "could not configure probe pin GPA0";
        fatal_err = err;
        return 0;
    }

    for (int i = 0; i < ARRAY_SIZE(results); i++) {
        measure_at_speed(&results[i], bus, mcp);
    }

    /* Leave the bus at the devicetree speed so the matrix scans at the
     * known-good rate regardless of what the sweep found. */
    i2c_configure(bus, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);

    return 0;
}

SYS_INIT(i2c_scan_timing_probe, POST_KERNEL, 99);

#endif /* DT_NODE_HAS_STATUS(mcp23017, okay) */
