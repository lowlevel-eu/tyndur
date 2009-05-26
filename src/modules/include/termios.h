/*  
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#ifndef _TERMIOS_H_
#define _TERMIOS_H_

#ifndef CONFIG_LIBC_NO_STUBS

/// Namen fuer Zeichen in c_cc
enum {
    VINTR,
    VQUIT,
    VERASE,
    VKILL,
    VEOF,
    VTIME,
    VMIN,
    VSTART,
    VSTOP,
    VSUSP,
    VEOL,
    VWERASE,
    VLNEXT,
    VEOL2,

    /// Anzahl der Kontrollzeichen
    NCCS
};

// Eingabeflags
/// CR => NL bei der Eingabe
#define ICRNL       (1 << 0)
/// NL => CR bei der Eingabe
#define INLCR       (1 << 1)
/// ignoriere CR
#define IGNCR       (1 << 2)
/// TODO
#define ISTRIP      (1 << 3)
///
#define IXON        (1 << 4)
///
#define BRKINT      (1 << 5)
///
#define PARMRK      (1 << 6)
///
#define IGNBRK      (1 << 7)
///
#define IGNPAR      (1 << 8)
///
#define INPCK       (1 << 9)
///
#define IXOFF       (1 << 10)


// Ausgabeflags
///
#define OPOST       (1 << 0)


// Kontrollflags
/// Groesse eines Zeichens in Bit
#define CSIZE       ((1 << 0) | (1 << 1))
    #define CS5     (0 << 0)
    #define CS6     (1 << 0)
    #define CS7     (2 << 0)
    #define CS8     (3 << 0)
/// Baudrate
#define CBAUD       ((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5))
    #define B0      (0 << 2)
    #define B50     (1 << 2)
    #define B75     (2 << 2)
    #define B110    (3 << 2)
    #define B134    (4 << 2)
    #define B150    (5 << 2)
    #define B200    (6 << 2)
    #define B300    (7 << 2)
    #define B600    (8 << 2)
    #define B1200   (9 << 2)
    #define B1800   (10 << 2)
    #define B2400   (11 << 2)
    #define B4800   (12 << 2)
    #define B9600   (13 << 2)
    #define B19200  (14 << 2)
    #define B38400  (15 << 2)
///
#define CLOCAL      (1 << 6)
///
#define CREAD       (1 << 7)
///
#define CSTOPB      (1 << 8)
///
#define HUPCL       (1 << 9)
///
#define PARENB      (1 << 10)
///
#define PARODD      (1 << 11)


// Lokale Flags
///
#define ECHO        (1 << 0)
///
#define ECHONL      (1 << 1)
/// Cannonical Input
#define ICANON      (1 << 2)
/// Signale Aktivieren
#define ISIG        (1 << 3)
///
#define IEXTEN      (1 << 4)
///
#define NOFLSH      (1 << 5)
///
#define ECHOE       (1 << 6)
///
#define ECHOK       (1 << 7)

// Fuer tcflush
/// Flush pending input. Flush untransmitted output.
#define TCIFLUSH    0
/// Flush both pending input and untransmitted output.
#define TCIOFLUSH   1
/// Flush untransmitted output.
#define TCOFLUSH    2

// Fuer tcsetattr
/// Change attributes immediately.
#define TCSANOW     0
/// Change attributes when output has drained.
#define TCSADRAIN   1
/// Change attributes when output has drained; also flush pending input.
#define TCSAFLUSH   2


typedef unsigned int speed_t;
typedef unsigned int tcflag_t;
typedef char cc_t;
struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t     c_cc[NCCS];
};

int tcgetattr(int file, struct termios* tios);
int tcsetattr(int fd, int optional_actions, const struct termios* tios);
int tcflush(int fd, int queue_selector);
speed_t cfgetospeed(const struct termios* tios);

#endif
#endif
