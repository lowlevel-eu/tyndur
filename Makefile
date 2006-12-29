CC=gcc -c -fno-leading-underscore -Wall -fno-strict-aliasing -O2 
ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=.
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

.SILENT: all makefiles subdirs obj clean
image: all
	    dd if=/dev/zero of=build/lost.img bs=1024 count=1440
	    mkfs.msdos build/lost.img
	    mkdir -p build/mnt
	    mount -oloop build/lost.img build/mnt
	
	    mkdir build/mnt/grub
	    cp /boot/grub/stage? build/mnt/grub
	
	echo timeout 2 >> build/mnt/grub/menu.lst
	echo title LOST >> build/mnt/grub/menu.lst
	echo kernel /lost.krn >> build/mnt/grub/menu.lst
	for i in build/*.mod; do echo module /`basename $$i` >> build/mnt/grub/menu.lst; done
	echo boot >> build/mnt/grub/menu.lst
	
	cp build/*.krn build/mnt
	cp build/*.mod build/mnt
	umount build/mnt
	rmdir build/mnt
	
	echo device '(fd0)' build/lost.img > build/grub.txt
	echo root '(fd0)' >> build/grub.txt
	echo setup '(fd0)' >> build/grub.txt
	grub --batch < build/grub.txt
	rm build/grub.txt

test-qemu: image
	qemu -fda build/lost.img -boot a

test-bochs: image
	bochs -f bochs.config
