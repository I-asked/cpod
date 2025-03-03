#pragma once

#include "../cpod.h"

typedef struct _CTimerData {
  uint32_t config;
  dispatch_source_t source;
} CTimerData;

CTimerData *create_timer(CPod *cpod);

void map_timer(CTimerData *timer, CMemoryMap *map, uint64_t base, CCore *core);
