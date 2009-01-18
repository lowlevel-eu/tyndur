// Das hier ist ein gemeiner Hack, damit es in den Locking-Funktionen von 
// liballoc keine nicht-aufgelösten Symbole gibt.
void  __attribute__((weak)) p(void) {}
void  __attribute__((weak)) v(void) {}
