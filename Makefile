CC=""gcc -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 
CPP=""g++ -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 
PPC=fpc -Cn -CX -Tlost 

ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/toni/arbeitskopien/lost/trunk/build
BUILD_DIR=$(BUILD_ROOT)


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj:

subdirs:
	make --no-print-directory -sC apps
	make --no-print-directory -sC build
	make --no-print-directory -sC doc
	make --no-print-directory -sC src

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean
image: all
	    dd if=/dev/zero of=build/lost.img bs=1024 count=1440
	    mkfs.msdos build/lost.img
	    mkdir -p build/mnt
	    mount -oloop build/lost.img build/mnt
	
	    mkdir build/mnt/grub
	    cp /boot/grub/stage? build/mnt/grub
	

	cp build/default.cfg build/mnt/grub/menu.lst

	cp build/*.krn.gz build/mnt
	cp build/*.mod.gz build/mnt
	umount build/mnt
	rmdir build/mnt
	
	echo device '(fd0)' build/lost.img > build/grub.txt
	echo root '(fd0)' >> build/grub.txt
	echo setup '(fd0)' >> build/grub.txt
	grub --batch < build/grub.txt
	rm build/grub.txt

image-update: all
	mkdir -p build/mnt
	    mount -oloop build/lost.img build/mnt
	    rm -rf build/mnt/grub/menu.lst
    
	 cp build/default.cfg build/mnt/grub/menu.lst
	cp build/*.krn.gz build/mnt
	cp build/*.mod.gz build/mnt
	umount build/mnt
	rm -rf build/mnt

test-qemu: image
	qemu -serial stdio -fda build/lost.img -boot a

test-bochs: image
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
