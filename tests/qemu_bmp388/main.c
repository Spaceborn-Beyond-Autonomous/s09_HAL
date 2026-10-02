/**
 * @file    main.c
 * @brief   Host-side (QEMU-buildable) test suite for the BMP388 driver.
 * @details Links bmp388.c against mock_bmp388_bus.c. Exit code is nonzero
 *          if any check fails (DRIVER_STANDARD.md Section 7 / PR checklist).
 */

#include "bmp388.h"
#include "mock_bmp388_bus.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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

static BMP388_Handle_t make_handle(void)
{
    BMP388_Handle_t h;
    memset(&h, 0, sizeof(h));
    h.bus_context = (void *)0x1;
    h.timeout_ms  = 50U;
    h.press_osr   = BMP388_OVERSAMPLING_X8;
    h.temp_osr    = BMP388_OVERSAMPLING_X1;
    return h;
}

static void test_init_happy_path(void)
{
    printf("test_init_happy_path:\n");
    mock_bmp388_bus_reset();
    BMP388_Handle_t h = make_handle();

    BMP388_Status_t st = BMP388_Init(&h);
    CHECK(st == BMP388_OK, "Init() returns BMP388_OK");
    CHECK(h.initialized == true, "handle marked initialized after Init()");
    CHECK(h.calib_loaded == true, "calibration block loaded during Init()");

    uint8_t id = 0U;
    st = BMP388_ReadDeviceID(&h, &id);
    CHECK(st == BMP388_OK && id == 0x50U, "CHIP_ID reads back 0x50");
}

static void test_init_null_handle(void)
{
    printf("test_init_null_handle:\n");
    BMP388_Status_t st = BMP388_Init(NULL);
    CHECK(st == BMP388_ERROR_INVALID_PARAM, "Init(NULL) rejected as invalid param");
}

static void test_init_wrong_device_id(void)
{
    printf("test_init_wrong_device_id:\n");
    mock_bmp388_bus_reset();
    mock_bmp388_bus_force_chip_id(0x00U); /* not the real 0x50 */
    BMP388_Handle_t h = make_handle();

    BMP388_Status_t st = BMP388_Init(&h);
    CHECK(st == BMP388_ERROR_WRONG_DEVICE_ID, "Init() detects CHIP_ID mismatch");
    CHECK(h.initialized == false, "handle stays uninitialized after failed Init()");
}

static void test_read_data_before_init(void)
{
    printf("test_read_data_before_init:\n");
    mock_bmp388_bus_reset();
    BMP388_Handle_t h = make_handle();
    BMP388_Data_t data;

    BMP388_Status_t st = BMP388_ReadData(&h, &data);
    CHECK(st == BMP388_ERROR_NOT_INITIALIZED, "ReadData() before Init() is rejected");
}

static void test_read_data_plausible_and_changing(void)
{
    printf("test_read_data_plausible_and_changing:\n");
    mock_bmp388_bus_reset();
    BMP388_Handle_t h = make_handle();
    BMP388_Init(&h);

    BMP388_Data_t a, b;
    BMP388_Status_t st1 = BMP388_ReadData(&h, &a);
    mock_bmp388_bus_tick(); /* let another "sample" become available */
    BMP388_Status_t st2 = BMP388_ReadData(&h, &b);

    CHECK(st1 == BMP388_OK && st2 == BMP388_OK, "two ReadData() calls (with a tick between) succeed");
    CHECK(a.pressure_raw != b.pressure_raw || a.temperature_raw != b.temperature_raw,
          "sample data is not a static/frozen value across reads");

    int pressure_plausible = (a.pressure_pa > 95000.0) && (a.pressure_pa < 108000.0);
    CHECK(pressure_plausible, "compensated pressure is plausibly near sea level (~101.3 kPa)");

    int temp_plausible = (a.temperature_c > 15.0) && (a.temperature_c < 35.0);
    CHECK(temp_plausible, "compensated temperature is plausibly room temperature");
}

static void test_data_ready_handshake(void)
{
    printf("test_data_ready_handshake:\n");
    mock_bmp388_bus_reset();
    BMP388_Handle_t h = make_handle();
    BMP388_Init(&h);

    BMP388_Data_t data;
    BMP388_Status_t st = BMP388_ReadData(&h, &data); /* consumes the ready flags */
    CHECK(st == BMP388_OK, "first ReadData() after Init() succeeds");

    mock_bmp388_bus_clear_data_ready();
    st = BMP388_ReadData(&h, &data);
    CHECK(st == BMP388_ERROR_NOT_READY, "ReadData() reports NOT_READY when DRDY bits are clear");

    mock_bmp388_bus_tick();
    st = BMP388_ReadData(&h, &data);
    CHECK(st == BMP388_OK, "ReadData() succeeds again once DRDY is set by a tick");
}

static void test_bus_fault_propagation(void)
{
    printf("test_bus_fault_propagation:\n");
    mock_bmp388_bus_reset();
    mock_bmp388_bus_set_fault_injection(1);
    BMP388_Handle_t h = make_handle();

    BMP388_Status_t st = BMP388_Init(&h);
    CHECK(st == BMP388_ERROR_BUS, "Init() propagates a bus fault as BMP388_ERROR_BUS");
    CHECK(h.fault_count > 0U, "fault_count increments on a bus error");

    mock_bmp388_bus_set_fault_injection(0);
}

static void test_deinit_sleeps_device(void)
{
    printf("test_deinit_sleeps_device:\n");
    mock_bmp388_bus_reset();
    BMP388_Handle_t h = make_handle();
    BMP388_Init(&h);

    BMP388_Status_t st = BMP388_DeInit(&h);
    CHECK(st == BMP388_OK, "DeInit() returns BMP388_OK");
    CHECK(h.initialized == false, "handle no longer marked initialized after DeInit()");

    BMP388_Data_t data;
    st = BMP388_ReadData(&h, &data);
    CHECK(st == BMP388_ERROR_NOT_INITIALIZED, "ReadData() rejected after DeInit()");
}

static void test_set_oversampling(void)
{
    printf("test_set_oversampling:\n");
    mock_bmp388_bus_reset();
    BMP388_Handle_t h = make_handle();
    BMP388_Init(&h);

    BMP388_Status_t st = BMP388_SetOversampling(&h, BMP388_OVERSAMPLING_X16, BMP388_OVERSAMPLING_X2);
    CHECK(st == BMP388_OK, "SetOversampling() returns BMP388_OK");
    CHECK(h.press_osr == BMP388_OVERSAMPLING_X16 && h.temp_osr == BMP388_OVERSAMPLING_X2,
          "handle reflects the newly requested oversampling");
}

int main(void)
{
    printf("=== BMP388 QEMU/host driver test suite ===\n\n");

    test_init_happy_path();
    test_init_null_handle();
    test_init_wrong_device_id();
    test_read_data_before_init();
    test_read_data_plausible_and_changing();
    test_data_ready_handshake();
    test_bus_fault_propagation();
    test_deinit_sleeps_device();
    test_set_oversampling();

    printf("\n=== %d/%d checks passed ===\n", g_tests_run - g_tests_failed, g_tests_run);
    return (g_tests_failed == 0) ? 0 : 1;
}
