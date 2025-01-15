XDIR:=/u/cs452/public/xdev
ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CC:=$(XBINDIR)/$(TRIPLE)-gcc
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump

# COMPILE OPTIONS
WARNINGS=-Wall -Wextra -Wpedantic -Wno-unused-const-variable
CFLAGS:=-g -pipe -static -mstrict-align -ffreestanding -mgeneral-regs-only \
	-mcpu=$(ARCH) -march=armv8-a $(WARNINGS)

# -Wl,option tells g++ to pass 'option' to the linker with commas replaced by spaces
# doing this rather than calling the linker directly simplifies the compilation procedure
LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld -Wl,--no-warn-rwx-segments -nostartfiles

# Source files and include dirs
SOURCES := $(wildcard src/*.c) $(wildcard src/*.S)
$(info $(SOURCES))
# Create .o and .d files for every .cc and .S (hand-written assembly) file
OBJECTS := $(patsubst src/%.c, build/%.o, $(patsubst src/%.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst src/%.c, build/%.d, $(patsubst src/%.S, build/%.d, $(SOURCES)))

# The first rule is the default, ie. "make", "make all" and "make kernal8.img" mean the same
all: bin/kernel.img

clean:
	rm -f $(OBJECTS) $(DEPENDS) kernel.elf bin/kernel.img

bin/kernel.img: kernel.elf
	$(OBJCOPY) $< -O binary $@

kernel.elf: $(OBJECTS) linker.ld
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)
	@$(OBJDUMP) -d $@ | grep -Fq q0 && printf "\n***** WARNING: SIMD DETECTED! *****\n\n" || true

build/%.o: src/%.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: src/%.S Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
