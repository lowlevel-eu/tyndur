CC=gcc -m32 -g -c -fno-stack-protector -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -Werror -fno-strict-aliasing -O2 -fno-builtin -I . -I-  -I ../include -I ../../include/arch/i386 -I ../../include
CPP=g++ -m32 -g -c -fno-stack-protector -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 -I ../include -I ../../include/arch/i386 -I ../../include
PPC=fpc -g -n -Cn -CX -Tlost -Fi../include -Fi../../include/arch/i386 -Fi../../include -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 -I ../include/ -I ../../include/arch/i386/ -I ../../include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/kevin/progzeug/os/lost/trunk/build
BUILD_DIR=$(BUILD_ROOT)/src/modules/speaker

AS=as -32

all:
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 
	LOST_BUILDMK_ROOT=/home/kevin/progzeug/os/lost/trunk sh Makefile.all $(BUILD_ROOT) '../lib/library.a ../../lib/library.a /usr/lib/gcc/i486-linux-gnu/4.1.2/libgcc.a' '/home/kevin/progzeug/os/lost/trunk'

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj: main.o

subdirs:
	echo
	echo '---- /home/kevin/progzeug/os/lost/trunk/src/modules/speaker'

main.o: main.c
	echo 'CC   main.c:  '
	$(CC) main.c

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" -a ! -f "$$file/.nobuild" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean image
