CC="/usr/local/i686-pc-elf/bin/i686-pc-elf-"gcc -g -c -fno-leading-underscore -fno-omit-frame-pointer -Wall -fno-strict-aliasing -O0 -I ../../include
ASM_ELF=nasm -felf -O99 -I ../../include/
ASM_BIN=nasm -fbin -O99

BUILD_ROOT=/home/toni/arbeitskopien/lost/lost/trunk/build
BUILD_DIR=$(BUILD_ROOT)/src/lib/string


all: 
	make --no-print-directory -s makefiles
	make --no-print-directory -s subdirs
	make --no-print-directory -s obj 

makefiles:
	if [ -f buildmk.sh ]; then sh ./buildmk.sh; fi

obj: bcmp.o bcopy.o bzero.o ffs.o ffsl.o fls.o flsl.o index.o memccpy.o memchr.o memcmp.o memcpy.o memmem.o memmove.o memset.o rindex.o stpcpy.o strcasecmp.o strcasestr.o strcat.o strchr.o strcmp.o strcpy.o strcspn.o strdup.o strlcat.o strlcpy.o strlen.o strncat.o strncmp.o strncpy.o strnstr.o strpbrk.o strrchr.o strsep.o strspn.o strstr.o strtok.o

subdirs:
	echo
	echo '---- /home/toni/arbeitskopien/lost/lost/trunk/src/lib/string'

bcmp.o: bcmp.c
	echo 'CC   bcmp.c:  '
	$(CC) bcmp.c

bcopy.o: bcopy.c
	echo 'CC   bcopy.c:  '
	$(CC) bcopy.c

bzero.o: bzero.c
	echo 'CC   bzero.c:  '
	$(CC) bzero.c

ffs.o: ffs.c
	echo 'CC   ffs.c:  '
	$(CC) ffs.c

ffsl.o: ffsl.c
	echo 'CC   ffsl.c:  '
	$(CC) ffsl.c

fls.o: fls.c
	echo 'CC   fls.c:  '
	$(CC) fls.c

flsl.o: flsl.c
	echo 'CC   flsl.c:  '
	$(CC) flsl.c

index.o: index.c
	echo 'CC   index.c:  '
	$(CC) index.c

memccpy.o: memccpy.c
	echo 'CC   memccpy.c:  '
	$(CC) memccpy.c

memchr.o: memchr.c
	echo 'CC   memchr.c:  '
	$(CC) memchr.c

memcmp.o: memcmp.c
	echo 'CC   memcmp.c:  '
	$(CC) memcmp.c

memcpy.o: memcpy.c
	echo 'CC   memcpy.c:  '
	$(CC) memcpy.c

memmem.o: memmem.c
	echo 'CC   memmem.c:  '
	$(CC) memmem.c

memmove.o: memmove.c
	echo 'CC   memmove.c:  '
	$(CC) memmove.c

memset.o: memset.c
	echo 'CC   memset.c:  '
	$(CC) memset.c

rindex.o: rindex.c
	echo 'CC   rindex.c:  '
	$(CC) rindex.c

stpcpy.o: stpcpy.c
	echo 'CC   stpcpy.c:  '
	$(CC) stpcpy.c

strcasecmp.o: strcasecmp.c
	echo 'CC   strcasecmp.c:  '
	$(CC) strcasecmp.c

strcasestr.o: strcasestr.c
	echo 'CC   strcasestr.c:  '
	$(CC) strcasestr.c

strcat.o: strcat.c
	echo 'CC   strcat.c:  '
	$(CC) strcat.c

strchr.o: strchr.c
	echo 'CC   strchr.c:  '
	$(CC) strchr.c

strcmp.o: strcmp.c
	echo 'CC   strcmp.c:  '
	$(CC) strcmp.c

strcpy.o: strcpy.c
	echo 'CC   strcpy.c:  '
	$(CC) strcpy.c

strcspn.o: strcspn.c
	echo 'CC   strcspn.c:  '
	$(CC) strcspn.c

strdup.o: strdup.c
	echo 'CC   strdup.c:  '
	$(CC) strdup.c

strlcat.o: strlcat.c
	echo 'CC   strlcat.c:  '
	$(CC) strlcat.c

strlcpy.o: strlcpy.c
	echo 'CC   strlcpy.c:  '
	$(CC) strlcpy.c

strlen.o: strlen.c
	echo 'CC   strlen.c:  '
	$(CC) strlen.c

strncat.o: strncat.c
	echo 'CC   strncat.c:  '
	$(CC) strncat.c

strncmp.o: strncmp.c
	echo 'CC   strncmp.c:  '
	$(CC) strncmp.c

strncpy.o: strncpy.c
	echo 'CC   strncpy.c:  '
	$(CC) strncpy.c

strnstr.o: strnstr.c
	echo 'CC   strnstr.c:  '
	$(CC) strnstr.c

strpbrk.o: strpbrk.c
	echo 'CC   strpbrk.c:  '
	$(CC) strpbrk.c

strrchr.o: strrchr.c
	echo 'CC   strrchr.c:  '
	$(CC) strrchr.c

strsep.o: strsep.c
	echo 'CC   strsep.c:  '
	$(CC) strsep.c

strspn.o: strspn.c
	echo 'CC   strspn.c:  '
	$(CC) strspn.c

strstr.o: strstr.c
	echo 'CC   strstr.c:  '
	$(CC) strstr.c

strtok.o: strtok.c
	echo 'CC   strtok.c:  '
	$(CC) strtok.c

clean:
	rm -f *.o *.a
	for file in *; do if [ -f "$$file/Makefile" ]; then make -sC "$$file" clean; rm "$$file/Makefile"; fi done  

.SILENT: all makefiles subdirs obj clean
