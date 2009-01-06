CC=gcc -m32 -g -c -fno-stack-protector -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -Werror -Wstrict-prototypes -fno-strict-aliasing -O2 -fno-builtin -I .   
CPP=g++ -m32 -g -c -fno-stack-protector -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 
PPC=fpc -n -Cn -CX -Ttyndur  -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/siolalex/Development/lost/svn/trunk/build/output
BUILD_DIR=$(BUILD_ROOT)

AS=as -32

all:
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 

makefiles:
	if [ -f buildmk.sh ]; then bash ./buildmk.sh; fi

obj:

subdirs:
	make --no-print-directory -sC doc
	make --no-print-directory -sC src

clean:
	rm -f *.o *.a *.mod
	for file in *; do if [ -f "$$file/Makefile" -a \( ! -f "$$file/.nobuild" -o -f "$$file/.ignorenobuild" \) ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean clean_root updateroot image-floppy image-hd test-qemu test-qemu-hd
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
