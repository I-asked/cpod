#include "cpod.h"
#include "sanity.h"
#include "device/ram.h"
#include "device/cpuid.h"
#include "device/mmu.h"

#include <stdio.h>
#include <setjmp.h>

#include <Block.h>
#include <dispatch/dispatch.h>
#include <SDL3/SDL.h>

static CPod cpod;

static void register_dump(CCore *core) {
  // TODO
}

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
    ERROR("unmapped read%d from 0x%08x @ pc = 0x%08x\n", size * 8, address, core->arm7.registers[PC]);
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
    ERROR("unmapped write%d to 0x%08x @ pc = 0x%08x\n", size * 8, address, core->arm7.registers[PC]);
  });
  core->arm7.write8 = Block_copy(^(uint32_t address, uint8_t value) { mwrite(address, 1, value); });
  core->arm7.write16 = Block_copy(^(uint32_t address, uint16_t value) { mwrite(address, 2, value); });
  core->arm7.write32 = Block_copy(^(uint32_t address, uint32_t value) { mwrite(address, 4, value); });

  return 0;
}

static void schedule_core(CCore *core) {
  dispatch_async(core->cpod->queue, ^{
    for (;;) {
      dispatch_semaphore_wait(core->resource, DISPATCH_TIME_FOREVER);
      arm7_exec_instruction(&core->arm7);
      dispatch_semaphore_signal(core->resource);
    }
  });
}

static void my_close(void) {
  free(cpod.iram);
  free(cpod.sram);
}

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
  for (unsigned i = 0; i < cpod.num_cores; ++i) {
    CCore *core = &cpod.cores[i];
    core->index = i;
    core->cpod = &cpod;
    core->entry = 0x10000000;
    core->resource = dispatch_semaphore_create(1);
    init_core(core);

    core->arm7.registers[PC] = 0x10000000;
    core->arm7.registers[CPSR] = 0xD3;

    CMemoryMap *map = core->mmap_head = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_ram(map, cpod.sram, cpod.sram_sz);

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_ram(map, cpod.iram, cpod.iram_sz);

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_cpuid(map, 0x60000000, Block_copy(^uint8_t{
      return i == 0 ? 0x55 : 0xAA;
    }));

    map = map->next = malloc(sizeof(CMemoryMap));
    memset(map, '\0', sizeof(*map));
    map_memcon(map, 0xf0000000, core);
  }

  SDL_CreateWindowAndRenderer(cpod.model_name, cpod.lcd_width, cpod.lcd_height, 0, &cpod.sdl_win, &cpod.sdl_ren);

  atexit(my_close);

  cpod.queue = dispatch_queue_create("cpod.cores", DISPATCH_QUEUE_CONCURRENT);
  schedule_core(&cpod.cores[0]);
  schedule_core(&cpod.cores[1]);

  dispatch_main();
}

static int find_device(const void *a, const void *b) {
  return (*(const CDevice **)a)->device_id - (*(const CDevice **)b)->device_id;
}

CDevice *get_device(CPod *cpod, uint64_t device_id) {
  CDevice cdev = { device_id };
  CDevice *com = &cdev;
  return bsearch(&com, cpod->devices, cpod->num_devices, sizeof(CDevice *), find_device);
}

void suspend_core(CCore *core) {
  dispatch_semaphore_wait(core->resource, DISPATCH_TIME_FOREVER);
}

void resume_core(CCore *core) {
  dispatch_semaphore_signal(core->resource);
}
