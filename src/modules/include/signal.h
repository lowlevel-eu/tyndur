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

#ifndef _SIGNAL_H_
#define _SIGNAL_H_
#include <sys/types.h>
#include <stdint.h>

typedef uint32_t sig_atomic_t;

// Signale

/// Verbindung zu Terminal Verloren
#define SIGHUP 1

/// Unterbrechung vom Benutzer angeforder (Ctrl + C)
#define SIGINT 2

/// TODO
#define SIGQUIT 3

/// Fehlerhafte Instruktion
#define SIGILL 4

/// Debug
#define SIGTRAP 5

/// Prozess terminieren (Kann zwar abgefangen werden, aber nach abarbeiten der
/// Routine wird der Prozess trotzdem beendet).
#define SIGABRT 6

/// TODO
#define SIGBUS 7

/// Fehler bei einer arithmetischen Operation
#define SIGFPE 8

/// Prozess terminieren (kann _nicht_ abgefangen werden)
#define SIGKILL 9

/// Benutzerdefiniertes Signal 1
#define SIGUSR1 10

/// Speicherzugriffsfehler
#define SIGSEGV 11

/// Benutzerdefiniertes Signal 2
#define SIGUSR2 12

/// Pipe-Fehler
#define SIGPIPE 13

/// alarm()-Timer ist abgelaufen
#define SIGALRM 14

/// Prozess terminieren (kann abgefangen werden)
#define SIGTERM 15

/// Start des frei verwendbaren Bereichs
#define SIGRTMIN 32

/// Ende des frei verwendbaren Bereichs
#define SIGRTMAX 63

/// Prozess stoppen
#define SIGTSTP 64

/// Prozess gestoppt
#define SIGSTOP 65

/// Prozess forgesetzt
#define SIGCONT 66

/// Terminal-Größe verändert
#define SIGWINCH 67

/// Kindprozess beendet, gestoppt oder fortgesetzt
#define SIGCHLD 68

/// Nur fuer interne Verwendung. Muss immer 1 groesser sein als die groesste
/// Signalnummer und teilbar durch 8
#define _SIGNO_MAX 72




#ifdef __cplusplus
extern "C" {
#endif


// Gehoert nach Spezifikation da nicht rein.
void _signal_default_handler(int signal);

/// Standard-Signalhandler
#define SIG_DFL (&_signal_default_handler)

/// Das gibt signal im Fehlerfall zurueck
#define SIG_ERR ((sighandler_t) -1)

/// Signalhandler fuer ignonierte Signale
#define SIG_IGN (NULL)


// Verwaltung von sigsets

/// Typ fuer eine Sammlung von Signalen
typedef struct {
    uint8_t bitmask[_SIGNO_MAX/8];
} sigset_t;

/// Sigset leeren
int sigemptyset(sigset_t *sigset);

/// Alle Signale ins sigset packen
int sigfillset(sigset_t *sigset);

/// Einzelnes Signal zu sigset hinzufuegen
int sigaddset(sigset_t *sigset, int signal);

/// Einzelnes Signal aus sigset loeschen
int sigdelset(sigset_t *sigset, int signal);

/// Pruefen ob ein Signal im sigset enthalten ist
int sigismember(const sigset_t *sigset, int signal);



// Signal-Handler

/// Typ fuer einen Pointer auf einen Signal-Handler
typedef void (*sighandler_t)(int);

/// Bei einem Signal auszufuehrende Aktion
struct sigaction {
    sighandler_t    sa_handler;
    sigset_t        sa_mask;
    int             sa_flags;
};

enum {
    SIG_BLOCK,
    SIG_UNBLOCK,
    SIG_SETMASK,
};

/// Fuer SIGCHLD: Signal nicht ausloesen, wenn ein Kindprozess gestoppt wird
#define SA_NOCLDSTOP 1

/// Fuer SIGCHLD: Nicht auf Kindprozesse warten
#define SA_NOCLDWAIT 2

/// Signal wird wahrend dem Signalhandler nicht automatisch blockiert
#define SA_NODEFER 4


/**
 * Aendert die auszufuehrende Aktion fuer ein Signal und gibt die alte Aktion
 * zurueck.
 *
 * @param sig Nummer des Signals, fuer das die Aktion geaendert wird
 * @param action Neue Aktion. Wenn NULL, wird nur die aktuelle Aktion
 * zurueckgegeben, aber keine Aenderung vorgenommen
 * @param old Wenn nicht NULL, wird hier die alte Aktion gespeichert
 */
int sigaction(int sig, const struct sigaction* action, struct sigaction* old);

/// Einen Signal-Handler aendern
sighandler_t signal(int signum, sighandler_t handler);

/**
 * Verwaltung von blockierten Signalen
 *
 * @param mode
 *      SIG_BLOCK: Alle Signale aus sigset blockieren
 *      SIG_UNBLOCK: Alle Signale aus sigset wieder aktivieren
 *      SIG_SETMASK: sigset als neue Signal-Maske benutzen
 *
 * @return 0 bei Erfolg. -1 und errno = EINVAL, wenn mode einen unerlaubten
 * Wert hat.
 */
int sigprocmask(int mode, const sigset_t* sigset, sigset_t* oldset);

/// Ein Signal zum aktuellen Prozess senden
int raise(int signal);

/// Ein Signal an einen anderen Prozess senden
int kill(pid_t pid, int signal);


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // ifndef _SIGNAL_H_

