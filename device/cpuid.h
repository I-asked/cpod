#pragma once

#include "../cpod.h"

void map_cpuid(CMemoryMap *cpod, uint64_t base, uint8_t (^read)(void));
