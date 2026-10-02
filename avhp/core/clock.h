#ifndef ANSA_AVHP_CLOCK_H
#define ANSA_AVHP_CLOCK_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint64_t start_us; uint64_t tick_hz; } avhp_clock_t;
bool avhp_clock_init(avhp_clock_t *clock,uint64_t tick_hz);
uint64_t avhp_clock_now_us(const avhp_clock_t *clock);
uint64_t avhp_clock_now_ms(const avhp_clock_t *clock);
void avhp_clock_delay_ms(const avhp_clock_t *clock,uint32_t ms);
#endif
