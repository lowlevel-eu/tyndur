CC=gcc -c -fno-leading-underscore  -I /home/kevin/progzeug/os/lost-nt/trunk/include
ASM_WIN_TRANSLATOR=/home/kevin/progzeug/os/lost-nt/trunk/win-tools/bin/AsmForWin.exe
ASM_ELF=nasm -felf -O99
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/kevin/progzeug/os/lost-nt/trunk/build
BUILD_DIR=$(BUILD_ROOT)

all: makefiles obj subdirs

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj:

subdirs:
	make -C build
	make -C doc
	make -C include
	make -C kernel
	make -C load_process
	make -C modules
	make -C win-tools

clean:  
	rm -f *.o
	for file in *; do if [ -f $$file/Makefile ]; then make -C $$file clean; rm $$file/Makefile; fi done
