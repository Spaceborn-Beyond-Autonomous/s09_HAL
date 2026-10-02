/**
 * @file  mock_bmp388_bus.h
 * @brief Test-only control surface for mock_bmp388_bus.c. Never included by
 *        bmp388.c itself — only by the test's main.c.
 */

#ifndef MOCK_BMP388_BUS_H
#define MOCK_BMP388_BUS_H

#include <stdint.h>

void mock_bmp388_bus_reset(void);
void mock_bmp388_bus_set_fault_injection(int enabled);
void mock_bmp388_bus_force_chip_id(uint8_t value);
void mock_bmp388_bus_tick(void);
void mock_bmp388_bus_clear_data_ready(void);

#endif /* MOCK_BMP388_BUS_H */
