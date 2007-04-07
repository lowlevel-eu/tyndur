#!/bin/bash

# Copyright (c) 2006 The LOST Project. All rights reserved.
# 
# This code is derived from software contributed to the LOST Project
# by Kevin Wolf.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#     This product includes software developed by the LOST Project
#     and its contributors.
# 4. Neither the name of the LOST Project nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR 
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

shopt -s extglob


INCLUDES=`echo $1 | sed -e 's#-I \?\([^ ]\+\)#-I ../\1#g'`
NASMINCLUDES=`echo $1 | sed -e 's#-I \?\([^ ]\+\)#-I ../\1/#g'`
LIBDIRS=`echo $2 | sed -e 's#lib/#../lib/#g'`

if [ -d include ]; then
  INCLUDES="$INCLUDES -I include"
  NASMINCLUDES="$NASMINCLUDES -I include/"
fi
FPCINCLUDES=`echo $INCLUDES | sed -e 's#-I \?\([^ ]\+\)#-Fi\1#g'`

if [ -d lib ]; then
    if [ -f lib/prt0.asm ]; then
        LIBDIRS="lib/prt0.o $LIBDIRS"
    fi
    LIBDIRS="lib/library.a $LIBDIRS"
fi

if [ -z $LOST_BUILDMK_ROOT ]; then
  export LOST_BUILDMK_ROOT="`pwd`"
fi


cat <<EOF > Makefile
CC="$COMPILER_PREFIX"gcc -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 $INCLUDES
CPP="$COMPILER_PREFIX"g++ -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0 $INCLUDES
PPC=fpc -Cn -CX -Tlost $FPCINCLUDES

ASM_ELF=nasm -felf -O99 $NASMINCLUDES
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=$LOST_BUILDMK_ROOT/build
BUILD_DIR=`pwd | sed s#^$LOST_BUILDMK_ROOT#\$\(BUILD_ROOT\)#`


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 
EOF

if [ -f Makefile.all ]; then
  echo -e \\tsh Makefile.all '$(BUILD_ROOT)' \'$LIBDIRS\' >> Makefile
fi

cat <<EOF >> Makefile

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

EOF


# Dateien zusammensuchen,die im aktuellen Verzeichnis
# kompiliert werden müssen
FILES=
for file in *.c *.cpp *.asm *.pas; do
  if [ -f $file ]; then
    FILES="$FILES `basename \`basename \\\`basename \\\\\\\`basename $file .pas\\\\\\\` .cpp\\\` .c\` .asm`.o"
  fi
done
echo obj: $FILES >> Makefile


# Unterverzeichnisse von src rekursiv durchsuchen
echo >> Makefile
echo subdirs: >> Makefile
for file in lib !(lib); do
  if [ -d "$file" ]; then
    if [ "$file" == "include" -o -f "$file/.nobuild" ]; then
      echo Ueberspringe `pwd`/$file/Makefile
      continue
    fi
    echo Erzeuge `pwd`/$file/Makefile
    echo -e \\tmake --no-print-directory -sC "$file" >> Makefile
#    echo -e \\tmake -C "$file" '| (grep -vE "^(make|if)" || true)' >> Makefile

    cd $file
    sh $LOST_BUILDMK_ROOT/buildmk.sh "$INCLUDES" "$LIBDIRS"
    cd ..
  fi
done

# Kompilieren der Quellcode-Dateien
firstfileindir=1
for file in *.c *.cpp *.asm *.pas; do
  if [ "$firstfileindir" == 1 -a -f "$file" ]; then
    echo -e \\techo >> Makefile
    echo -e \\techo "'---- `pwd`'" >> Makefile
    firstfileindir=0
  fi

  if [ -f $file ]; then
    echo >> Makefile
    case $file in

    *.c)    echo `basename $file .c`.o: $file >> Makefile
            echo -e \\techo "'CC   $file:  $CC'" >> Makefile
          
            echo -e \\t\$\(CC\) $file >> Makefile
            ;;

    *.cpp)  echo `basename $file .cpp`.o: $file >> Makefile
            echo -e \\techo "'CPP  $file:  $CPP'" >> Makefile
          
            echo -e \\t\$\(CPP\) $file >> Makefile
            ;;


    *.asm)  echo `basename $file .asm`.o: $file >> Makefile

            echo -e \\techo "'ASM  $file'" >> Makefile

            # Assemblieren
            if egrep -q '^org (0x)?[0-9A-Fa-f]+h?' $file; then
                echo -e \\t\$\(ASM_BIN\) -o `basename $file .asm`.o $file >> Makefile
            else
                echo -e \\t\$\(ASM_ELF\) -o `basename $file .asm`.o $file >> Makefile
            fi
            ;;

    *.pas)  echo `basename $file .pas`.o: $file >> Makefile
     
            echo -e \\techo "'PPC  $file'" >> Makefile
            if [ "$file" == "system.pas" ]; then
                echo -e \\t\$\(PPC\) -Us -Fiinclude/inc/ -Fiinclude/i386/ -Fiinclude/common/ $file >> Makefile
            else
                echo -e \\t\$\(PPC\) $file >> Makefile
            fi
            ;;

    esac
  fi
done


# Code für make clean
cat <<EOF >> Makefile

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "\$\$file/Makefile" ]; then make -sC "\$\$file" clean; rm "\$\$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean
EOF

if [ "$LOST_BUILDMK_ROOT" == "`pwd`" ]; then
cat <<EOF >> Makefile
image: all
	    dd if=/dev/zero of=build/lost.img bs=1024 count=1440
	    mkfs.msdos build/lost.img
	    mkdir -p build/mnt
	    mount -oloop build/lost.img build/mnt
	
	    mkdir build/mnt/grub
	    cp /boot/grub/stage? build/mnt/grub
	
	echo timeout 2 >> build/mnt/grub/menu.lst
	echo title LOST >> build/mnt/grub/menu.lst
	echo kernel /lost.krn.gz >> build/mnt/grub/menu.lst
	gzip -c build/lost.krn > build/lost.krn.gz

	for f in build/*.mod; do gzip -c \$\$f > \$\$f.gz; done
	echo module /init.mod.gz >> build/mnt/grub/menu.lst
	for i in build/*.mod.gz; do [ "\`basename \$\$i\`" != "init.mod.gz" ] && echo module /\`basename \$\$i\` >> build/mnt/grub/menu.lst; done

	echo boot >> build/mnt/grub/menu.lst
	
	cp build/*.krn.gz build/mnt
	cp build/*.mod.gz build/mnt
	umount build/mnt
	rmdir build/mnt
	
	echo device '(fd0)' build/lost.img > build/grub.txt
	echo root '(fd0)' >> build/grub.txt
	echo setup '(fd0)' >> build/grub.txt
	grub --batch < build/grub.txt
	rm build/grub.txt

image-update: all
	mkdir -p build/mnt
	    mount -oloop build/lost.img build/mnt
	    rm -rf build/mnt/grub/menu.lst
	    
	
	echo timeout 2 >> build/mnt/grub/menu.lst
	echo title LOST >> build/mnt/grub/menu.lst
	echo kernel /lost.krn.gz >> build/mnt/grub/menu.lst
	gzip -c build/lost.krn > build/lost.krn.gz

	for f in build/*.mod; do gzip -c \$\$f > \$\$f.gz; done
	echo module /init.mod.gz >> build/mnt/grub/menu.lst
	for i in build/*.mod.gz; do [ "\`basename \$\$i\`" != "init.mod.gz" ] && echo module /\`basename \$\$i\` >> build/mnt/grub/menu.lst; done

	echo boot >> build/mnt/grub/menu.lst
	
	cp build/*.krn.gz build/mnt
	cp build/*.mod.gz build/mnt
	umount build/mnt
	rm -rf build/mnt

test-qemu: image
	qemu -serial stdio -fda build/lost.img -boot a

test-bochs: image
	bochs -f bochs.config

menuconfig:
	python config.py

documentation:
	test -d doc/doxygen || mkdir doc/doxygen
	test -d doc/doxygen/modules || mkdir doc/doxygen/modules
	test -d doc/doxygen/kernel || mkdir doc/doxygen/kernel
	rm -R doc/doxygen/modules/*
	rm -R doc/doxygen/kernel/*
	doxygen doxyfile_modules
	doxygen doxyfile_kernel

bugreport:
	mkdir bugreport
	gcc -v 2> bugreport/gcc.version
	nasm -v > bugreport/nasm.version
	ld -v > bugreport/ld.version
	
	cp build/*.mod bugreport/
	cp build/*.krn bugreport/
	
	tar cjf bugreport.tar.bz2 bugreport
	rm -rf bugreport
EOF
fi
