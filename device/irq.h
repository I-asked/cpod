#pragma once

#include "../cpod.h"

#include <dispatch/dispatch.h>

#include <stdbool.h>

typedef struct _CIRQConData {
  struct {
    uint32_t lostate;
    uint32_t mask;
  } cores[MAX_CORES];
} CIRQConData;

CIRQConData *create_irqcon(CPod *cpod);

void map_irqcon(CIRQConData *data, CMemoryMap *map, uint64_t base, CCore *core);

bool irq_sched(CIRQConData *irqcon, CCore *core);

void irq_raise(CIRQConData *irqcon, CCore *core, uint32_t irq);

void irq_clear(CIRQConData *irqcon, CCore *core, uint32_t irq);
