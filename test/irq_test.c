#include "../device/irq.h"

#include <setjmp.h>
#include <cmocka.h>

static void irq_test(void **state) {
  // TODO:XXX:
  map_irqcon(NULL, 0, NULL);
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(irq_test),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
