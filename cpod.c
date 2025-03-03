#include "cpod.h"
#include "sanity.h"
#include "device/ram.h"
#include "device/cpuid.h"
#include "device/cpucon.h"
#include "device/irq.h"
#include "device/timer.h"
#include "device/mmu.h"

#include <stdio.h>
#include <setjmp.h>

#include <Block.h>
#include <dispatch/dispatch.h>
#include <SDL3/SDL.h>

static CPod cpod;

static int init_core(CCore *core) {
  core->arm7 = arm7_init();

  uint32_t (^mread)(uint32_t addr, size_t size) = Block_copy(^uint32_t(uint32_t address, size_t size) {
    CMemoryMap *map = core->mmap_head;
    do {
      if ((address >= map->base) && ((address + size) <= (map->base + map->size)) /*&& (((size - 1) << __builtin_clz(address)) & map->mask)*/) {
        uint32_t res = map->read(address, size);
        return res;
      }
    } while ((map = map->next)) ;
    ERROR("unmapped read%d from 0x%08x @ pc = 0x%08x (core %d)\n", size * 8, address, core->arm7.registers[PC], core->index);
  });
  core->arm7.read8 = Block_copy(^uint8_t(uint32_t address) { return mread(address, 1); });
  core->arm7.read16 = Block_copy(^uint32_t(uint32_t address) { return mread(address, 2); });
  core->arm7.read32 = Block_copy(^uint32_t(uint32_t address) { return mread(address, 4); });
  core->arm7.read16_seq = Block_copy(^uint32_t(uint32_t address, bool _isseq) { return mread(address, 2); });
  core->arm7.read32_seq = Block_copy(^uint32_t(uint32_t address, bool _isseq) { return mread(address, 4); });

  void (^mwrite)(uint32_t addr, size_t size, uint32_t value) = Block_copy(^(uint32_t address, size_t size, uint32_t value) {
    CMemoryMap *map = core->mmap_head;
    do {
      if ((address >= map->base) && ((address + size) <= (map->base + map->size)) /*&& (((size - 1) << __builtin_clz(address)) & map->mask)*/) {
        map->write(address, size, value);
        return;
      }
    } while ((map = map->next)) ;
    ERROR("unmapped write%zu to 0x%08x @ pc = 0x%08x, sp = 0x%08x (core %zu)\n", size * 8, address, core->arm7.registers[PC], core->arm7.registers[13], core->index);
  });
  core->arm7.write8 = Block_copy(^(uint32_t address, uint8_t value) { mwrite(address, 1, value); });
  core->arm7.write16 = Block_copy(^(uint32_t address, uint16_t value) { mwrite(address, 2, value); });
  core->arm7.write32 = Block_copy(^(uint32_t address, uint32_t value) { mwrite(address, 4, value); });

  return 0;
}

static void schedule_core(CCore *core) {
  core->clock = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, core->queue);
  dispatch_source_set_timer(core->clock, DISPATCH_TIME_NOW, NSEC_PER_USEC / core->megahertz + 1, 0);
  dispatch_source_set_event_handler(core->clock, ^{
    do {
      arm7_exec_instruction(&core->arm7);
    } while ((core->arm7.phased_op_id)) ;
  });
  dispatch_resume(core->clock);
}

static void my_close(void) {
  free(cpod.iram);
  free(cpod.sram);
}

#ifndef CPOD_TEST

int main(int argc, char *argv[]) {
  if (argc < 2) {
    ERROR("Usage: %s [rom]\n", argv[0]);
  }

  memset(&cpod, '\0', sizeof(cpod));

  cpod.iram_sz = 96 * 1024;
  cpod.sram_sz = 32 * 1024 * 1024;

  cpod.iram = calloc(cpod.iram_sz, 1);
  cpod.sram = calloc(cpod.sram_sz, 1);

  cpod.model_name = "iPod Photo";
  cpod.lcd_width = 220;
  cpod.lcd_height = 176;

  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    ERROR("failed to open \"%s\"\n", argv[1]);
  }
  fread(cpod.sram, 1, cpod.sram_sz, f);
  fclose(f);

  SDL_Init(SDL_INIT_VIDEO);

  cpod.num_cores = 2;

  cpod.cpucon = create_cpucon(&cpod);
  cpod.irqcon = create_irqcon(&cpod);
  cpod.timer = create_timer(&cpod);

  for (unsigned i = 0; i < cpod.num_cores; ++i) {
    CCore *core = &cpod.cores[i];
    core->megahertz = 80;
    core->queue = dispatch_queue_create("core_queue", DISPATCH_QUEUE_SERIAL);
    core->index = i;
    core->cpod = &cpod;
    core->entry = 0x10000000;
    init_core(core);

    core->arm7.registers[PC] = 0x10000000;
    core->arm7.registers[CPSR] = 0xD3;

    CMemoryMap *map;

    map = core->mmap_head = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_ram(map, cpod.sram, 0x10000000, cpod.sram_sz);

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_ram(map, cpod.iram, 0x40000000, cpod.iram_sz);

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_cpuid(map, 0x60000000, (i == 0) ? 0x55 : 0xAA);

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_cpucon(cpod.cpucon, map, 0x60007000, core);

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_irqcon(cpod.irqcon, map, 0x60004000, core);

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_timer(cpod.timer, map, 0x60005000, core);

    // memcon goes last
    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_memcon(map, 0xf0000000, core);
  }

  SDL_CreateWindowAndRenderer(cpod.model_name, cpod.lcd_width, cpod.lcd_height, 0, &cpod.sdl_win, &cpod.sdl_ren);
  cpod.sdl_sur = SDL_CreateSurface(cpod.lcd_width, cpod.lcd_height, SDL_PIXELFORMAT_BGR565);

  atexit(my_close);

  for (unsigned i = 0; i < cpod.num_cores; ++i) {
    schedule_core(&cpod.cores[i]);
  }

  dispatch_main();
}

#endif
