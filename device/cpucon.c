#include "cpucon.h"

#include <Block.h>

#include "irq.h"

CCPUConData *create_cpucon(CPod *cpod) {
  CCPUConData *cpucon = malloc(sizeof(CCPUConData));
  memset(cpucon, '\0', sizeof(CCPUConData));
  return cpucon;
}

void map_cpucon(CCPUConData *cpucon, CMemoryMap *map, uint64_t base, CCore *core) {
  map->base = base;
  map->size = 0x1000;
  map->mask = -1;
  map->data = cpucon;
  map->read = Block_copy(^uint32_t(uint32_t address, uint32_t size) {
    const unsigned offset = address - base;
    ERROR("cpucon: unimplemented read%d at offset 0x%03x\n", size, offset);
  });
  map->write = Block_copy(^(uint32_t address, uint32_t size, uint32_t value) {
    //const unsigned offset = address - base;
    //fprintf(stderr, "cpucon at 0x%08x set to 0x%04x\n", address, value);
  });
}
