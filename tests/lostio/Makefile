-include ../../Makefile.local

CC=$(CC_BINARY)
LD=$(LD_BINARY)
QEMU=$(QEMU_BINARY)

LIBGCC=$(shell $(CC) -print-libgcc-file-name)

CFLAGS=-I ../../src/include -I../../src/modules/include -I ../../src/include/arch/i386 -I../../src/modules/include/arch/i386
LDFLAGS=-T ../../src/modules/user-i386.ld --start-group ../../src/lib/library.a ../../src/modules/lib/library.a ../../src/modules/lib/crt0.o $(LIBGCC) --end-group

export QEMU

qemu: 001
	@./run_test.sh

001: 001.o
	@$(LD) $^ $(LDFLAGS) -o $@

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ $^

.PHONY: qemu
