# Ab hier anpassen
COMPILER_PREFIX_AMD64=
COMPILER_PREFIX_I386=

# Ab hier nicht mehr anpassen
CONFIG_ARCH=`grep CONFIG_ARCH $LOST_BUILDMK_ROOT/src/include/config.h | awk '{ print $3 }'`
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
    LOST_TOOLS_GCC=$COMPILER_PREFIX"gcc -m64 -g -c -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 -I . -iquote ."
    LOST_TOOLS_GPP=$COMPILER_PREFIX"g++ -m64 -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0"
    LOST_TOOLS_OBJCOPY=$COMPILER_PREFIX"objcopy -B i386:x86-64"
    LOST_TOOLS_STRIP=$COMPILER_PREFIX"strip -F elf64-x86-64"
else
    COMPILER_PREFIX=$COMPILER_PREFIX_I386
    
    LOST_TOOLS_AR=$COMPILER_PREFIX"ar"
    LOST_TOOLS_AS=$COMPILER_PREFIX"as -32"
    LOST_TOOLS_LD=$COMPILER_PREFIX"ld -m elf_i386"
    LOST_TOOLS_GCC=$COMPILER_PREFIX"gcc -m32 -g -c -nostdinc -fno-leading-underscore -fno-omit-frame-pointer -Wall -Werror -fno-strict-aliasing -O2 -fno-builtin -I . -I-"
    LOST_TOOLS_GPP=$COMPILER_PREFIX"g++ -m32 -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-use-cxa-atexit -O0"
    LOST_TOOLS_OBJCOPY=$COMPILER_PREFIX"objcopy -B i386:i386"
    LOST_TOOLS_STRIP=$COMPILER_PREFIX"strip -F elf32-i386"
fi

#Libgcc bei LD standardmaessig mitlinken
if [ -z $LIB_GCC ]; then
    LIB_GCC="`$LOST_TOOLS_GCC -print-libgcc-file-name`"
fi

