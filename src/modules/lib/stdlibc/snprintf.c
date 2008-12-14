/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "types.h"

#include "jprintf.h"

struct snprintf_args
{
    char * buffer;
    size_t buflen;
    size_t bytes_written;
};

int snprintf_putc(struct snprintf_args * arg, char c)
{
    if(arg->buflen - 1 > arg->buflen) // integer underflow
    {
        return 1;
    }
    
    if(arg->bytes_written + 1 < arg->bytes_written) // integer overflow
    {
        return 1;
    }

    if(arg->bytes_written < arg->buflen - 1)
    {
        arg->buffer[arg->bytes_written++] = c;
        return 1;
    }
    else if(arg->bytes_written == arg->buflen - 1)
    {
        // kein platz mehr im puffer. Terminieren
        arg->buffer[arg->bytes_written++] = 0;
    }

    return 1;
}

int vsnprintf(char * buffer, size_t size, const char * format, va_list ap)
{
    struct snprintf_args args = { buffer, size, 0 };
    struct jprintf_args snprintf_handler = { (pfn_putc)&snprintf_putc, 0, (void*)&args };
    int retval;

	retval = jvprintf(&snprintf_handler, format, ap);
	if(retval >= 0)
	{
	    if(retval < size)
	    {
            buffer[retval] = 0;
	    }
	}
	
	return retval;
}

int snprintf(char * buffer, size_t size, const char * format, ...)
{
	va_list ap;
	int retval;

	va_start(ap, format);
	retval = vsnprintf(buffer, size, format, ap);
	va_end(ap);

	return retval;
}
