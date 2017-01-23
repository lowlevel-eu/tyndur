# Ab hier anpassen
COMPILER_PREFIX_AMD64=
COMPILER_PREFIX_I386=
LOST_TOOLS_PPC=$LOST_BUILDMK_ROOT/lbuilds/env/bin/fpc
LOST_GRUB_STAGESDIR=/boot/grub
LOST_TOOLS_GRUB=grub
LOST_TOOLS_MKE2FS=/sbin/mke2fs
LOST_TOOLS_QEMU=qemu-system-x86_64

if [ -d /usr/lib/grub2/i386-pc ]; then
    LOST_GRUB2_MODULESDIR=/usr/lib/grub2/i386-pc
else
    LOST_GRUB2_MODULESDIR=/usr/lib/grub/i386-pc
fi

if [ -d /usr/lib/syslinux ]; then
    LOST_SYSLINUX_DIR=/usr/lib/syslinux
else
    LOST_SYSLINUX_DIR=/usr/share/syslinux
fi

if [ -d ${LOST_SYSLINUX_DIR}/bios ]; then
    LOST_SYSLINUX_DIR=${LOST_SYSLINUX_DIR}/bios
fi

[ -f  $LOST_BUILDMK_ROOT/myconf.sh ] && source  $LOST_BUILDMK_ROOT/myconf.sh

# Ab hier nicht mehr anpassen
CONFIG_ARCH=`grep 'define CONFIG_ARCH' $LOST_BUILDMK_ROOT/src/include/lost/config.h | awk '{ print $3 }'`
if [ "$CONFIG_ARCH" == "ARCH_AMD64" ]; then
    LOST_ARCH=amd64
else
    LOST_ARCH=i386
fi

if [ "$LOST_ARCH" == "amd64" ]; then
    COMPILER_PREFIX=$COMPILER_PREFIX_AMD64
    
    LOST_TOOLS_AR=$COMPILER_PREFIX"ar"
    LOST_TOOLS_AS=$COMPILER_PREFIX"as -64"
    LOST_TOOLS_LD=$COMPILER_PREFIX"ld -m elf_x86_64"
    LOST_TOOLS_GCC=$COMPILER_PREFIX"gcc -m64 -g -c -fno-stack-protector -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -fno-pic -O0 -std=gnu11 -I . -iquote ."
    LOST_TOOLS_GPP=$COMPILER_PREFIX"g++ -m64 -g -c -fno-stack-protector -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -fno-pic -O0"
    LOST_TOOLS_OBJCOPY=$COMPILER_PREFIX"objcopy -B i386:x86-64"
    LOST_TOOLS_STRIP=$COMPILER_PREFIX"strip -F elf64-x86-64"
else
    COMPILER_PREFIX=$COMPILER_PREFIX_I386
    
    LOST_TOOLS_AR=$COMPILER_PREFIX"ar"
    LOST_TOOLS_AS=$COMPILER_PREFIX"as -32"
    LOST_TOOLS_LD=$COMPILER_PREFIX"ld -m elf_i386"
    LOST_TOOLS_GCC=$COMPILER_PREFIX"gcc -m32 -g -c -fno-stack-protector -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -Werror -Wstrict-prototypes -fno-strict-aliasing -fno-pic -std=gnu11 -O2 -fno-builtin -I ."
    LOST_TOOLS_GPP=$COMPILER_PREFIX"g++ -m32 -g -c -fno-stack-protector -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -fno-pic -O0"
    LOST_TOOLS_OBJCOPY=$COMPILER_PREFIX"objcopy -B i386:i386"
    LOST_TOOLS_STRIP=$COMPILER_PREFIX"strip -F elf32-i386"
fi

#Libgcc bei LD standardmaessig mitlinken
if [ -z $LIB_GCC ]; then
    LIB_GCC="`$LOST_TOOLS_GCC -print-libgcc-file-name`"
fi

