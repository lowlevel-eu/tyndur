CC=gcc -m32 -g -c -fno-stack-protector -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -Werror -fno-strict-aliasing -O2 -fno-builtin -I .  -I ../../../include -I ../../../../include/arch/i386 -I ../../../../include 
CPP=g++ -m32 -g -c -fno-stack-protector -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 -I ../../../include -I ../../../../include/arch/i386 -I ../../../../include
PPC=/home/toni/lostfpc/build/fpcsrc/compiler/ppcross386 -n -Cn -CX -Tlost -Fi../../../include -Fi../../../../include/arch/i386 -Fi../../../../include -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 -I ../../../include/ -I ../../../../include/arch/i386/ -I ../../../../include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/toni/arbeitskopien/lost_svn/trunk/build/output
BUILD_DIR=$(BUILD_ROOT)/src/modules/lib/posix/regex

AS=as -32

all:
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 

makefiles:
	if [ -f buildmk.sh ]; then bash ./buildmk.sh; fi

obj: regcomp.o regerror.o regexec.o regfree.o

subdirs:
	echo
	echo '---- /home/toni/arbeitskopien/lost_svn/trunk/src/modules/lib/posix/regex'

regcomp.o: regcomp.c
	echo 'CC   regcomp.c:  '
	$(CC) regcomp.c

regerror.o: regerror.c
	echo 'CC   regerror.c:  '
	$(CC) regerror.c

regexec.o: regexec.c
	echo 'CC   regexec.c:  '
	$(CC) regexec.c

regfree.o: regfree.c
	echo 'CC   regfree.c:  '
	$(CC) regfree.c

clean:
	rm -f *.o *.a *.mod
	for file in *; do if [ -f "$$file/Makefile" -a \( ! -f "$$file/.nobuild" -o -f "$$file/.ignorenobuild" \) ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean clean_root updateroot image-floppy image-hd test-qemu test-qemu-hd
