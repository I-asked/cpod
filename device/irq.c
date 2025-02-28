#include "../cpod.h"
#include "irq.h"

#include <Block.h>

void map_irqcon(CMemoryMap *map, uint64_t base, CCore *core) {
  map->base = base;
  map->size = 0x200;
  map->mask = -1;
  map->read = Block_copy(^uint32_t(uint32_t address, uint32_t size) {
    return 0;
  });
  map->write = Block_copy(^(uint32_t address, uint32_t size, uint32_t value) {

  });
}
