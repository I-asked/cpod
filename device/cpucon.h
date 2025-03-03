#pragma once

#include "../cpod.h"

typedef struct _CCPUConData {
  uint32_t data;
} CCPUConData;

CCPUConData *create_cpucon(CPod *cpod);

void map_cpucon(CCPUConData *cpucon, CMemoryMap *map, uint64_t base, CCore *core);
