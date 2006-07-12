CC=gcc -c -fno-leading-underscore 
ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/c/lostni/build
BUILD_DIR=$(BUILD_ROOT)


all: makefiles obj subdirs

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj:

subdirs:
	make -C build || true
	make -C src || true

clean:
	rm -f *.o
	for file in *; do if [ -f $$file/Makefile ]; then make -C $$file clean; rm $$file/Makefile; fi done
