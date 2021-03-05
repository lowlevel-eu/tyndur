/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#include <stdint.h>
#include "syscall.h"
#include "syscallx.h"

/**
 * Prozessnummer des aktuellen Prozesses abfragen.
 *
 * @return Prozessnummer
 */
pid_t get_pid()
{
    pid_t pid;

    SYSCALL0(SYSCALL_PM_GET_PID, pid);

    return pid;
}


/**
 * Prozessnummer des Elternprozesses beim Kernel anfragen
 *
 * @return Prozessnummer
 */
pid_t get_parent_pid(pid_t pid)
{
    pid_t result;

    SYSCALL1(SYSCALL_PM_GET_PARENT_PID, pid, result);

    return result;
}


/**
 * Neuen Prozess erstellen. Dieser ist solange blockiert, bis er mit
 * unblock_task erloest wird. Waerend dieser Zeit kann der Elternprozess Mit
 * Hilfe von init_child_page Einzelne Seiten in den Adressraum des neuen
 * Prozesses mappen.
 *
 * @param initial_eip Der Einsprungspunkt an dem der Prozess seine Arbeit
 *                      beginnen soll.
 * @param uid Die Benutzernummer unter der der Neue Prozess laufen soll (FIXME:
 *              Sollte vom Elternprozess geerbt werden)
 * @param args Pointer auf die Kommandozeilen-Parameter
 * @param parent Prozessnummer des Elternprozesses, dem der neue Task
 *          untergeordenet werden soll, oder 0 fuer den aktuellen Prozess.
 *
 * @return Prozessnummer
 */
pid_t create_process(uint32_t initial_eip, uid_t uid, const char* args,
    pid_t parent)
{
    pid_t pid;

    SYSCALL4(SYSCALL_PM_CREATE_PROCESS, initial_eip, uid, args, parent, pid);

    return pid;
}


/**
 * Den aktuellen Prozess beenden. TODO: Sollte auch mit Kinderprozessen
 * moeglich sein.
 */
void destroy_process()
{
    int result;

    SYSCALL0(SYSCALL_PM_EXIT_PROCESS, result);
}


/**
 * Pointer auf die Kommandozeilen-Argumente vom Kernel holen. Diese werden
 * einfach als normaler String zurueckgegeben. Das Parsen ist sache des
 * Prozesses.
 *
 * @return Pointer auf den Argument-String
 */
char* get_cmdline()
{
    char* result;

    SYSCALL0(SYSCALL_PM_GET_CMDLINE, result);

    return result;
}
