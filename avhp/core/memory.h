#ifndef ANSA_AVHP_MEMORY_H
#define ANSA_AVHP_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum { AVHP_MEM_FLASH, AVHP_MEM_SRAM, AVHP_MEM_EEPROM, AVHP_MEM_STACK, AVHP_MEM_HEAP } avhp_memory_region_type_t;
typedef struct { uint32_t base; size_t size; uint8_t *storage; avhp_memory_region_type_t type; } avhp_memory_region_t;
typedef struct { avhp_memory_region_t *regions; size_t count; } avhp_memory_map_t;

bool avhp_memory_init(avhp_memory_map_t *map, avhp_memory_region_t *regions, size_t count);
avhp_memory_region_t *avhp_memory_find(avhp_memory_map_t *map, uint32_t address, size_t len);
bool avhp_memory_read(avhp_memory_map_t *map, uint32_t address, void *out, size_t len);
bool avhp_memory_write(avhp_memory_map_t *map, uint32_t address, const void *data, size_t len);
void avhp_memory_reset(avhp_memory_map_t *map, uint8_t fill);
#endif
