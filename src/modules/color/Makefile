CC=gcc -c -fno-leading-underscore -I /home/damian/Desktop/lost_last/trunk/src/include/
ASM_ELF=nasm -felf -O99 -I /home/damian/Desktop/lost_last/trunk/src/include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/damian/Desktop/lost_last/trunk/build
BUILD_DIR=$(BUILD_ROOT)/src/modules/color


all: makefiles obj subdirs

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj:

subdirs:

clean:
	rm -f *.o
	for file in *; do if [ -f $$file/Makefile ]; then make -C $$file clean; rm $$file/Makefile; fi done
