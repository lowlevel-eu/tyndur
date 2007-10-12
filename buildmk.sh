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

if [ -z $LOST_BUILDMK_ROOT ]; then
  export LOST_BUILDMK_ROOT="`pwd`"
fi

source $LOST_BUILDMK_ROOT/config.sh



if [ -d include ]; then
  INCLUDES="-I include $INCLUDES"
  NASMINCLUDES="-I include/ $NASMINCLUDES"

    if [ -d include/arch/$LOST_ARCH ]; then
        INCLUDES="-I include/arch/$LOST_ARCH $INCLUDES"
    fi
fi
FPCINCLUDES=`echo $INCLUDES | sed -e 's#-I \?\([^ ]\+\)#-Fi\1#g'`

if [ -d lib ]; then
    if [ -f lib/prt0.asm ]; then
        LIBDIRS="lib/prt0.o $LIBDIRS"
    fi
    LIBDIRS="lib/library.a $LIBDIRS"
fi

CC_FLAGS=
if [ -f Makefile.conf ]; then
    source Makefile.conf
fi

cat <<EOF > Makefile
CC=$LOST_TOOLS_GCC $CC_FLAGS $INCLUDES
CPP=$LOST_TOOLS_GPP $INCLUDES
PPC=fpc -n -Cn -CX -Tlost $FPCINCLUDES -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 $NASMINCLUDES
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=$LOST_BUILDMK_ROOT/build
BUILD_DIR=`pwd | sed s#^$LOST_BUILDMK_ROOT#\$\(BUILD_ROOT\)#`

AS=$LOST_TOOLS_AS

all:
EOF

if [ "`pwd | grep '/src/kernel2'`" == "" ]; then
cat <<EOF >> Makefile
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 
EOF
else
cat <<EOF >> Makefile
	make --no-print-directory -s makefiles
	make --no-print-directory -s obj
	make --no-print-directory -s subdirs
EOF
fi

if [ -f Makefile.all ]; then
  echo -e \\tLOST_BUILDMK_ROOT="$LOST_BUILDMK_ROOT" sh Makefile.all '$(BUILD_ROOT)' \'$LIBDIRS $LIB_GCC\' \'$LOST_BUILDMK_ROOT\' >> Makefile
fi

cat <<EOF >> Makefile

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

EOF

# Wenn eine .nobuildfiles mit eintraegen fuer alle Dateien die nicht gebaut
# werden sollen exisitiert, wird diese eingelesen
NOBUILDFILES=
if [ -f .nobuildfiles ]; then NOBUILDFILES="`cat .nobuildfiles`"; fi

# Dateien zusammensuchen,die im aktuellen Verzeichnis
# kompiliert werden müssen
FILES=
for file in *.c *.cpp *.asm *.pas *.S; do
  if [ "`echo \"$NOBUILDFILES\" | grep -e ^$file\$`" ]; then continue; fi
  if [ -f $file ]; then
    FILES="$FILES `basename \`basename \\\`basename \\\\\\\`basename \\\\\\\\\\\\\\\`basename $file .S\\\\\\\\\\\\\\\` .pas\\\\\\\` .cpp\\\` .c\` .asm`.o"
  fi
done
echo obj: $FILES >> Makefile


# Unterverzeichnisse von src rekursiv durchsuchen
echo >> Makefile
echo subdirs: >> Makefile
for file in lib rtl !(lib|rtl|arch) arch; do
  if [ -d "$file" ]; then
    if [ "$file" == "include" -o -f "$file/.nobuild" ]; then
      echo Ueberspringe `pwd`/$file/Makefile
      continue
    fi
    if [ "`basename \`pwd\``" == "arch" ] && [ "$file" != "$LOST_ARCH" ]; then
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
for file in *.c *.cpp *.asm *.pas *.S; do
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

    *.S)  echo `basename $file .S`.o: $file >> Makefile
            echo -e \\techo "'AS  $file:  $CPP'" >> Makefile
          
            echo -e \\t\$\(AS\) -o `basename $file .S`.o $file >> Makefile
            ;;
    esac
  fi
done


# Code für make clean
cat <<EOF >> Makefile

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "\$\$file/Makefile" -a ! -f "\$\$file/.nobuild" ]; then make -sC "\$\$file" clean; rm "\$\$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean image
EOF

if [ "$LOST_BUILDMK_ROOT" == "`pwd`" ]; then
cat <<EOF >> Makefile
internal_image:
	if ! [ -f build/lost.img ]; then \
        PATH=\$\$PATH:/sbin:/usr/sbin:/usr/local/sbin ;\
		dd if=/dev/zero of=build/lost.img bs=1024 count=1440 ;\
		mkfs.msdos build/lost.img ;\
		mmd -i build/lost.img ::/grub ;\
		mcopy -i build/lost.img /boot/grub/stage? ::/grub ;\
		mcopy -i build/lost.img build/default.cfg ::/grub/menu.lst ;\
		echo device '(fd0)' build/lost.img > build/grub.txt ;\
		echo root '(fd0)' >> build/grub.txt ;\
		echo setup '(fd0)' >> build/grub.txt ;\
        grub --batch < build/grub.txt ;\
		rm build/grub.txt ;\
	fi
	
	for i in build/*.krn; do \
		gzip -c \$\$i > build/\`basename \$\$i .krn\`.kgz ;\
	done
	
	for i in build/*.mod; do \
		gzip -c \$\$i > build/\`basename \$\$i .mod\`.mgz ;\
	done

	mcopy -D o -i build/lost.img build/default.cfg ::/grub/menu.lst
	mcopy -D o -i build/lost.img build/*.kgz ::/
	mcopy -D o -i build/lost.img build/*.mgz ::/
	mcopy -D o -i build/lost.img build/apps ::/
	mcopy -D o -i build/lost.img build/data ::/

image:
	make --no-print-directory -s all
	make --no-print-directory -s internal_image 

simage:
	make --no-print-directory -s subdirs
	make --no-print-directory -s internal_image 
	
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
