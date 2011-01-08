#!/bin/bash

# Copyright (c) 2006 The tyndur Project. All rights reserved.
# 
# This code is derived from software contributed to the tyndur Project
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
#     This product includes software developed by the tyndur Project
#     and its contributors.
# 4. Neither the name of the tyndur Project nor the names of its
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
LIBDIRS="$2"


if [ -z $LOST_BUILDMK_ROOT ]; then
  export LOST_BUILDMK_ROOT="`pwd`"

  mkdir -p build/output/apps build/output/gz/apps build/output/gz/modules \
      build/output/gz/kernel build/output/modules build/output/kernel \
      build/images
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
        LIBDIRS="`pwd`/lib/prt0.o $LIBDIRS"
    fi
    if [ -f lib/crt0.c ]; then
        LIBDIRS="`pwd`/lib/crt0.o $LIBDIRS"
    fi
    LIBDIRS="`pwd`/lib/library.a $LIBDIRS"
fi

if [ -f "user-$LOST_ARCH.ld" ]; then
    export LDSCRIPT="-T `pwd`/user-$LOST_ARCH.ld"
fi

CC_FLAGS=
CC_FLAGS_APPEND=
if [ -f Makefile.conf ]; then
    source Makefile.conf
fi

if [ "$LOST_BUILDMK_ROOT" == "`pwd`" ]; then
  cat <<EOF > Makefile.local
BUILD_ROOT=$LOST_BUILDMK_ROOT/build/output
PPC_BINARY=$LOST_TOOLS_PPC
EOF

  # Ist grub legacy installiert?
  if [ -f $GRUB_STAGEDIR/stage1 ]; then
    echo GRUB_VERSION=grub1 >> Makefile.local
  else
    echo GRUB_VERSION=grub2 >> Makefile.local
  fi
fi

# Nur die Unterverzeichnisse nutzen den absoluten Pfad zu Makefile.local, um
# Änderungen am Makefile im Wurzelverzeichnis zu verhindern
if [ "$LOST_BUILDMK_ROOT" == "`pwd`" ]; then
  echo "-include Makefile.local" > Makefile
else
  echo "-include $LOST_BUILDMK_ROOT/Makefile.local" > Makefile
fi

cat <<EOF >> Makefile

CC=$LOST_TOOLS_GCC $CC_FLAGS $INCLUDES $CC_FLAGS_APPEND
CPP=$LOST_TOOLS_GPP $INCLUDES
PPC=\$(PPC_BINARY) -n -Cn -CX -Ttyndur $FPCINCLUDES -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 $NASMINCLUDES
ASM_BIN=nasm -fbin -O99

BUILD_DIR=`pwd | sed s#^$LOST_BUILDMK_ROOT#\$\(BUILD_ROOT\)#`

AS=$LOST_TOOLS_AS

all:
EOF

if [ "`pwd | grep '/src/kernel2'`" == "" ]; then
cat <<EOF >> Makefile
	\$(MAKE) --no-print-directory -s makefiles
	\$(MAKE) --no-print-directory -s subdirs
	\$(MAKE) --no-print-directory -s obj 
EOF
else
cat <<EOF >> Makefile
	\$(MAKE) --no-print-directory -s makefiles
	\$(MAKE) --no-print-directory -s obj
	\$(MAKE) --no-print-directory -s subdirs
EOF
fi

if [ -f Makefile.all ]; then
  echo -e \\tLOST_BUILDMK_ROOT="$LOST_BUILDMK_ROOT" LDSCRIPT="'$LDSCRIPT'" bash Makefile.all '$(BUILD_ROOT)' \'$LIBDIRS $LIB_GCC\' \'$LOST_BUILDMK_ROOT\' >> Makefile
fi

cat <<EOF >> Makefile

makefiles:
	if [ -f buildmk.sh ]; then bash ./buildmk.sh; fi

EOF

# Wenn eine .nobuildfiles mit eintraegen für alle Dateien die nicht gebaut
# werden sollen exisitiert, wird diese eingelesen
NOBUILDFILES=
if [ -f .nobuildfiles ]; then NOBUILDFILES="`cat .nobuildfiles`"; fi

# Dateien zusammensuchen,die im aktuellen Verzeichnis
# kompiliert werden müssen
FILES=
for file in *.c *.cpp *.asm *.pas *.S; do
  if [ "`echo \"$NOBUILDFILES\" | grep -e ^$file\$`" ]; then continue; fi
  if [ -f $file ]; then
    FILES="$FILES ${file%.*}.o"
  fi
done
PWDLINE=
if [ "$FILES" !=  "" ]; then
	PWDLINE="pwdline"
	echo pwdline: >> Makefile
    echo -e \\techo >> Makefile
    echo -e \\techo "'---- `pwd`'" >> Makefile
	echo >> Makefile
fi
echo obj: $PWDLINE $FILES >> Makefile


# Unterverzeichnisse von src rekursiv durchsuchen
echo >> Makefile
echo subdirs: >> Makefile
for file in lib rtl !(lib|rtl|arch) arch; do
  if [ -d "$file" ]; then
    if [ "$file" == "include" -o \( -f "$file/.nobuild" -a ! -f "$file/.ignorenobuild" \) ]; then
      echo Ueberspringe `pwd`/$file/Makefile
      continue
    fi
    if [ "`basename \`pwd\``" == "arch" ] && [ "$file" != "$LOST_ARCH" ]; then
        echo Ueberspringe `pwd`/$file/Makefile
        continue
    fi
    echo Erzeuge `pwd`/$file/Makefile
    echo -e "\\t\$(MAKE)" --no-print-directory -sC "$file" >> Makefile
#    echo -e \\tmake -C "$file" '| (grep -vE "^(make|if)" || true)' >> Makefile

    cd $file
    bash $LOST_BUILDMK_ROOT/buildmk.sh "$INCLUDES" "$LIBDIRS"
    cd ..
  fi
done

# Kompilieren der Quellcode-Dateien
for file in *.c *.cpp *.asm *.pas *.S; do
  if [ -f $file ]; then
    echo >> Makefile
    case $file in

    *.c)    echo ${file%.c}.o: $file >> Makefile
            echo -e \\techo "'CC   $file:  $CC'" >> Makefile
          
            echo -e \\t\$\(CC\) $file >> Makefile
            ;;

    *.cpp)  echo ${file%.cpp}.o: $file >> Makefile
            echo -e \\techo "'CPP  $file:  $CPP'" >> Makefile
          
            echo -e \\t\$\(CPP\) $file >> Makefile
            ;;


    *.asm)  echo ${file%.asm}.o: $file >> Makefile

            echo -e \\techo "'ASM  $file'" >> Makefile

            # Assemblieren
            if egrep -q '^org (0x)?[0-9A-Fa-f]+h?' $file; then
                echo -e \\t\$\(ASM_BIN\) -o ${file%.asm}.o $file >> Makefile
            else
                echo -e \\t\$\(ASM_ELF\) -o ${file%.asm}.o $file >> Makefile
            fi
            ;;

    *.pas)  echo ${file%.pas}.o: $file >> Makefile
     
            echo -e \\techo "'PPC  $file'" >> Makefile
            if [ "$file" == "system.pas" ]; then
                echo -e \\t\$\(PPC\) -Us -Fiinclude/inc/ -Fiinclude/i386/ -Fiinclude/common/ $file >> Makefile
            else
                echo -e \\t\$\(PPC\) $file >> Makefile
            fi
            ;;

    *.S)  echo ${file%.S}.o: $file >> Makefile
            echo -e \\techo "'AS  $file:  $CPP'" >> Makefile
          
            echo -e \\t\$\(AS\) -o ${file%.S}.o $file >> Makefile
            ;;
    esac
  fi
done

# Code für make clean
cat <<EOF >> Makefile

clean_objs:
	rm -f *.o *.a *.mod *.ppu link.res ppas.sh `basename \`pwd\``

softclean: clean_objs
	for file in *; do if [ -f "\$\$file/Makefile" -a \( ! -f "\$\$file/.nobuild" -o -f "\$\$file/.ignorenobuild" \) ]; then \$(MAKE) -sC "\$\$file" softclean; fi done

clean: clean_objs
	for file in *; do if [ -f "\$\$file/Makefile" -a \( ! -f "\$\$file/.nobuild" -o -f "\$\$file/.ignorenobuild" \) ]; then \$(MAKE) -sC "\$\$file" clean; rm "\$\$file/Makefile"; fi done
	rm -f Makefile.local

.SILENT: all makefiles subdirs obj lbuilds-env enable-pascal disable-pascal clean softclean clean_objs clean_root updateroot image-floppy image-hd test-qemu test-qemu-hd
EOF

if [ "$LOST_BUILDMK_ROOT" == "`pwd`" ]; then
cat <<EOF >> Makefile
.PHONY: lbuilds-env

# Code für lbuilds
lbuilds-env:
	if [ -d lbuilds ]; then cd lbuilds && git pull; else git clone git://git.tyndur.org/lbuilds.git lbuilds; fi
	touch lbuilds/.nobuild
	cd lbuilds && scripts/build_crosstools && scripts/update_env \`dirname \\\`pwd\\\`\` && ln -sf \`pwd\`/tmp/cross-fpc/fpc-2.4.0/rtl/ \`dirname \\\`pwd\\\`\`/src/modules/pas/lib/rtl/include
	make enable-pascal

enable-pascal:
	touch src/modules/pas/.ignorenobuild

disable-pascal:
	rm -f src/modules/pas/.ignorenobuild

clean: clean_root

clean_root:
	rm -rf build/images/* build/root/*
	rm -f build/output/apps/* build/output/modules/* build/output/kernel/* build/output/gz/*/*
	rm -f qemu-serial.log

updateroot: subdirs
	build/scripts/updateroot

image-floppy: updateroot
	build/scripts/image_floppy_\$(GRUB_VERSION)

image-hd: updateroot
	build/scripts/image_hd_\$(GRUB_VERSION)

image-cdrom: updateroot
	build/scripts/image_cdrom_\$(GRUB_VERSION)

test-qemu-floppy: image-floppy
	$LOST_TOOLS_QEMU -serial stdio -fda build/images/floppy.img -boot a -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-qemu-hd: image-hd
	$LOST_TOOLS_QEMU -serial stdio -hda build/images/hd.img -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-qemu-cdrom: image-cdrom
	$LOST_TOOLS_QEMU -serial stdio -cdrom build/images/cdrom.img -boot d -net user -net nic,model=rtl8139 | tee qemu-serial.log

test-bochs: image-floppy
	bochs -f bochs.config

cscope:
	rm -f cscope.*
	find \`pwd\`/src -name '*.[ch]' > cscope.files
	cscope -bk

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
