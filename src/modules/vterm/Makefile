CC=""gcc -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 -I ../../include -I ../include
CPP=""g++ -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 -I ../../include -I ../include
PPC=fpc -Cn -CX -Tlost -Fi../../include -Fi../include

ASM_ELF=nasm -felf -O99 -I ../../include/ -I ../include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/dunklermeuchler/lost-dev/svn/trunk/build
BUILD_DIR=$(BUILD_ROOT)/src/modules/vterm


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 
	sh Makefile.all $(BUILD_ROOT) '../lib/library.a ../../lib/library.a'

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj: vterm.o

subdirs:
	echo
	echo '---- /home/dunklermeuchler/lost-dev/svn/trunk/src/modules/vterm'

vterm.o: vterm.c
	echo 'CC   vterm.c:  '
	$(CC) vterm.c

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean
