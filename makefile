CSRC		:= $(wildcard *.c) $(wildcard **/*.c)
OBJS		:= $(CSRC:.c=.o)
DEPS		:= $(wildcard *.d) $(wildcard **/*.d)

TARGET  := cpod

CFLAGS  += -MMD -MP -Wall -Wextra -Werror -std=c99 -pedantic -ggdb -O0
CFLAGS  += -fsanitize=address
LDFLAGS += -fsanitize=address

CFLAGS  += -Wno-unused-parameter -Wno-gnu-zero-variadic-macro-arguments
CFLAGS  += -Wno-error=sign-compare -Wno-error=missing-field-initializers \
           -Wno-error=unused-variable -Wno-error=strict-prototypes \
					 -Wno-error=char-subscripts -Wno-error=format \
					 -Wno-error=unused-function

CCID		:= $(shell $(CC) --version |head -1 |grep -E '^\S*')
OSID		:= $(shell uname -s)

ifeq ($(OSID),Linux)
ifneq ($(CCID),clang)
$(warning Compilers other than clang are not guaranteed to work on Linux!)
endif
endif


# SDL3
CFLAGS	+= $(shell pkg-config --cflags sdl3)
LDLIBS	+= $(shell pkg-config --libs-only-l sdl3)
LDFLAGS += $(shell pkg-config --libs-only-L --libs-only-other sdl3)


# Grand Central Dispatch
CFLAGS	+= -fblocks
LDLIBS	+= -ldispatch -lBlocksRuntime


$(TARGET): $(OBJS)


.PHONY: clean distclean
clean:
	$(RM) $(OBJS) $(DEPS)

distclean: clean
	$(RM) $(TARGET)


-include $(DEPS)
