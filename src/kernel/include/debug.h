#include <stdint.h>
#include <stdbool.h>

/* Debug-Funktionen und Helferlein */

#define DEBUG_FLAG_INIT 1
#define DEBUG_FLAG_STACK_BACKTRACE 2
#define DEBUG_FLAG_PEDANTIC 4
#define DEBUG_FLAG_SYSCALL 8
#define DEBUG_FLAG_NO_KCONSOLE 16

/* Int2Str , Wandelt signed ints in Strings um */
char* int2str(signed int value,char *result);

/* Einfache Print-Funktion für grundlegende Ausgaben */
void print(char *string,const int len);

///Setzt die richtigen Debug-Flags anhand der Commandline vom bootloader
void debug_parse_cmdline(char* cmdline);

///Ueberprueft ob ein bestimmtes Debug-Flag gesetzt ist
bool debug_test_flag(uint32_t flag);

///Gibt die Debug-Meldung aus, wenn das Flag gesetzt ist
void debug_print(uint32_t flag, const char* message);

/* 
 * Gibt einen Stack Backtrace aus, beginnend an den übergebenen Werten
 * für ebp und eip
 */
void stack_backtrace_ebp(uint32_t start_ebp, uint32_t start_eip);
