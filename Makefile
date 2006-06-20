CC=gcc -c -fno-leading-underscore 
ASM_ELF=nasm -felf -O99 
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/c/lost/build
BUILD_DIR=$(BUILD_ROOT)


all: makefiles obj subdirs

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj:

subdirs:
	make -C build || true
	make -C doc || true
	make -C src || true
	make -C vfd || true
	make -C windows || true

clean:
	rm -f *.o
	for file in *; do if [ -f $$file/Makefile ]; then make -C $$file clean; rm $$file/Makefile; fi done
