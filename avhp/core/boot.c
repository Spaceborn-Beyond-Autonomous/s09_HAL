#include "boot.h"
bool avhp_boot_run(const avhp_boot_sequence_t *boot){if(!boot)return false;if(boot->memory_init)boot->memory_init();if(boot->interrupt_init)boot->interrupt_init();if(boot->clock_init)boot->clock_init();if(boot->hal_init)boot->hal_init();return true;}
