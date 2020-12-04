### Kernel
kernel: build/lost2.krn

KERNEL2_SRC = $(call SOURCE_FILES_WILDCARD,src/kernel/src/arch/$(ARCH)) $(call SOURCE_FILES_WILDCARD,src/kernel/src/arch/$(ARCH)/*) $(call SOURCE_FILES,src/kernel/src) $(call SOURCE_FILES_WILDCARD,src/kernel/src/*)
KERNEL2_OBJS = $(call OBJECT_FILES,$(KERNEL2_SRC))
ALL_OBJS += $(KERNEL2_OBJS)
build/lost2.krn: $(KERNEL2_OBJS) src/lib/library.a
	ld -o $@ $(LDFLAGS_KERNEL2) $^
build/lost2.krn: CPPFLAGS += -Isrc/kernel/include -Isrc/kernel/include/arch/$(ARCH)

zkernel: build/lost2.kgz

src/kernel/src/smp/rm_trampoline.o: src/kernel/src/smp/rm_trampoline.asm
	$(NASM) -f bin $< -o $(notdir $@)
	$(OBJCOPY) -B i386:i386 -I binary -O elf32-i386 $(notdir $@) $@
	rm $(notdir $@)
