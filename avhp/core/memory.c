#include "memory.h"
#include <string.h>

bool avhp_memory_init(avhp_memory_map_t *map, avhp_memory_region_t *regions, size_t count) {
    if (!map || !regions || count == 0U) return false;
    map->regions = regions; map->count = count; return true;
}
avhp_memory_region_t *avhp_memory_find(avhp_memory_map_t *map, uint32_t address, size_t len) {
    if (!map || !map->regions) return NULL;
    for (size_t i=0;i<map->count;i++) {
        uint64_t start=map->regions[i].base, end=start+map->regions[i].size;
        uint64_t req=(uint64_t)address + len;
        if ((uint64_t)address >= start && req <= end) return &map->regions[i];
    }
    return NULL;
}
bool avhp_memory_read(avhp_memory_map_t *map, uint32_t address, void *out, size_t len) {
    avhp_memory_region_t *r=avhp_memory_find(map,address,len); if (!r || (!out && len)) return false;
    memcpy(out,r->storage+(address-r->base),len); return true;
}
bool avhp_memory_write(avhp_memory_map_t *map, uint32_t address, const void *data, size_t len) {
    avhp_memory_region_t *r=avhp_memory_find(map,address,len); if (!r || (!data && len)) return false;
    memcpy(r->storage+(address-r->base),data,len); return true;
}
void avhp_memory_reset(avhp_memory_map_t *map, uint8_t fill) { if (!map) return; for(size_t i=0;i<map->count;i++) if(map->regions[i].storage) memset(map->regions[i].storage,fill,map->regions[i].size); }
