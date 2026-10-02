/**
 * @file main.c
 * @brief Standalone QEMU unit test suite entry point for the Honeywell HMC5883L driver.
 * @details Compiles on host to execute comprehensive register-decode validations,
 *          lifecycle FSM transition audits, self-test verification, and fault-injection audits
 *          against the in-memory mock bus.
 *
 * @note Prints explicit [PASS] or [FAIL] metrics for CI/QEMU automated parsing.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hmc5883l.h"
#include "hmc5883l_registers.h"

/*============================================================================*
 *                   EXTERN SIMULATION CONTROL PROTOYPES                      *
 *============================================================================*/

extern void mock_hmc5883l_reset_bus(void);
extern void mock_hmc5883l_set_reg(uint8_t reg_addr, uint8_t value);
extern uint8_t mock_hmc5883l_get_reg(uint8_t reg_addr);
extern void mock_hmc5883l_inject_bus_error(hal_status_t status);
extern void mock_hmc5883l_set_static_field(double x_gauss, double y_gauss, double z_gauss);
extern void mock_hmc5883l_enable_dynamic_sim(bool enable);
extern hal_status_t mock_hmc5883l_bus_read(void *bus, uint8_t dev_addr, uint8_t reg_addr, uint8_t *dest, size_t len, uint32_t timeout);
extern hal_status_t mock_hmc5883l_bus_write(void *bus, uint8_t dev_addr, uint8_t reg_addr, const uint8_t *src, size_t len, uint32_t timeout);

/*============================================================================*
 *                     MOCKED PLATFORM TIME & DELAY STUBS                     *
 *============================================================================*/

static uint32_t virtual_clock_us = 0;

uint32_t hal_timer_now_us(void *ctx) {
    (void)ctx;
    virtual_clock_us += 1000; /* Increment by 1ms per query to mock clock ticks */
    return virtual_clock_us;
}

void hal_delay_ms(uint32_t ms) {
    virtual_clock_us += (ms * 1000);
}

/*============================================================================*
 *                            TEST CASE FUNCTIONS                             *
 *============================================================================*/

static void test_initialization_happy_path(void) {
    printf("  [RUN] test_initialization_happy_path... ");

    mock_hmc5883l_reset_bus();

    HMC5883L_Handle_t handle = {0}; /* zero-init: avoid reading fault_count/last_error as stack garbage */
    handle.bus_handle  = NULL;
    handle.dev_address = HMC5883L_I2C_ADDRESS;
    handle.timeout_ms  = 100;
    handle.averaging   = HMC5883L_AVG_8;
    handle.odr         = HMC5883L_ODR_15_HZ;
    handle.bias        = HMC5883L_BIAS_NORMAL;
    handle.gain        = HMC5883L_GAIN_1_3_GA;
    handle.mode        = HMC5883L_MODE_CONTINUOUS;
    handle.bus_read    = mock_hmc5883l_bus_read;
    handle.bus_write   = mock_hmc5883l_bus_write;
    handle.state       = DRV_STATE_UNINIT;

    hal_status_t status = HMC5883L_Init(&handle);

    assert(status == HAL_OK);
    assert(handle.state == DRV_STATE_ACTIVE);
    assert(handle.last_error == HAL_OK);
    assert(handle.fault_count == 0);

    /* Verify written registers in mock bus */
    uint8_t cra_val = mock_hmc5883l_get_reg(HMC5883L_REG_CRA);
    uint8_t crb_val = mock_hmc5883l_get_reg(HMC5883L_REG_CRB);
    uint8_t mode_val = mock_hmc5883l_get_reg(HMC5883L_REG_MODE);

    /* Average=8, ODR=15Hz, Bias=Normal -> (0x03 << 5) | (0x04 << 2) | (0x00) = 0x60 | 0x10 | 0x00 = 0x70 */
    assert(cra_val == 0x70);
    /* Gain=1.3Ga (1) shifted by 5 -> 0x20 */
    assert(crb_val == 0x20);
    /* Continuous mode (0x00) */
    assert(mode_val == 0x00);

    printf("[PASS]\n");
}

static void test_initialization_invalid_parameters(void) {
    printf("  [RUN] test_initialization_invalid_parameters... ");

    hal_status_t status = HMC5883L_Init(NULL);
    assert(status == HAL_ERR_INVALID_PARAM);

    HMC5883L_Handle_t handle = {0}; /* zero-init: avoid reading fault_count/last_error as stack garbage */
    handle.bus_read  = NULL;
    handle.bus_write = mock_hmc5883l_bus_write;
    status = HMC5883L_Init(&handle);
    assert(status == HAL_ERR_INVALID_PARAM);
    assert(handle.state == DRV_STATE_FAULT);

    printf("[PASS]\n");
}

static void test_initialization_chip_id_mismatch(void) {
    printf("  [RUN] test_initialization_chip_id_mismatch... ");

    mock_hmc5883l_reset_bus();
    /* Inject a corrupt identifier register byte */
    mock_hmc5883l_set_reg(HMC5883L_REG_IDA, 0xFF);

    HMC5883L_Handle_t handle = {0}; /* zero-init: avoid reading fault_count/last_error as stack garbage */
    handle.bus_handle  = NULL;
    handle.dev_address = HMC5883L_I2C_ADDRESS;
    handle.timeout_ms  = 100;
    handle.averaging   = HMC5883L_AVG_1;
    handle.odr         = HMC5883L_ODR_15_HZ;
    handle.bias        = HMC5883L_BIAS_NORMAL;
    handle.gain        = HMC5883L_GAIN_1_3_GA;
    handle.mode        = HMC5883L_MODE_CONTINUOUS;
    handle.bus_read    = mock_hmc5883l_bus_read;
    handle.bus_write   = mock_hmc5883l_bus_write;
    handle.state       = DRV_STATE_UNINIT;

    hal_status_t status = HMC5883L_Init(&handle);

    assert(status == HAL_ERR_HW_FAULT);
    assert(handle.state == DRV_STATE_FAULT);
    assert(handle.last_error == HAL_ERR_HW_FAULT);

    printf("[PASS]\n");
}

static void test_data_decode_revolving_fields(void) {
    printf("  [RUN] test_data_decode_revolving_fields... ");

    mock_hmc5883l_reset_bus();
    mock_hmc5883l_enable_dynamic_sim(true);

    HMC5883L_Handle_t handle = {0}; /* zero-init: avoid reading fault_count/last_error as stack garbage */
    handle.bus_handle  = NULL;
    handle.dev_address = HMC5883L_I2C_ADDRESS;
    handle.timeout_ms  = 100;
    handle.bus_read    = mock_hmc5883l_bus_read;
    handle.bus_write   = mock_hmc5883l_bus_write;
    handle.state       = DRV_STATE_READY;

    HMC5883L_RawData_t data1, data2;

    hal_status_t status = HMC5883L_ReadData(&handle, &data1);
    assert(status == HAL_OK);

    status = HMC5883L_ReadData(&handle, &data2);
    assert(status == HAL_OK);

    /* Verify data stream changes across reads (dynamic simulation verified) */
    assert(data1.x_raw != data2.x_raw || data1.y_raw != data2.y_raw);
    assert(data1.timestamp_us < data2.timestamp_us);

    printf("[PASS]\n");
}

static void test_self_test_verification_happy_path(void) {
    printf("  [RUN] test_self_test_verification_happy_path... ");

    mock_hmc5883l_reset_bus();

    HMC5883L_Handle_t handle = {0}; /* zero-init: avoid reading fault_count/last_error as stack garbage */
    handle.bus_handle  = NULL;
    handle.dev_address = HMC5883L_I2C_ADDRESS;
    handle.timeout_ms  = 100;
    handle.averaging   = HMC5883L_AVG_1;
    handle.odr         = HMC5883L_ODR_15_HZ;
    handle.bias        = HMC5883L_BIAS_NORMAL;
    handle.gain        = HMC5883L_GAIN_1_3_GA;
    handle.mode        = HMC5883L_MODE_SINGLE;
    handle.bus_read    = mock_hmc5883l_bus_read;
    handle.bus_write   = mock_hmc5883l_bus_write;
    handle.state       = DRV_STATE_READY;

    bool pass_out = false;
    hal_status_t status = HMC5883L_RunSelfTest(&handle, &pass_out);

    assert(status == HAL_OK);
    assert(pass_out == true);

    /* Enforce that original running states are completely restored on test exits */
    assert(handle.averaging == HMC5883L_AVG_1);
    assert(handle.bias == HMC5883L_BIAS_NORMAL);
    assert(handle.gain == HMC5883L_GAIN_1_3_GA);
    assert(handle.mode == HMC5883L_MODE_SINGLE);

    printf("[PASS]\n");
}

static void test_self_test_verification_failure_path(void) {
    printf("  [RUN] test_self_test_verification_failure_path... ");

    mock_hmc5883l_reset_bus();
    /* Force simulated measurements into bad ranges */
    mock_hmc5883l_set_static_field(0.01, 0.01, 0.01);

    HMC5883L_Handle_t handle = {0}; /* zero-init: avoid reading fault_count/last_error as stack garbage */
    handle.bus_handle  = NULL;
    handle.dev_address = HMC5883L_I2C_ADDRESS;
    handle.timeout_ms  = 100;
    handle.averaging   = HMC5883L_AVG_1;
    handle.odr         = HMC5883L_ODR_15_HZ;
    handle.bias        = HMC5883L_BIAS_NORMAL;
    handle.gain        = HMC5883L_GAIN_1_3_GA;
    handle.mode        = HMC5883L_MODE_SINGLE;
    handle.bus_read    = mock_hmc5883l_bus_read;
    handle.bus_write   = mock_hmc5883l_bus_write;
    handle.state       = DRV_STATE_READY;

    bool pass_out = true;
    hal_status_t status = HMC5883L_RunSelfTest(&handle, &pass_out);

    assert(status == HAL_OK);
    assert(pass_out == false); /* Out of datasheet specifications limits */

    printf("[PASS]\n");
}

static void test_bus_error_fault_propagation(void) {
    printf("  [RUN] test_bus_error_fault_propagation... ");

    mock_hmc5883l_reset_bus();
    /* Inject bus NACK */
    mock_hmc5883l_inject_bus_error(HAL_ERR_NACK);

    HMC5883L_Handle_t handle = {0}; /* zero-init: avoid reading fault_count/last_error as stack garbage */
    handle.bus_handle  = NULL;
    handle.dev_address = HMC5883L_I2C_ADDRESS;
    handle.timeout_ms  = 100;
    handle.averaging   = HMC5883L_AVG_1;
    handle.odr         = HMC5883L_ODR_15_HZ;
    handle.bias        = HMC5883L_BIAS_NORMAL;
    handle.gain        = HMC5883L_GAIN_1_3_GA;
    handle.mode        = HMC5883L_MODE_SINGLE;
    handle.bus_read    = mock_hmc5883l_bus_read;
    handle.bus_write   = mock_hmc5883l_bus_write;
    handle.state       = DRV_STATE_READY;
    handle.fault_count = 0;

    HMC5883L_RawData_t data;
    hal_status_t status = HMC5883L_ReadData(&handle, &data);

    assert(status == HAL_ERR_NACK);
    assert(handle.state == DRV_STATE_FAULT);
    assert(handle.fault_count == 1);
    assert(handle.last_error == HAL_ERR_NACK);

    printf("[PASS]\n");
}

/*============================================================================*
 *                                MAIN RUNNER                                 *
 *============================================================================*/

int main(void) {
    printf("=====================================================\n");
    printf("STARTING HOST-SIDE HMC5883L DRIVER UNIT TEST SUITE\n");
    printf("=====================================================\n");

    test_initialization_happy_path();
    test_initialization_invalid_parameters();
    test_initialization_chip_id_mismatch();
    test_data_decode_revolving_fields();
    test_self_test_verification_happy_path();
    test_self_test_verification_failure_path();
    test_bus_error_fault_propagation();

    printf("=====================================================\n");
    printf("ALL TESTS COMPLETED SUCCESSFULLY: [PASS]\n");
    printf("=====================================================\n");

    return 0;
}
