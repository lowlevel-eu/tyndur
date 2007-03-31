CC="/usr/local/i686-pc-elf/bin/i686-pc-elf-"gcc -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 -I ../../include -I ../include -I include
CPP="/usr/local/i686-pc-elf/bin/i686-pc-elf-"g++ -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -O0 -I ../../include -I ../include -I include

ASM_ELF=nasm -felf -O99 -I ../../include/ -I ../include/ -I include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/opt/server/data/home/toni/arbeitskopien/lost/lost/trunk/build
BUILD_DIR=$(BUILD_ROOT)/src/modules/cpp


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj:

subdirs:
	make --no-print-directory -sC lib
	make --no-print-directory -sC cpptest
	make --no-print-directory -sC include

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean
