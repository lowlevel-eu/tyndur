/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */  

#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>
#include <stddef.h>
#include <lost/config.h>


#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define EOF -1

#define _IONBF 1
#define _IOFBF 2
#define _IOLBF 3

#define FILENAME_MAX 4096
#define BUFSIZ 65536

#define P_tmpdir "file:/tmp"

#ifndef __FILE_DEFINED
#define __FILE_DEFINED
typedef struct lostio_internal_file FILE;
#endif

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#ifdef __cplusplus
extern "C" {
#endif

int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char* str);
void perror(const char* message);

int getc(FILE* io_res);
int getchar(void);
int ungetc(int c, FILE* io_res);
char* gets(char* dest);


int printf(const char * format, ...);
int sprintf(char * buffer, const char * format, ...);
int snprintf(char * buffer, size_t size, const char * format, ...);
int fprintf(FILE * fp, const char * format, ...);
int asprintf(char ** buffer, const char * format, ...);

int vprintf(const char * format, va_list);
int vsprintf(char * buffer, const char * format, va_list);
int vsnprintf(char * buffer, size_t size, const char * format, va_list);
int vfprintf(FILE * fp, const char * format, va_list);
int vasprintf(char ** buffer, const char * format, va_list);


int scanf(const char* fmt, ...);
int sscanf(const char* input, const char* fmt, ...);
int fscanf(FILE* f, const char* fmt, ...);

int vscanf(const char* fmt, va_list ap);
int vsscanf(const char* input, const char* fmt, va_list ap);
int vfscanf(FILE* f, const char* fmt, va_list ap);


//Dateihandling
FILE* fopen(const char* filename, const char* mode);
FILE* fdopen(int fd, const char* mode);
FILE* freopen(const char* filename, const char* mode, FILE* stream);
int fclose(FILE* io_res);
FILE* tmpfile(void);

size_t fread(void* dest, size_t blocksize, size_t blockcount, FILE* io_res);
int fgetc(FILE* io_res);
char* fgets(char* dest, int length, FILE *io_res);

size_t fwrite(const void* src, size_t blocksize, size_t blockcount, FILE* io_res);
int fputc(int c, FILE *io_res);
int fputs(const char *str, FILE *io_res);

int fseek(FILE* io_res, long int offset, int origin);
long ftell(FILE* io_res);

int feof(FILE* io_res);
int ferror(FILE* io_res);
void clearerr(FILE* io_res);
void rewind(FILE* io_res);

int fflush(FILE* io_res);
int fpurge(FILE* io_res);
int setvbuf(FILE* io_res, char* buffer, int mode, size_t size);
int setbuf(FILE* io_res, char* buffer);
int setbuffer(FILE* io_res, char* buffer, size_t size);
int setlinebuf(FILE* io_res);

int remove(const char* filename);
#ifndef CONFIG_LIBC_NO_STUBS
/// Datei verschieben
int rename(const char *oldpath, const char *newpath);
#endif


/// Den Filedeskriptor holen, um diese Datei mit den Unix-Dateifunktionen
/// benutzen zu koennen
int fileno(FILE* io_res);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
