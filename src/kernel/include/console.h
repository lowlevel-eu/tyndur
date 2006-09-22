#ifndef CONSOLE_H
#define CONSOLE_H

extern void con_putc(const char c); // Zeichen ausgeben ohne Auswertung der ANSI-Codes
extern void con_putc_ansi(const char c); // Zeichen ausgeben mit Auswertung der ANSI-Codes
extern void con_puts(const char * s); // Zeichenkette ausgeben mit Auswertung der ANSI-Codes
extern void con_flush_ansi_escape_code_sequence(void); // Zwischenspeicher leeren

#endif /* ndef CONSOLE_H */
