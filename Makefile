-include Makefile.local

CC=$(CC_BINARY)   
CPP=$(CPP_BINARY) 
PPC=$(PPC_BINARY) -n -Cn -CX -Ttyndur  -Fu../lib/units -Fu../units
QEMU=$(QEMU_BINARY)

ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_DIR=$(BUILD_ROOT)

AS=$(AS_BINARY)

all: config
	$(MAKE) --no-print-directory -s makefiles
	$(MAKE) --no-print-directory -s subdirs
	$(MAKE) --no-print-directory -s obj 

makefiles:
	if [ -f buildmk.sh ]; then bash ./buildmk.sh; fi

obj:

subdirs:
	$(MAKE) --no-print-directory -sC doc
	$(MAKE) --no-print-directory -sC src

clean_objs:
	rm -f *.o *.a *.mod *.ppu link.res ppas.sh tyndur

softclean: clean_objs
	for file in *; do if [ -f "$$file/Makefile" -a \( ! -f "$$file/.nobuild" -o -f "$$file/.ignorenobuild" \) ]; then $(MAKE) -sC "$$file" softclean; fi done

clean: clean_objs
	for file in *; do if [ -f "$$file/Makefile" -a \( ! -f "$$file/.nobuild" -o -f "$$file/.ignorenobuild" \) ]; then $(MAKE) -sC "$$file" clean; rm "$$file/Makefile"; fi done
	rm -f Makefile.local

.SILENT: all makefiles subdirs obj lbuilds-env enable-pascal disable-pascal clean softclean clean_objs clean_root updateroot image-floppy image-hd test-qemu test-qemu-hd
.PHONY: lbuilds-env

# Code für lbuilds
lbuilds-env:
	if [ -d lbuilds ]; then cd lbuilds && git pull; else git clone git://git.tyndur.org/lbuilds.git lbuilds; fi
	touch lbuilds/.nobuild
	cd lbuilds && scripts/build_crosstools && scripts/update_env `dirname \`pwd\`` && ln -sf `pwd`/tmp/cross-fpc/fpc-2.4.0/rtl/ `dirname \`pwd\``/src/modules/pas/lib/rtl/include
	make enable-pascal
	$(MAKE)
	cd lbuilds && scripts/update_env ..

enable-pascal:
	touch src/modules/pas/.ignorenobuild

disable-pascal:
	rm -f src/modules/pas/.ignorenobuild

clean: clean_root

clean_root:
	rm -rf build/images/* build/root/*
	rm -f build/output/apps/* build/output/modules/* build/output/kernel/* build/output/gz/*/*
	rm -f qemu-serial.log

updateroot: subdirs
	build/scripts/updateroot

image-floppy: updateroot
	build/scripts/image_floppy_$(GRUB_VERSION)

image-hd: updateroot
	build/scripts/image_hd_$(GRUB_VERSION)

image-cdrom: updateroot
	build/scripts/image_cdrom_$(GRUB_VERSION)

test-qemu-floppy: image-floppy
	$(QEMU) -serial stdio -fda build/images/floppy.img -boot a -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-qemu-hd: image-hd
	$(QEMU) -serial stdio -hda build/images/hd.img -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-qemu-cdrom: image-cdrom
	$(QEMU) -serial stdio -cdrom build/images/cdrom.img -boot d -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-bochs: image-floppy
	bochs -f bochs.config

cscope:
	rm -f cscope.*
	find `pwd`/src -name '*.[ch]' > cscope.files
	cscope -bk

.PHONY: config
config: src/include/lost/config.h

src/include/lost/config.h:
	cp src/include/lost/default_config.h src/include/lost/config.h

menuconfig: src/include/lost/config.h
	python config.py

documentation:
	test -d doc/doxygen || mkdir doc/doxygen
	test -d doc/doxygen/modules || mkdir doc/doxygen/modules
	test -d doc/doxygen/kernel || mkdir doc/doxygen/kernel
	rm -R doc/doxygen/modules/*
	rm -R doc/doxygen/kernel/*
	doxygen doxyfile_modules
	doxygen doxyfile_kernel

bugreport:
	mkdir bugreport
	gcc -v 2> bugreport/gcc.version
	nasm -v > bugreport/nasm.version
	ld -v > bugreport/ld.version
	
	cp build/*.mod bugreport/
	cp build/*.krn bugreport/
	
	tar cjf bugreport.tar.bz2 bugreport
	rm -rf bugreport
