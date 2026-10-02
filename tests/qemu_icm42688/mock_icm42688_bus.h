/**
 * @file  mock_icm42688_bus.h
 * @brief Test-only control surface for mock_icm42688_bus.c. Never included
 *        by icm42688.c itself — only by the test's main.c.
 */

#ifndef MOCK_ICM42688_BUS_H
#define MOCK_ICM42688_BUS_H

#include <stdint.h>

void mock_icm42688_bus_reset(void);
void mock_icm42688_bus_set_fault_injection(int enabled);
void mock_icm42688_bus_force_who_am_i(uint8_t value);
void mock_icm42688_bus_tick(void);

#endif /* MOCK_ICM42688_BUS_H */
