#ifndef ANSA_AVHP_INTERRUPTS_H
#define ANSA_AVHP_INTERRUPTS_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef void (*avhp_irq_handler_t)(uint32_t irq, void *ctx);
typedef struct { avhp_irq_handler_t handler; void *ctx; uint8_t priority; bool enabled; } avhp_irq_entry_t;
typedef struct { avhp_irq_entry_t *entries; size_t count; } avhp_irq_controller_t;
bool avhp_irq_init(avhp_irq_controller_t *ic, avhp_irq_entry_t *entries, size_t count);
bool avhp_irq_register(avhp_irq_controller_t *ic,uint32_t irq,uint8_t priority,avhp_irq_handler_t handler,void *ctx);
bool avhp_irq_enable(avhp_irq_controller_t *ic,uint32_t irq,bool enabled);
bool avhp_irq_trigger(avhp_irq_controller_t *ic,uint32_t irq);
#endif
