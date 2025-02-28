#pragma once

#include "sanity.h"

#include <dispatch/dispatch.h>
#include <SDL3/SDL.h>

#include "arm7.h"

#include <stdbool.h>

#define MAX_CORES   (2)
#define MAX_DEVICES (32)

typedef enum _CMemAccess {
  CMEM_READ,
  CMEM_WRITE,
  CMEM_FETCH,
} CMemAccess;

typedef struct _CMemoryMap {
  struct _CMemoryMap *next;

  uint32_t base;
  uint32_t size;
  uint32_t mask;
  uint8_t prot;

  uint32_t (^read)(uint32_t address, uint32_t size);
  void (^write)(uint32_t address, uint32_t size, uint32_t value);

  void *data;
} CMemoryMap;

struct _CCore;
typedef uint32_t (* mem_callback)(struct _CCore *core, uint32_t addr, uint32_t value, CMemAccess type);

typedef struct _CCore {
  size_t index;
  struct _CPod *cpod;

  arm7_t arm7;
  CMemoryMap *mmap_head;

  dispatch_semaphore_t resource;

  uint32_t entry;
} CCore;

typedef struct _CPod {
  CCore cores[MAX_CORES];
  size_t num_cores;

  size_t sram_sz, iram_sz;
  char *sram;
  char *iram;

  char *model_name;
  uint32_t lcd_width, lcd_height;

  SDL_Window *sdl_win;
  SDL_Renderer *sdl_ren;

  dispatch_queue_t queue;
} CPod;

void suspend_core(CCore *core);

void resume_core(CCore *core);
