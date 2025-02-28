#pragma once

#include <stdio.h>

#define ASSERT(expr, op, expect) \
  do { \
    signed err = (expr); \
    if (err op expect) { \
      fprintf(stderr, "assertion failed for " #expr " " #op " " #expect ": %s\n", uc_strerror(err)); \
      abort(); \
    } \
  } while (0)

#define ERROR(fmt, ...) \
  do { \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    abort(); \
  } while (0)
