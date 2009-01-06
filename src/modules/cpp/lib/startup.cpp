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

extern "C"
{
    #include "init.h"
    #include "stdlib.h"
    #include "stdio.h"
    #include "rpc.h"
    #include "syscall.h"

    void __libcpp_initialize();
    int __libcpp_startup();
    void __libcpp_finalize(int result);
    
    ///Pointer auf die Konstruktorenliste (im Linkerskript definiert)
    extern void (*__libcpp_constructors_start)();
    
    ///Pointer auf die Destruktorenliste (im Linkerskript definiert)
    extern void (*__libcpp_destructors_start)();
}
extern int main(int argc, char* argv[]);


/**
 * Ruft die Konstruktoren der globalen objekte auf.
 */
void __libcpp_initialize()
{
    init_memory_manager();
    init_messaging();

    void (**constructor_ptr)() = &__libcpp_constructors_start;
    while(*((dword*)constructor_ptr) != 0)
    {
        //Konstruktor ausfuehren
        (*constructor_ptr)();

        //Pointer auf naechsten Konstruktor setzen
        constructor_ptr++;
    }
}

/**
 * Ruft die Destruktoren der globalen objekte auf.
 */
void __libcpp_finalize(int result)
{
    void (**destructor_ptr)() = &__libcpp_destructors_start;
    while(*((dword*)destructor_ptr) != 0)
    {
        //Destruktor ausfuehren
        (*destructor_ptr)();

        //Pointer auf naechsten Destruktor setzen
        destructor_ptr++;
    }

    init_process_exit(result);
    destroy_process();
}

/**
 *
 */
int __libcpp_startup()
{
    
    return main(0, (char**)NULL);
}

