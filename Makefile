XDIR:=/u/cs452/public/xdev
ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CC:=$(XBINDIR)/$(TRIPLE)-gcc
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump
INC:=-Iinclude -Itest

MMU?=on
OPT?=on
ICACHE?=on
DCACHE?=on
PERFTEST?=off
MEASURE_TRAIN_SPEED?=off
TRACKA?=on
IDLE_TIME_WINDOW_TICKS?=500 # in ticks

# COMPILE OPTIONS
WARNINGS=-Wall -Wextra -Wpedantic -Wno-unused-const-variable
CFLAGS:=-g -pipe -static -mstrict-align -ffreestanding -mgeneral-regs-only \
	-mcpu=$(ARCH) -march=armv8-a $(WARNINGS) $(INC)

ifeq ($(MMU),on)
MMUFLAGS:=-DMMU
else
MMUFLAGS:=-mstrict-align -mgeneral-regs-only
endif
ifeq ($(OPT),on)
OPTFLAGS=-O3
endif

CFLAGS:=-g -pipe -static -mstrict-align -ffreestanding -mgeneral-regs-only \
	-mcpu=$(ARCH) -march=armv8-a $(WARNINGS) $(INC) $(MMUFLAGS) $(OPTFLAGS)

ifneq ($(ICACHE),on)
CFLAGS += -DNOICACHE
endif
ifneq ($(DCACHE),on)
CFLAGS += -DNODCACHE
endif

ifeq ($(PERFTEST),on)
CFLAGS += -DPERFTEST
endif

ifeq ($(MEASURE_TRAIN_SPEED),on)
CFLAGS += -DMEASURE_TRAIN_SPEED
endif

ifeq ($(TRACKA),on)
CFLAGS += -DTRACKA
endif

CFLAGS += -DIDLE_TIME_WINDOW_TICKS=$(IDLE_TIME_WINDOW_TICKS)

# -Wl,option tells g++ to pass 'option' to the linker with commas replaced by spaces
# doing this rather than calling the linker directly simplifies the compilation procedure
LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld -Wl,--no-warn-rwx-segments -nostartfiles

# Source files and include dirs
SOURCES := $(wildcard src/*.c) $(wildcard asm/*.S)
$(info $(SOURCES))
# Create .o and .d files for every .cc and .S (hand-written assembly) file
OBJECTS := $(patsubst src/%.c, build/%.o, $(patsubst asm/%.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst src/%.c, build/%.d, $(patsubst asm/%.S, build/%.d, $(SOURCES)))

# The first rule is the default, ie. "make", "make all" and "make kernal8.img" mean the same
all: bin/kernel.img

clean:
	rm -rf bin build

bin/kernel.img: build/kernel.elf | bin
	$(OBJCOPY) $< -O binary $@

bin:
	mkdir -p $@

build/kernel.elf: $(OBJECTS) linker.ld | build
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)
ifneq ($(MMU),on)
	@$(OBJDUMP) -d $@ | grep -Fq q0 && printf "\n***** WARNING: SIMD DETECTED! *****\n\n" || true
endif

build/%.o: src/%.c Makefile | build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: asm/%.S Makefile | build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p $@

-include $(DEPENDS)
