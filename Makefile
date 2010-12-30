CC=gcc -m32 -g -c -fno-stack-protector -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -Werror -Wstrict-prototypes -fno-strict-aliasing -O2 -fno-builtin -I .   
CPP=g++ -m32 -g -c -fno-stack-protector -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 
PPC=fpc -n -Cn -CX -Ttyndur  -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/johannes/Programme/Tyndur/tyndur/build/output
BUILD_DIR=$(BUILD_ROOT)

AS=as -32

all:
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

.SILENT: all makefiles subdirs obj clean softclean clean_objs clean_root updateroot image-floppy image-hd test-qemu test-qemu-hd
clean: clean_root

clean_root:
	rm -rf build/images/* build/root/*
	rm -f build/output/apps/* build/output/modules/* build/output/kernel/* build/output/gz/*/*
	rm -f qemu-serial.log

updateroot: subdirs
	build/scripts/updateroot

image-floppy: updateroot
	build/scripts/image_floppy

image-hd: updateroot
	build/scripts/image_hd

image-cdrom: updateroot
	build/scripts/image_cdrom

grub2-cdrom: updateroot
	build/scripts/grub2_cdrom

test-qemu: image-floppy
	qemu -serial stdio -fda build/images/floppy.img -boot a -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-qemu-hd: image-hd
	qemu -serial stdio -hda build/images/hd.img -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-bochs: image-floppy
	bochs -f bochs.config

menuconfig:
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
