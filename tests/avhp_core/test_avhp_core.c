#include "memory.h"
#include "interrupts.h"
#include "clock.h"
#include "boot.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int irq_count;
static void irq(uint32_t n,void*c){assert(n==2);(*(int*)c)++;}
static int init_count;
static void init(void){init_count++;}
int main(void){
 uint8_t flash[64],ram[64]; avhp_memory_region_t rs[2]={{0x1000,64,flash,AVHP_MEM_FLASH},{0x2000,64,ram,AVHP_MEM_SRAM}};avhp_memory_map_t m;assert(avhp_memory_init(&m,rs,2));avhp_memory_reset(&m,0);uint32_t x=0x12345678;assert(avhp_memory_write(&m,0x2004,&x,sizeof x));uint32_t y=0;assert(avhp_memory_read(&m,0x2004,&y,sizeof y)&&y==x);assert(!avhp_memory_read(&m,0x20ff,&y,2));
 avhp_irq_entry_t es[4];avhp_irq_controller_t ic;assert(avhp_irq_init(&ic,es,4));assert(avhp_irq_register(&ic,2,1,irq,&irq_count));assert(avhp_irq_enable(&ic,2,true));assert(avhp_irq_trigger(&ic,2)&&irq_count==1);
 avhp_clock_t c;assert(avhp_clock_init(&c,1000));assert(avhp_clock_now_us(&c) < 1000000ULL);
 avhp_boot_sequence_t b={init,init,init,init};assert(avhp_boot_run(&b)&&init_count==4);
 puts("PASS: AVHP core memory/interrupt/clock/boot tests");return 0;}
