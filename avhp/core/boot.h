#ifndef ANSA_AVHP_BOOT_H
#define ANSA_AVHP_BOOT_H
#include <stdbool.h>
typedef void (*avhp_init_fn_t)(void);
typedef struct { avhp_init_fn_t memory_init; avhp_init_fn_t interrupt_init; avhp_init_fn_t clock_init; avhp_init_fn_t hal_init; } avhp_boot_sequence_t;
bool avhp_boot_run(const avhp_boot_sequence_t *boot);
#endif
