#include "timer.h"

#include <Block.h>

#include "irq.h"

CTimerData *create_timer(CPod *cpod) {
  CTimerData *timer = malloc(sizeof(CTimerData));
  memset(timer, '\0', sizeof(CTimerData));
  timer->config = 0;
  timer->source = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
  dispatch_source_set_event_handler(timer->source, ^{
    for (unsigned i = 0; i < cpod->num_cores; ++i) {
      irq_raise(cpod->irqcon, &cpod->cores[i], 0);
    }
  });
  return timer;
}

void map_timer(CTimerData *timer, CMemoryMap *map, uint64_t base, CCore *core) {
  map->base = base;
  map->size = 0x1000;
  map->mask = -1;
  map->data = timer;
  map->read = Block_copy(^uint32_t(uint32_t address, uint32_t size) {
    const unsigned offset = address - base;
    switch (offset) {
    case 0x4: {
      irq_clear(core->cpod->irqcon, core, 0);
      return 0; // TODO:
    } break;
    default: {
      ERROR("timer: unimplemented read%d at offset 0x%03x\n", size, offset);
    } break;
    }
  });
  map->write = Block_copy(^(uint32_t address, uint32_t size, uint32_t value) {
    const unsigned offset = address - base;
    switch (offset) {
    case 0x0: {
      if (value & 0xC0000000) {
        dispatch_source_set_timer(timer->source, DISPATCH_TIME_NOW, (value & 0x1fffffff) * NSEC_PER_USEC, 0);
        dispatch_resume(timer->source);
      } else if (timer->config & 0xC0000000) {
        dispatch_suspend(timer->source);
      }
      timer->config = value;
    } break;
    default: {
      ERROR("timer: unimplemented write%d at offset 0x%03x\n", size, offset);
    } break;
    }
  });
}
