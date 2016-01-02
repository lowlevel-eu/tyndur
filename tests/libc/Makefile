-include ../../Makefile.local

CC=$(CC_BINARY)
LD=$(LD_BINARY)
QEMU=$(QEMU_BINARY)

LIBGCC=$(shell $(CC) -print-libgcc-file-name)

CFLAGS=-I ../include -I ../../src/include -I../../src/modules/include -I ../../src/include/arch/i386 -I../../src/modules/include/arch/i386
LDFLAGS=-T ../../src/modules/user-i386.ld --start-group ../../src/lib/library.a ../../src/modules/lib/library.a ../../src/modules/lib/crt0.o $(LIBGCC) --end-group

export QEMU

qemu: main main_thread
	@./run_test.sh

main: main.o qsort.o bsearch.o sprintf.o stdio.o time.o ../lib/testlib.o
	@$(LD) $^ $(LDFLAGS) -o $@

main_thread: main_thread.o ../lib/testlib.o
	@$(LD) $^ $(LDFLAGS) -o $@

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ $^

.PHONY: qemu
