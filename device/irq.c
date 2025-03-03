#include "../cpod.h"
#include "irq.h"

#include <Block.h>

CIRQConData *create_irqcon(CPod *cpod) {
  CIRQConData *irqcon = malloc(sizeof(CIRQConData));
  memset(irqcon, '\0', sizeof(CIRQConData));
  return irqcon;
}

void map_irqcon(CIRQConData *irqcon, CMemoryMap *map, uint64_t base, CCore *core) {
  map->base = base;
  map->size = 0x200;
  map->mask = -1;
  map->data = irqcon;
  map->read = Block_copy(^uint32_t(uint32_t address, uint32_t size) {
    const unsigned offset = address - base;
    switch (offset) {
    case 0: {
      //__block uint32_t state;
      //dispatch_sync(core->cpod->cores[0].queue, ^{ state = irqcon->cores[0].lostate; });
      //return state;
      return irqcon->cores[0].lostate;
    } break;
    default: {
      ERROR("irqcon: unimplemented read%d at offset 0x%03x\n", size, offset);
    } break;
    }
  });
  map->write = Block_copy(^(uint32_t address, uint32_t size, uint32_t value) {
    const unsigned offset = address - base;
    switch (offset) {
    case 0x024: {
      dispatch_async(core->cpod->cores[0].queue, ^{ irqcon->cores[0].mask |= value; });
    } break;
    case 0x028: {
      dispatch_async(core->cpod->cores[0].queue, ^{ irqcon->cores[0].mask &= ~value; });
    } break;
    default: {
      ERROR("irqcon: unimplemented write%d at offset 0x%03x\n", size, offset);
    } break;
    }
  });
}

bool irq_sched(CIRQConData *irqcon, CCore *core) {
  return irqcon->cores[core->index].lostate;
}

void irq_raise(CIRQConData *irqcon, CCore *core, uint32_t irq) {
  dispatch_async(core->queue, ^{
    if (irqcon->cores[core->index].mask & (1 << irq)) {
      irqcon->cores[core->index].lostate |= (1 << irq);
      arm7_process_interrupts(&core->arm7);
    }
  });
}

void irq_clear(CIRQConData *irqcon, CCore *core, uint32_t irq) {
  dispatch_async(core->queue, ^{
    irqcon->cores[core->index].lostate &= ~(1 << irq);
  });
}
