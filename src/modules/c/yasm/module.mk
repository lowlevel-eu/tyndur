yasm_subdirs += \
frontends/yasm \
libyasm \
$(addprefix modules/, \
	arch/x86 \
	$(addprefix dbgfmts/, \
		codeview \
		dwarf2 \
		null \
		stabs \
	) \
	listfmts/nasm/ \
	$(addprefix objfmts/, \
		bin \
		coff \
		dbg \
		elf \
		macho \
		rdf \
		win32 \
		xdf \
	) \
	$(addprefix parsers/, \
		gas \
		nasm \
	) \
	$(addprefix preprocs/, \
		nasm \
		raw \
	) \
)
	
yasm_SRC += $(foreach subdir,$(yasm_subdirs),$(call SOURCE_FILES,$(MODULE_PATH)/$(subdir)))
yasm_OBJS := $(call OBJECT_FILES,$(yasm_SRC))

$(MODULE_PATH)/modules/arch/x86/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/arch/x86/include
$(MODULE_PATH)/modules/dbgfmts/codeview/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/dbgfmts/codeview/include
$(MODULE_PATH)/modules/dbgfmts/dwarf2/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/dbgfmts/dwarf2/include
$(MODULE_PATH)/modules/objfmts/coff/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/objfmts/coff/include
$(MODULE_PATH)/modules/objfmts/elf/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/objfmts/elf/include
$(MODULE_PATH)/modules/parsers/gas/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/parsers/gas/include
$(MODULE_PATH)/modules/parsers/nasm/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/parsers/nasm/include
$(MODULE_PATH)/modules/preprocs/nasm/%.o: CPPFLAGS += -I$(MODULE_PATH)/modules/preprocs/nasm/include

build/apps/yasm: CPPFLAGS += -I$(MODULE_PATH)/frontends/yasm/include 

eys:
	echo $(yasm_SRC)