#include "mmu.h"

#include <Block.h>

void map_memcon(CMemoryMap *map, uint64_t base, CCore *core) {
  map->base = base;
  map->size = 0x10000;
  map->prot = CMEM_READ | CMEM_WRITE;
  map->mask = -1;
  map->data = malloc(sizeof(CMemoryMap[4]));
  CMemoryMap *maps = (CMemoryMap *)map->data;
  maps[0].next = &maps[1];
  maps[1].next = &maps[2];
  maps[2].next = &maps[3];
  maps[3].next = core->mmap_head;
  core->mmap_head = maps;
  map->write = Block_copy(^(uint32_t address, uint32_t size, uint32_t value) {
    uint32_t index = address - base;
    if (index > 0xF000 && index <= 0xF01C) {
      uint8_t offset = index & 0xFF;
      uint8_t unit_index = offset / 8;
      uint8_t unit_sub = offset % 8;
      CMemoryMap *map = &maps[unit_index];
      if (unit_sub == 4) {
        map->prot |= CMEM_READ | CMEM_WRITE | CMEM_FETCH;
        map->read = Block_copy(^uint32_t(uint32_t address, uint32_t size) {
          return core->arm7.read32((value & 0xFFFF0000) + address - map->base);
        });
        map->write = Block_copy(^(uint32_t address, uint32_t size, uint32_t value_wr) {
          core->arm7.write32((value & 0xFFFF0000) + address - map->base, value_wr);
        });
      } else if (unit_sub == 0) {
        // TODO:XXX: logic!
        map->base = value & 0xFFFF0000;
        map->size = 0x10000000;
        map->mask = -1;
      }
    }
  });
}
