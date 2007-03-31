CC=""gcc -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 -I ../../include -I ../include
CPP=""g++ -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 -I ../../include -I ../include

ASM_ELF=nasm -felf -O99 -I ../../include/ -I ../include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/damian/Programmierzeug/OS/lost/trunk/build
BUILD_DIR=$(BUILD_ROOT)/src/modules/sysinfo


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 
	sh Makefile.all $(BUILD_ROOT) '../lib/library.a ../../lib/library.a'

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj: cpuid.o

subdirs:
	echo
	echo '---- /home/damian/Programmierzeug/OS/lost/trunk/src/modules/sysinfo'

cpuid.o: cpuid.c
	echo 'CC   cpuid.c:  '
	$(CC) cpuid.c

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean
