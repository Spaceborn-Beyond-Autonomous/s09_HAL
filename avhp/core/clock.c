#define _POSIX_C_SOURCE 200809L
#include "clock.h"
#include <time.h>
static uint64_t now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000ULL+(uint64_t)t.tv_nsec/1000ULL;}
bool avhp_clock_init(avhp_clock_t *clock,uint64_t tick_hz){if(!clock||tick_hz==0)return false;clock->start_us=now();clock->tick_hz=tick_hz;return true;}
uint64_t avhp_clock_now_us(const avhp_clock_t *clock){return clock?now()-clock->start_us:0;}
uint64_t avhp_clock_now_ms(const avhp_clock_t *clock){return avhp_clock_now_us(clock)/1000ULL;}
void avhp_clock_delay_ms(const avhp_clock_t *clock,uint32_t ms){uint64_t target=avhp_clock_now_us(clock)+(uint64_t)ms*1000ULL;while(avhp_clock_now_us(clock)<target){}}
