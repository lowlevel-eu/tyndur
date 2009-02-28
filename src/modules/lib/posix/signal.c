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

#include <signal.h>
#include <stdio.h>
#include <collections.h>
#include <rpc.h>
#include <syscall.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sleep.h>

/// Array mit Pointern auf die Signal-Handler
static sighandler_t signal_handlers [_SIGNO_MAX];
static bool initialized = FALSE;

/**
 * Signale initialisieren
 */
static void init_signals(void)
{
    int i;
    for (i = 0; i < _SIGNO_MAX; i++) {
        signal_handlers[i] = SIG_DFL;
    }

    initialized = TRUE;
}

/**
 * Ein Signal an den aktuellen Prozess senden
 *
 * @param signum Signalnummer
 *
 * @return 0 bei Erfolg, != 0 im Fehlerfall
 */
int raise(int signum)
{
    // Falls die Liste noch nicht initialisiert ist, wird das jetzt gemacht
    if (initialized == FALSE) {
        init_signals();
    }

    // Wenn das Signal Groesser ist als die Groesste gueltige wird sofort
    // abgebrochen.
    if (signum >= _SIGNO_MAX) {
        return -1;
    }

    sighandler_t handler = signal_handlers[signum];
    if (handler != NULL) {
        handler(signum);
    }
    return 0;
}

/**
 * Ein Signal an einen anderen Prozess senden
 *
 * @param pid Prozessnummer
 * @param signum Signalnummer
 *
 * @return 0 bei Erfolg, != 0 im Fehlerfall
 */
int kill(pid_t pid, int signum)
{
    // Ungueltige Signalnummer
    if ((RPC_SIGNALS_START + signum) > RPC_SIGNALS_END) {
        return -1;
    }

    send_message(pid, RPC_SIGNALS_START + signum, 0, 0, NULL);
    return 0;
}

/**
 * Standard-Handler fuer Signale
 *
 * @param signum Signalnummer
 */
void _signal_default_handler(int signum)
{
    switch (signum) {
        // Signale die den Prozess beenden
        case SIGQUIT:
        case SIGILL:
        case SIGTRAP:
        case SIGABRT:
        case SIGBUS:
        case SIGFPE:
        case SIGSEGV:
        case SIGHUP:
        case SIGINT:
        case SIGKILL:
        case SIGUSR1:
        case SIGUSR2:
        case SIGPIPE:
        case SIGALRM:
        case SIGTERM:
            _exit(EXIT_FAILURE);
            break;
        // TODO: Dinge wie Prozess anhalten und wieder starten
    }
}

/**
 * Handler fuer ein bestimmtes Signal aendern
 *
 * @param signum Nummer des Signals
 * @param handler Neuer Handler
 *
 * @return Vorheriger Signalhandler bei Erfolg, SIG_ERR im Fehlerfall
 */
sighandler_t signal(int signum, sighandler_t handler)
{
    // Initialisieren falls das noch nicht gemacht ist
    if (initialized == FALSE) {
        init_signals();
    }
    
    // Ungueltige Signale und SIGKILL koennen nicht gesetzt werden
    if ((signum >= _SIGNO_MAX) || (signum == SIGKILL)) {
        return SIG_ERR;
    }    

    // Alter Handler Speichern, weil er zurueck gegeben wird
    sighandler_t old_handler = signal_handlers[signum];
    
    // Neuer setzen
    signal_handlers[signum] = handler;

    return old_handler;
}


/**
 * Sigset initialisieren und/oder leeren.
 *
 * @param sigset Pointer auf sigset
 * 
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int sigemptyset(sigset_t* sigset)
{
    memset(sigset->bitmask, 0, _SIGNO_MAX/8);
    return 0;
}

/**
 * Einem sigset alle Signale hinzufuegen
 *
 * @param sigset Pointer auf sigset
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int sigfillset(sigset_t* sigset)
{
    memset(sigset->bitmask, 0xFF, _SIGNO_MAX/8);
    return 0;
}

/**
 * Einzelnes Signal zu einem sigset hinzufuegen
 *
 * @param sigset Pointer auf sigset
 * @param signum Signalnummer
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int sigaddset(sigset_t* sigset, int signum)
{
    // Signale groesser als die groesste moegliche Signalnummer werden hier
    // abgefangen.
    if (signum >= _SIGNO_MAX) {
        errno = EINVAL;
        return -1;
    }
    
    // Entsprechendes Bit in der Bitmaske setzen
    sigset->bitmask[signum / 8] |= 1 << (signum % 8);

    return 0;
}

/**
 * Einzelnes Signal aus einem sigset loeschen
 *
 * @param sigset Pointer auf sigset
 * @param signum Signalnummer
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
 int sigdelset(sigset_t* sigset, int signum)
{
    // Signale groesser als die groesste moegliche Signalnummer werden hier
    // abgefangen.
    if (signum >= _SIGNO_MAX) {
        errno = EINVAL;
        return -1;
    }
    
    // Entsprechendes Bit in der Bitmaske loeschen
    sigset->bitmask[signum / 8] &= ~(1 << (signum % 8));

    return 0;
}

/**
 * Pruefen ob ein Signal im sigset enthalten ist
 *
 * @param sigset Pointer auf sigset
 * @param signum Signalnummer
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int sigismember(const sigset_t *sigset, int signum)
{
    // Signale groesser als die groesste moegliche Signalnummer werden hier
    // abgefangen.
    if (signum >= _SIGNO_MAX) {
        errno = EINVAL;
        return -1;
    }
    
    // Pruefen ob das entsprechende Bit in der Bitmaske gesetzt ist.
    return ((sigset->bitmask[signum / 8] & (1 <<(signum % 8))) != 0);
}

/**
 * Callbackfunktion fuer alarm()
 */
static void do_alarm(void)
{
    raise(SIGALRM);
}

/**
 * Timer setzen, der beim ablaufen ein SIGALARM sendet
 */
long alarm(long seconds)
{
    static dword id = 0;

    if (id != 0) {
        timer_cancel(id);
    }

    id = timer_register(do_alarm, seconds * 1000000);

    // FIXME
    return 0;
}
