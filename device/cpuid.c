#include "cpuid.h"

#include "Block.h"

void map_cpuid(CMemoryMap *map, uint64_t base, uint8_t (^read)(void)) {
  map->base = base;
  map->mask = base;
  map->size = 1;
  map->prot = CMEM_READ;
  map->data = read;
  map->read = Block_copy(^uint32_t(uint32_t address, uint32_t size) { return ((uint8_t(^)(void))map->data)(); });
  map->write = Block_copy(^void(uint32_t address, uint32_t size, uint32_t data) { ERROR("Unimplemented\n"); });
}
