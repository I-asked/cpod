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
        map->read = Block_copy(^uint32_t(uint32_t address_wr, uint32_t size_wr) {
          switch (size_wr) {
          case 1: {
            return core->arm7.read8((value & 0xFFFF0000) + address_wr - map->base);
          } break;
          case 2: {
            return core->arm7.read16((value & 0xFFFF0000) + address_wr - map->base);
          } break;
          case 4: {
            return core->arm7.read32((value & 0xFFFF0000) + address_wr - map->base);
          } break;
          default: {
            ERROR("mmu: read size mismatch\n");
          }
          }
        });
        map->write = Block_copy(^(uint32_t address_wr, uint32_t size_wr, uint32_t value_wr) {
          switch (size_wr) {
          case 1: {
            core->arm7.write8((value & 0xFFFF0000) + address_wr - map->base, value_wr);
          } break;
          case 2: {
            core->arm7.write16((value & 0xFFFF0000) + address_wr - map->base, value_wr);
          } break;
          case 4: {
            core->arm7.write32((value & 0xFFFF0000) + address_wr - map->base, value_wr);
          } break;
          }
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
