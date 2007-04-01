#ifndef _STDARG_H
#define _STDARG_H

typedef char * va_list;

// gibt die Größe des Typs type aufgerundet auf das nächste Vielfache von sizeof(int) zurück.
#define _va_rounded_size(type) (((sizeof(type) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, f) ( ap = (va_list)&f + sizeof(int) )
#define va_arg(ap, type) (*(type*)((ap += _va_rounded_size(type)) - _va_rounded_size(type)))
#define va_end(ap)

#ifndef _VA_LIST
#define _VA_LIST va_list
#endif /* ndef _VA_LIST */

#endif /* ndef _STDARG_H */
