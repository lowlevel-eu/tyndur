#!/bin/bash

INCLUDES=$@
if [ -d include ]; then
  INCLUDES="$INCLUDES -I `pwd`/include"
fi

if [ -z $LOST_BUILDMK_ROOT ]; then
  export LOST_BUILDMK_ROOT="`pwd`"
fi

cat <<EOF > Makefile
CC=gcc -c -fno-leading-underscore $INCLUDES
ASM_WIN_TRANSLATOR=$LOST_BUILDMK_ROOT/win-tools/bin/AsmForWin.exe
ASM_ELF=nasm -felf -O99
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=$LOST_BUILDMK_ROOT/build
BUILD_DIR=`pwd | sed s#^$LOST_BUILDMK_ROOT#\$\(BUILD_ROOT\)#`

all: makefiles obj subdirs
EOF


if [ -f Makefile.all ]; then
  echo -e \\t/bin/bash Makefile.all '$(BUILD_ROOT)' >> Makefile
fi

cat <<EOF >> Makefile

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

EOF


# Dateien zusammensuchen,die im aktuellen Verzeichnis
# kompiliert werden müssen
FILES=
for file in *.c *.asm; do
  if [ -f $file ]; then
    FILES="$FILES `basename \`basename $file .c\` .asm`.o"
  fi
done
echo obj: $FILES >> Makefile


# Unterverzeichnisse rekursiv durchsuchen
echo >> Makefile
echo subdirs: >> Makefile
for file in *; do
  if [ -d "$file" ]; then
    echo Erzeuge `pwd`/$file/Makefile
    echo -e \\tmake -C "$file" '|| true' >> Makefile

    cd $file
    sh $LOST_BUILDMK_ROOT/buildmk.sh $INCLUDES
    cd ..
  fi
done


# Kompilieren der Quellcode-Dateien
for file in *.c *.asm; do
  if [ -f $file ]; then
    echo >> Makefile
    case $file in

      *.c)    echo `basename $file .c`.o: $file >> Makefile
              echo -e \\t\$\(CC\) $file >> Makefile
              ;;

      *.asm)  echo `basename $file .asm`.o: $file >> Makefile

              # Unter Windows müssen zuerst Unterstriche eingefügt werden
              if [ "$WINDIR" != "" ]; then 
                echo -e \\t\$\(ASM_WIN_TRANSLATOR\) $file >> Makefile;
                asmfile="$file".tmp
              else
                asmfile="$file"
              fi
             
              # Assemblieren
              if egrep -q '^org (0x)?[0-9A-Fa-f]+h?' $file; then
                echo -e \\t\$\(ASM_BIN\) -o `basename $file .asm`.o $asmfile >> Makefile
              else
                echo -e \\t\$\(ASM_ELF\) -o `basename $file .asm`.o $asmfile >> Makefile
              fi

              # Temporäre Datei ggf. löschen
              if [ "$asmfile" != "$file" ]; then
                echo -e \\trm $asmfile >> Makefile
              fi
              ;;

    esac
  fi
done


# Code für make clean
cat <<EOF >> Makefile

clean:  
	rm -f *.o
	for file in *; do if [ -f \$\$file/Makefile ]; then make -C \$\$file clean; rm \$\$file/Makefile; fi done
EOF
