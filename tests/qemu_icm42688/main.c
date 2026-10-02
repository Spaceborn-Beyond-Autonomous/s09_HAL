/**
 * @file    main.c
 * @brief   Host-side (QEMU-buildable) test suite for the ICM-42688-P driver.
 * @details Links icm42688.c against mock_icm42688_bus.c — no physical
 *          hardware and no cross toolchain required to prove the driver
 *          logic is correct. Exit code is nonzero if any check fails, so
 *          this plugs straight into the repo's CI (see DRIVER_STANDARD.md
 *          Section 7 / the PR checklist).
 */

#include "icm42688.h"
#include "mock_icm42688_bus.h"
#include <stdio.h>
#include <string.h>

static int g_tests_run    = 0;
static int g_tests_failed = 0;

#define CHECK(cond, msg)                                                   \
    do {                                                                   \
        g_tests_run++;                                                     \
        if (cond) {                                                        \
            printf("  [PASS] %s\n", msg);                                  \
        } else {                                                           \
            printf("  [FAIL] %s (%s:%d)\n", msg, __FILE__, __LINE__);      \
            g_tests_failed++;                                              \
        }                                                                  \
    } while (0)

static ICM42688_Handle_t make_handle(void)
{
    ICM42688_Handle_t h;
    memset(&h, 0, sizeof(h));
    h.bus_context = (void *)0x1; /* mock ignores the value, just wants non-NULL */
    h.timeout_ms  = 50U;
    h.accel_fs    = ICM42688_ACCEL_FS_16G;
    h.gyro_fs     = ICM42688_GYRO_FS_2000DPS;
    return h;
}

static void test_init_happy_path(void)
{
    printf("test_init_happy_path:\n");
    mock_icm42688_bus_reset();
    ICM42688_Handle_t h = make_handle();

    ICM42688_Status_t st = ICM42688_Init(&h);
    CHECK(st == ICM42688_OK, "Init() returns ICM42688_OK");
    CHECK(h.initialized == true, "handle marked initialized after Init()");

    uint8_t who = 0U;
    st = ICM42688_ReadDeviceID(&h, &who);
    CHECK(st == ICM42688_OK && who == 0x47U, "WHO_AM_I reads back 0x47");
}

static void test_init_null_handle(void)
{
    printf("test_init_null_handle:\n");
    ICM42688_Status_t st = ICM42688_Init(NULL);
    CHECK(st == ICM42688_ERROR_INVALID_PARAM, "Init(NULL) rejected as invalid param");
}

static void test_init_wrong_device_id(void)
{
    printf("test_init_wrong_device_id:\n");
    mock_icm42688_bus_reset();
    mock_icm42688_bus_force_who_am_i(0x00U); /* not the real 0x47 */
    ICM42688_Handle_t h = make_handle();

    ICM42688_Status_t st = ICM42688_Init(&h);
    CHECK(st == ICM42688_ERROR_WRONG_DEVICE_ID, "Init() detects WHO_AM_I mismatch");
    CHECK(h.initialized == false, "handle stays uninitialized after failed Init()");
}

static void test_read_data_before_init(void)
{
    printf("test_read_data_before_init:\n");
    mock_icm42688_bus_reset();
    ICM42688_Handle_t h = make_handle();
    ICM42688_RawData_t data;

    ICM42688_Status_t st = ICM42688_ReadData(&h, &data);
    CHECK(st == ICM42688_ERROR_NOT_INITIALIZED, "ReadData() before Init() is rejected");
}

static void test_read_data_changes_across_reads(void)
{
    printf("test_read_data_changes_across_reads:\n");
    mock_icm42688_bus_reset();
    ICM42688_Handle_t h = make_handle();
    ICM42688_Init(&h);

    ICM42688_RawData_t a, b;
    ICM42688_Status_t st1 = ICM42688_ReadData(&h, &a);
    ICM42688_Status_t st2 = ICM42688_ReadData(&h, &b);

    CHECK(st1 == ICM42688_OK && st2 == ICM42688_OK, "two consecutive ReadData() calls succeed");
    int identical = (a.accel_x_raw == b.accel_x_raw) && (a.accel_y_raw == b.accel_y_raw) &&
                     (a.gyro_x_raw == b.gyro_x_raw) && (a.temp_raw == b.temp_raw);
    CHECK(!identical, "sample data is not a static/frozen value across reads");

    /* Sanity: accel Z should sit near +1 g (2048 LSB @ +-16 g) at rest. */
    int z_in_range = (a.accel_z_raw > 1800) && (a.accel_z_raw < 2300);
    CHECK(z_in_range, "accel Z sample is plausibly near +1 g at rest");
}

static void test_bus_fault_propagation(void)
{
    printf("test_bus_fault_propagation:\n");
    mock_icm42688_bus_reset();
    mock_icm42688_bus_set_fault_injection(1);
    ICM42688_Handle_t h = make_handle();

    ICM42688_Status_t st = ICM42688_Init(&h);
    CHECK(st == ICM42688_ERROR_BUS, "Init() propagates a bus fault as ICM42688_ERROR_BUS");
    CHECK(h.fault_count > 0U, "fault_count increments on a bus error");

    mock_icm42688_bus_set_fault_injection(0);
}

static void test_deinit_sleeps_device(void)
{
    printf("test_deinit_sleeps_device:\n");
    mock_icm42688_bus_reset();
    ICM42688_Handle_t h = make_handle();
    ICM42688_Init(&h);

    ICM42688_Status_t st = ICM42688_DeInit(&h);
    CHECK(st == ICM42688_OK, "DeInit() returns ICM42688_OK");
    CHECK(h.initialized == false, "handle no longer marked initialized after DeInit()");

    ICM42688_RawData_t data;
    st = ICM42688_ReadData(&h, &data);
    CHECK(st == ICM42688_ERROR_NOT_INITIALIZED, "ReadData() rejected after DeInit()");
}

static void test_fifo_lifecycle(void)
{
    printf("test_fifo_lifecycle:\n");
    mock_icm42688_bus_reset();
    ICM42688_Handle_t h = make_handle();
    ICM42688_Init(&h);

    ICM42688_Status_t st = ICM42688_FifoEnable(&h);
    CHECK(st == ICM42688_OK, "FifoEnable() succeeds");
    CHECK(h.fifo_enabled == true, "handle reflects FIFO enabled");

    for (int i = 0; i < 5; i++) {
        mock_icm42688_bus_tick();
    }

    uint16_t count_before = 0U;
    st = ICM42688_FifoGetCount(&h, &count_before);
    CHECK(st == ICM42688_OK && count_before > 0U, "FIFO count grows after ticks");

    ICM42688_RawData_t packets[16];
    uint16_t packets_read = 0U;
    st = ICM42688_FifoRead(&h, packets, 16U, &packets_read);
    CHECK(st == ICM42688_OK, "FifoRead() succeeds");
    CHECK(packets_read > 0U, "FifoRead() drains at least one packet");

    uint16_t count_after = 0U;
    ICM42688_FifoGetCount(&h, &count_after);
    CHECK(count_after < count_before, "FIFO count decreases after draining");

    st = ICM42688_FifoDisable(&h);
    CHECK(st == ICM42688_OK && h.fifo_enabled == false, "FifoDisable() clears fifo_enabled");
}

static void test_set_full_scale_ranges(void)
{
    printf("test_set_full_scale_ranges:\n");
    mock_icm42688_bus_reset();
    ICM42688_Handle_t h = make_handle();
    ICM42688_Init(&h);

    ICM42688_Status_t st = ICM42688_SetAccelFS(&h, ICM42688_ACCEL_FS_2G);
    CHECK(st == ICM42688_OK && h.accel_fs == ICM42688_ACCEL_FS_2G,
          "SetAccelFS() updates the handle's active range");

    st = ICM42688_SetGyroFS(&h, ICM42688_GYRO_FS_250DPS);
    CHECK(st == ICM42688_OK && h.gyro_fs == ICM42688_GYRO_FS_250DPS,
          "SetGyroFS() updates the handle's active range");
}

int main(void)
{
    printf("=== ICM-42688-P QEMU/host driver test suite ===\n\n");

    test_init_happy_path();
    test_init_null_handle();
    test_init_wrong_device_id();
    test_read_data_before_init();
    test_read_data_changes_across_reads();
    test_bus_fault_propagation();
    test_deinit_sleeps_device();
    test_fifo_lifecycle();
    test_set_full_scale_ranges();

    printf("\n=== %d/%d checks passed ===\n", g_tests_run - g_tests_failed, g_tests_run);
    return (g_tests_failed == 0) ? 0 : 1;
}
