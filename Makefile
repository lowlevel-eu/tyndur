CC=""gcc -m32 -g -c -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 -I . -I- 
CPP=""g++ -m32 -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 
PPC=fpc -n -Cn -CX -Tlost  -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=./build
BUILD_DIR=$(BUILD_ROOT)


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj:

subdirs:
	make --no-print-directory -sC build
	make --no-print-directory -sC doc
	make --no-print-directory -sC src

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean image
internal_image:
	if ! [ -f build/lost.img ]; then         PATH=$$PATH:/sbin:/usr/sbin:/usr/local/sbin ;		dd if=/dev/zero of=build/lost.img bs=1024 count=1440 ;		mkfs.msdos build/lost.img ;		mmd -i build/lost.img ::/grub ;		mcopy -i build/lost.img /boot/grub/stage? ::/grub ;		mcopy -i build/lost.img build/default.cfg ::/grub/menu.lst ;		echo device '(fd0)' build/lost.img > build/grub.txt ;		echo root '(fd0)' >> build/grub.txt ;		echo setup '(fd0)' >> build/grub.txt ;        grub --batch < build/grub.txt ;		rm build/grub.txt ;	fi
	
	for i in build/*.krn; do 		gzip -c $$i > build/`basename $$i .krn`.kgz ;	done
	
	for i in build/*.mod; do 		gzip -c $$i > build/`basename $$i .mod`.mgz ;	done

	mcopy -D o -i build/lost.img build/default.cfg ::/grub/menu.lst
	mcopy -D o -i build/lost.img build/*.kgz ::/
	mcopy -D o -i build/lost.img build/*.mgz ::/
	mcopy -D o -i build/lost.img build/apps ::/

image:
	make --no-print-directory -s all
	make --no-print-directory -s internal_image 

simage:
	make --no-print-directory -s subdirs
	make --no-print-directory -s internal_image 

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
