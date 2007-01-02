CC=gcc -c -fno-leading-underscore -Wall -fno-strict-aliasing -O2 -I ../../include -I ../include
ASM_ELF=nasm -felf -O99 -I ../../include/ -I ../include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=x:/lost/svn/trunk/build
BUILD_DIR=$(BUILD_ROOT)/src/modules/init_test


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 
	sh Makefile.all $(BUILD_ROOT) '../lib/library.a ../../lib/library.a'

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj: init_test.o

subdirs:
	echo
	echo '---- x:/lost/svn/trunk/src/modules/init_test'

init_test.o: init_test.c
	echo 'CC   init_test.c'
	$(CC) init_test.c

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean
