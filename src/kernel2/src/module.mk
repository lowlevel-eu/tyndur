### Kernel 2
kernel2: build/lost2.krn

KERNEL2_SRC = $(call SOURCE_FILES_WILDCARD,src/kernel2/src/arch/$(ARCH)) $(call SOURCE_FILES_WILDCARD,src/kernel2/src/arch/$(ARCH)/*) $(call SOURCE_FILES,src/kernel2/src) $(call SOURCE_FILES_WILDCARD,src/kernel2/src/*)
KERNEL2_OBJS = $(call OBJECT_FILES,$(KERNEL2_SRC))
ALL_OBJS += $(KERNEL2_OBJS)
build/lost2.krn: $(KERNEL2_OBJS) src/lib/library.a
	ld -o $@ $(LDFLAGS_KERNEL2) $^
build/lost2.krn: CPPFLAGS += -Isrc/kernel2/include -Isrc/kernel2/include/arch/$(ARCH)

zkernel2: build/lost2.kgz

src/kernel2/src/smp/rm_trampoline.o: src/kernel2/src/smp/rm_trampoline.asm
	$(NASM) -f bin $< -o $(notdir $@)
	$(OBJCOPY) -B i386:i386 -I binary -O elf32-i386 $(notdir $@) $@
	rm $(notdir $@)
