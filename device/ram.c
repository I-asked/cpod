#include "ram.h"

#include <Block.h>

void map_ram(CMemoryMap *map, char *ram, size_t ram_sz) {
  map->base = 0x10000000;
  map->mask = -1;
  map->size = ram_sz;
  map->prot = CMEM_FETCH | CMEM_READ | CMEM_WRITE;
  map->write = Block_copy(^(uint32_t address, uint32_t size, uint32_t value) {
    switch (size) {
    case 1:
      *(uint8_t *)&ram[address - map->base] = value;
      break;
    case 2:
      *(uint16_t *)&ram[address - map->base] = value;
      break;
    case 4:
      *(uint32_t *)&ram[address - map->base] = value;
      break;
    default:
      ERROR("invalid write size %d\n", size);
    }
  });
  map->read = Block_copy(^uint32_t(uint32_t address, uint32_t size) {
    switch (size) {
    case 1:
      return *(uint8_t *)&ram[address - map->base];
    case 2:
      return *(uint16_t *)&ram[address - map->base];
    case 4:
      return *(uint32_t *)&ram[address - map->base];
    default:
      ERROR("invalid read size %d\n", size);
    }
  });
}
