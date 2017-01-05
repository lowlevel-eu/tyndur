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
      build/output/system build/images
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

if [ -d lib ] && [ ! -f lib/.libdirs_ignore ] ; then
    if [ -f lib/prt0.asm ]; then
        LIBDIRS="`pwd`/lib/prt0.o $LIBDIRS"
    fi
    if [ -f lib/crt0.c ]; then
        LIBDIRS="`pwd`/lib/crt0.o $LIBDIRS"
    fi
    if (grep library.so lib/Makefile.all > /dev/null 2>&1); then
        LIBDIRS="`pwd`/lib/library.so $LIBDIRS"
    else
        LIBDIRS="`pwd`/lib/library.a $LIBDIRS"
    fi
fi

if [ -f "user-$LOST_ARCH.ld" ]; then
    export LDSCRIPT="-T `pwd`/user-$LOST_ARCH.ld"
fi

CC_FLAGS=
if [ -f Makefile.conf ]; then
    source Makefile.conf
fi

if [ "$LOST_BUILDMK_ROOT" == "`pwd`" ]; then
  cat <<EOF > Makefile.local
BUILD_ROOT=$LOST_BUILDMK_ROOT/build/output
AS_BINARY=$LOST_TOOLS_AS
CC_BINARY=$LOST_TOOLS_GCC
CPP_BINARY=$LOST_TOOLS_GPP
PPC_BINARY=$LOST_TOOLS_PPC
LD_BINARY=$LOST_TOOLS_LD
QEMU_BINARY=$LOST_TOOLS_QEMU
EOF

  # Ist grub legacy installiert?
  if [ -f $LOST_GRUB_STAGESDIR/stage1 ]; then
    echo BOOTLOADER=grub1 >> Makefile.local
  elif [ -d $LOST_GRUB2_MODULESDIR ]; then
    echo BOOTLOADER=grub2 >> Makefile.local
  elif [ -d $LOST_SYSLINUX_DIR ]; then
    echo BOOTLOADER=syslinux >> Makefile.local
  else
    echo "Kein passender Bootloader gefunden"
    exit -1
  fi

  for dir in doc src; do
    cd $dir
    bash $LOST_BUILDMK_ROOT/buildmk.sh "$INCLUDES" "$LIBDIRS"
    cd ..
  done

  exit 0
fi

cat <<EOF > Makefile
-include $LOST_BUILDMK_ROOT/Makefile.local

CC=\$(CC_BINARY) $CC_FLAGS $INCLUDES $CC_FLAGS_APPEND
CPP=\$(CPP_BINARY) $INCLUDES
PPC=\$(PPC_BINARY) -n -Cn -CX -Ttyndur $FPCINCLUDES -Fu../lib/units -Fu../units

ASM_ELF=nasm -felf -O99 $NASMINCLUDES
ASM_BIN=nasm -fbin -O99

BUILD_DIR=`pwd | sed s#^$LOST_BUILDMK_ROOT#\$\(BUILD_ROOT\)#`

AS=\$(AS_BINARY)

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
    if [ -z "$ENABLE_SHARED" ]; then
        LOCAL_LIBDIRS="$(echo "$LIBDIRS" | sed -e 's/\.so/\.a/g')"
    else
        LOCAL_LIBDIRS="$LIBDIRS"
    fi
    echo -e \\tLOST_BUILDMK_ROOT="$LOST_BUILDMK_ROOT" LDSCRIPT="'$LDSCRIPT'" bash Makefile.all '$(BUILD_ROOT)' \'$LOCAL_LIBDIRS $LIB_GCC\' \'$LOST_BUILDMK_ROOT\' >> Makefile
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
    if [ "$file" == "include" -o \( -f "$file/.nobuild" -a ! -f "$file/.ignorenobuild" \) ] ||
       [ "`basename \`pwd\``" == "arch" ] && [ "$file" != "$LOST_ARCH" ]; then
      echo Überspringe `pwd`/$file/
      continue
    fi
#    echo Erzeuge `pwd`/$file/Makefile
    echo -e "\\t\$(MAKE)" --no-print-directory -sC "$file" >> Makefile
#    echo -e \\tmake -C "$file" '| (grep -vE "^(make|if)" || true)' >> Makefile

    cd $file
    bash $LOST_BUILDMK_ROOT/buildmk.sh "$INCLUDES" "$LIBDIRS" &
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
            echo -e \\techo "'AS   $file:  $AS'" >> Makefile

            echo -e \\t\$\(AS\) -o ${file%.S}.o $file >> Makefile
            ;;
    esac
  fi
done

# Code für make clean
cat <<EOF >> Makefile

clean_objs:
	rm -f *.o *.a *.so *.mod *.ppu link.res ppas.sh `basename \`pwd\``

softclean: clean_objs
	for file in *; do if [ -f "\$\$file/Makefile" -a \( ! -f "\$\$file/.nobuild" -o -f "\$\$file/.ignorenobuild" \) ]; then \$(MAKE) -sC "\$\$file" softclean; fi done

clean: clean_objs
	for file in *; do if [ -f "\$\$file/Makefile" -a \( ! -f "\$\$file/.nobuild" -o -f "\$\$file/.ignorenobuild" \) ]; then \$(MAKE) -sC "\$\$file" clean; rm "\$\$file/Makefile"; fi done
	rm -f Makefile.local

.SILENT: all makefiles subdirs obj lbuilds-env enable-pascal disable-pascal clean softclean clean_objs clean_root updateroot image-floppy image-hd test-qemu test-qemu-hd
EOF

# Auf alle Unterprozesse warten
wait
