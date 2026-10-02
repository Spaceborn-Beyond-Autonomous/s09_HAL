#include "interrupts.h"
#include <string.h>
bool avhp_irq_init(avhp_irq_controller_t *ic,avhp_irq_entry_t *entries,size_t count){if(!ic||!entries||!count)return false;ic->entries=entries;ic->count=count;memset(entries,0,count*sizeof(*entries));return true;}
bool avhp_irq_register(avhp_irq_controller_t *ic,uint32_t irq,uint8_t priority,avhp_irq_handler_t handler,void *ctx){if(!ic||irq>=ic->count||!handler)return false;ic->entries[irq]=(avhp_irq_entry_t){handler,ctx,priority,false};return true;}
bool avhp_irq_enable(avhp_irq_controller_t *ic,uint32_t irq,bool enabled){if(!ic||irq>=ic->count||!ic->entries[irq].handler)return false;ic->entries[irq].enabled=enabled;return true;}
bool avhp_irq_trigger(avhp_irq_controller_t *ic,uint32_t irq){if(!ic||irq>=ic->count||!ic->entries[irq].enabled||!ic->entries[irq].handler)return false;ic->entries[irq].handler(irq,ic->entries[irq].ctx);return true;}
