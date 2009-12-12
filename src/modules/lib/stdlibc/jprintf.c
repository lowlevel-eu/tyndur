/*
 * Copyright (c) 2006-2007  The tyndur Project. All rights reserved.
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
 
/* 
 * Die hier definierte jvprintf Funktion wird von allen anderen *printfs
 * für die Formatierung genutzt. Die funktionsspezifische Ausgabe erfolgt 
 * über callbacks. Eine simple Anwendung dieser Funktion gibt es in printf.c.
 */

#include "stdlib.h"
#include "string.h"
#include "types.h"

#include "jprintf.h"

/** Dividiert ein uint64 durch einen uint32 und gibt das Ergebnis zurück
 *
 * @param dividend
 * @param divisor
 * @param remainder Wenn ungleich NULL, wird in *remainder der Rest zurückgegeben
 *
 * @return dividend/divisor
 */
unsigned long long divmod(unsigned long long dividend, unsigned int divisor, unsigned int * remainder)
{
    unsigned long long quotient;
    unsigned int rem;

    quotient = dividend / divisor;
    rem = dividend % divisor;
    
    if(remainder)
    {
        *remainder = rem;
    }

    return quotient;
}

/**
 * Konvertiert eine vorzeichenlose 64-Bit-Zahl in eine Zeichenkette.
 *
 * @param value Die Zahl
 * @param buf Der Puffer. strlen(buf) = 64*log(2)/log(radix)+1
 * @param radix Die Basis der auszugebenen Zeichenkette. 2 <= radix <= 36
 * @param uppercase Wenn ungleich 0, werden Großbuchstaben für Ziffern > 9 benutzt.
 *
 * @return buf
 */
char * ulltoa(unsigned long long value, char * buf, unsigned int radix, unsigned int uppercase)
{
    char * p = buf;
    const char * const chars = uppercase ? "0123456789ABCDEFGHIJKLMOPQRSTUVWXYZ" : "0123456789abcdefghijklmopqrstuvwxyz";
    unsigned long long temp;
    unsigned int digits;
    unsigned int remainder;

    // Es werden nur Basen zwischen 2 und 36 unterstützt
    if(radix < 2 || radix > 36)
    {
        return buf;
    }
    
    // Anzahl der Ziffern zählen
    temp = value;
    digits = 0;
    do
    {
        digits++;
        temp = divmod(temp, radix, 0);
    }
    while(temp > 0);

    // Zeiger auf das Ende der Zahl setzen und Nullterminierung einfügen
    p += digits;
    *p = 0; 

    // Ziffern rückwärts in den Puffer schreiben
    temp = value;
    do
    {
        temp = divmod(temp, radix, &remainder);
        *--p = chars[remainder];
    }
    while(--digits);

    return buf;
}

/**
 * Konvertiert eine Gleitkommazahl in einen String
 *
 * @param value Die Zahl
 * @param buf Der Puffer
 * @param precision Anzahl der Nachkommastellen
 *
 * @return buf
 */
static void ldtoa(long double value, char* buf, unsigned int precision)
{
    // Negative Zahlen gleich rausfiltern
    if (value < 0) {
        *buf++ = '-';
        value = -value;
    }

    // Integeranteil ausgeben
    ulltoa((unsigned long long) value, buf, 10, 0);
    value -= (unsigned long long) value;
    buf += strlen(buf);

    // Nachkommastellen
    if (value && precision) {
        *buf++ = '.';
    }

    while (value && precision--) {
        value *= 10.0;
        *buf++ = '0' + (int) value;
        value -= (int) value;
    }

    *buf = '\0';
}


/**
 * Ruft die dem Kontext entsprechende putc Funktion auf.
 *
 * @param args Der Kontext
 * @param c Das Zeichen
 *
 * @return Anzahl der ausgegebenen Bytes
 *
 * @see jvprintf()
 */
int jprintf_putc(struct jprintf_args * args, char c)
{
	if(args->putc_fct != NULL)
	{
		return args->putc_fct(args->arg, c);
	}

	return 1;
}

/**
 * Ruft die dem Kontext entsprechende putsn Funktion auf.
 *
 * @param args Der Kontext
 * @param string Die Zeichenkette
 * @param n Die Anzahl der Zeichen oder -1 für Ausgabe bis zur Nullterminierung
 *
 * @return Anzahl der ausgegebenen Bytes. Negative Zahl im Fehlerfall.
 *
 * @see jvprintf()
 */
int jprintf_putsn(struct jprintf_args * args, const char * string, int n)
{
	if(args->putsn_fct != NULL)
	{
		return args->putsn_fct(args->arg, string, n);
	}
	else
	{
	    // Wenn keine putsn Funktion definiert wurde, auf putc zurückfallen
		int i = 0;
		int bytes_written = 0;

		while(string[i] && (i < n || n == -1))
		{
			bytes_written += jprintf_putc(args, string[i]);
			i++;
		}

		return bytes_written;
	}
}

// WRITE_STRING und WRITE_CHAR geben einen String bzw. einen char aus, und
// verlassen gegebenen Falls die aufrufende Funktion. Nur in jprintf zu verwenden.
#define WRITE_STRING(string, num) \
do { \
	int ret = jprintf_putsn(args, string, num); \
	if(ret < 0) \
	{ \
		return ret; \
	} \
	else if(num != -1 && ret != num) \
	{ \
        return bytes_written + ret;\
	} \
	bytes_written += ret; \
} while(0) 

#define WRITE_CHAR(c) \
do { \
	int ret = jprintf_putc(args, c); \
	if(ret < 0) \
	{ \
		return ret; \
	} \
	else if(ret == 0) \
	{ \
        return bytes_written;\
	} \
	bytes_written += ret; \
} while(0) 

/**
 * Speichert Typ und Wert eines Parameters
 */
struct positional_parameter
{
    enum {
        unspecified = 0x01,
        int_type = 0x02,
        //long_type = 0x03,
        long_long_type = 0x04,
        integral_type_mask = 0x0f,
        unsigned_type_flag = 0x100,
        uint_type = unsigned_type_flag | int_type,
        //ulong_type = unsigned_type_flag | long_type,
        ptr_type = 0x10,
        double_type = 0x20,
        long_double_type = 0x30,
    } type;
    union
    {
        int int_value;
        //long long_value;
        long long long_long_value;
        unsigned int uint_value;
        //unsigned long ulong_value;
        unsigned long long ulong_long_value;
        char *ptr_value;
        double double_value;
        long double long_double_value;
    } value;
};

/**
 * Liest eine Zahl gefolgt von einem $ ein. Gibt die gelesene Zahl zurueck.
 * Bei einem Fehler wird 0 zurueckgegeben. Bei Erfolg, zeigt *ppformat nach
 * dem Aufruf auf das Zeichen hinter dem $.
 *
 * @param ppformat Zeiger auf einen Zeiger auf die zu parsende Zeichenkette.
 */
static int read_index(const char **ppformat)
{
    int index;

    // die Zahl parsen
    index = strtol(*ppformat, (char**)ppformat, 10);
    if (index > 0 && **ppformat == '$') {
        // wenn die Zahl von einem $ gefolgt wird, ist sie ein gueltiger Index
        ++*ppformat;
    } else {
        // ungueltiger positional parameter (z.B. es wurde keiner angegeben)
        //printf("ungueltiger positional parameter %d %s\n", index, *ppformat);
        return 0;
    }

    return index;
}

/**
 * Parst einen Format-String und gibt ein Array zurueck, das die Typen und
 * Werte der Argumente enthaelt.
 *
 * @param format Zu parsender Format-String.
 * @param ap Argumentenliste
 */
struct positional_parameter *parse_pos_params(const char *format, va_list ap)
{
    struct positional_parameter *pos_params;
    int buf_size = 32; // Anzahl Element im Array
    int index, width_index, precision_index; // indizes der parameter
    int arg_type; // Typ des Arguments
    int max_index = 0; // hoechster im format-string vorkommender index
    int i;

    // Das Array mit den Informationen ueber die Argumente erzeugen.
    //
    // pos_params[0] ist kein gueltiger Parameter, da die Parameter ab 1
    // gezaehlt werden. pos_params[1 ... max_index] sollten im Verlauf dieser
    // Funktion mit gueltigen Werten gefuellt werden.
    //
    // Existiert ein i mit 1 <= i <= max_index fuer das pos_params[i].type ==
    // unspecified ist, wurde ein Parameter nicht angegeben. In diesem Fall,
    // wird der Typ als int angenommen.
    pos_params = malloc(buf_size * sizeof (struct positional_parameter));
    for (i = 0; i < buf_size; i++) {
        pos_params[i].type = unspecified;
    }

    // Wenn positional und non-positional parameter gemischt werden, wird bei
    // einem non-positional parameter auf pos_params[0] zugegriffen. Diesen
    // fuers Debugging mit einem wiedererkennbaren Wert laden.
    pos_params[0].value.int_value = 0xdddddddd;

    while (*format) {
        // das naechste % suchen
        if (*format++ != '%') {
            continue;
        }

        // %% ueberspringen
        if (*format == '%') {
            format++;
            continue;
        }

        arg_type = unspecified;
        width_index = 0;
        precision_index = 0;

        index = read_index(&format);

        // flags
        switch (*format) {
            case '+': case '-': case ' ': case '#': case '0':
                format++;
                break;
        }

        // width
        if ('0' <= *format && *format <= '9') {
            strtol(format, (char**)&format, 10);
        } else if (*format == '*') {
            format++;
            width_index = read_index(&format);
        }

        // precision
        if (*format == '.') {
            format++;
            if ('0' <= *format && *format <= '9') {
                strtol(format, (char**)&format, 10);
            } else if (*format == '*') {
                format++;
                precision_index = read_index(&format);
            }
        }

        // length
        switch (*format) {
            case 'h':
                // h oder hh - char bzw. short
                format++;
                arg_type = int_type;
                if (*format == 'h')
                    format++;
                break;
            case 'l':
                // l oder ll - long bzw. long long
                format++;
                //arg_type = long_type;
                if (*format == 'l') {
                    format++;
                    arg_type = long_long_type;
                }
                break;
            case 'L':
                // L - long double
                format++;
                arg_type = long_double_type;
                break;
        }

        // Das Format auswerten und wenn noetig arg_type anpassen.
        switch (*format) {
            case 'o':
            case 'u':
            case 'x':
            case 'p':
            case 'X':
                // unsigned
                if ((arg_type & integral_type_mask) != 0) {
                    arg_type |= unsigned_type_flag;
                } else {
                    // widerspruch (z.B. %Lo)
                    //printf("ungueltiger format spezifizierer 1 %x\n", arg_type);
                }
                break;
            case 'd':
            case 'i':
            case 'c':
                // signed
                if ((arg_type & integral_type_mask) == 0) {
                    // widerspruch (z.B. %Ld)
                    //printf("ungueltiger format spezifizierer 2 %x\n", arg_type);
                }
                break;
            case 'f':
            case 'g':
                // double
                if (arg_type == unspecified) {
                    arg_type = double_type;
                } else if (arg_type != long_double_type) {
                    // widerspruch (z.B. %lf)
                    //printf("ungueltiger format spezifizierer 3 %x\n", arg_type);
                }
                break;
            case 's':
                // string
                if (arg_type != unspecified) {
                    // widerspruch (z.B. %ls)
                    //printf("ungueltiger format spezifizierer 4 %x\n", arg_type);
                }
                arg_type = ptr_type;
                break;
        }

        // feststellen, ob wir das array vergroessern muessen
        if (max_index < index)
            max_index = index;
        if (max_index < width_index)
            max_index = width_index;
        if (max_index < precision_index)
            max_index = precision_index;

        if (max_index >= buf_size) {
            pos_params = realloc(pos_params, (max_index + 1) * sizeof (struct positional_parameter));
            while (buf_size < max_index + 1) {
                pos_params[buf_size++].type = unspecified;
            }
        }

        // index, width_index und precision_index in pos_params eintragen
        if (index > 0) {
            if (pos_params[index].type == unspecified || pos_params[index].type == arg_type) {
                pos_params[index].type = arg_type;
            } else {
                // widerspruch bei den typen
                //printf("verschiedene typen fuer index %d (argument) spezifiziert: %d und %d\n", pos_params[index].type, arg_type);
            }
        }

        if (width_index > 0) {
            if (pos_params[width_index].type == unspecified || pos_params[width_index].type == int_type) {
                pos_params[width_index].type = int_type;
            } else {
                // widerspruch bei den typen
                //printf("verschiedene typen fuer index %d (width) spezifiziert: %d und int\n", pos_params[index].type);
            }
        }

        if (precision_index > 0) {
            if (pos_params[precision_index].type == unspecified || pos_params[precision_index].type == int_type) {
                pos_params[precision_index].type = int_type;
            } else {
                // widerspruch bei den typen
                //printf("verschiedene typen fuer index %d (precision) spezifiziert: %d und int\n", pos_params[index].type);
            }
        }
    }


    // alle Parameter in das Array eintragen
    for (i = 1; i <= max_index; i++) {
        switch (pos_params[i].type) {
            case unspecified:
                // ein parameter wurde ausgelassen
                //printf("parameter %d nicht spezifiziert\n", i);
                pos_params[i].type = int_type;
                // fall through
            case int_type:
                pos_params[i].value.int_value = va_arg(ap, int);
                break;
            //case long_type:
            //    pos_params[i].value.long_value = va_arg(ap, long);
            //    break;
            case long_long_type:
                pos_params[i].value.ulong_long_value = va_arg(ap, long long);
                break;
            case unsigned_type_flag | int_type:
                pos_params[i].value.uint_value = va_arg(ap, unsigned int);
                break;
            //case unsigned_type_flag | long_type:
            //    pos_params[i].value.ulong_value = va_arg(ap, unsigned long);
            //    break;
            case unsigned_type_flag | long_long_type:
                pos_params[i].value.ulong_long_value = va_arg(ap, unsigned long long);
                break;
            case double_type:
                pos_params[i].value.double_value = va_arg(ap, double);
                break;
            case long_double_type:
                pos_params[i].value.long_double_value = va_arg(ap, long double);
                break;
            case ptr_type:
                pos_params[i].value.ptr_value = va_arg(ap, char*);
                break;
            default:
                // interner fehler
                //printf("unerwarteter typ %d\n", pos_params[i].type);
                break;
        }
    }

    return pos_params;
}

/** 
 * vprintf mit Callbacks. Gibt nicht das terminierende Nullbyte aus.
 *
 * @param args Der Kontext bestehend aus Zeiger auf die putc und putsn-Funktionen sowie auf einen benutzerdefinierten Parameter
 * @param format der format string
 * @param ap die Argumente
 */
int jvprintf(struct jprintf_args * args, const char * format, va_list ap)
{
	const char * t;
	int bytes_written;

	char flags;
	unsigned int width;
	unsigned int precision;
	unsigned int length;
    unsigned int long_double;
	unsigned int temp;
	int arg_index = 0;
	struct positional_parameter *pos_params = NULL;
	
	bytes_written = 0;

	// Das erste % suchen, und bis dahin alles ausgeben.
	t = format;
	while(*format != '%' && *format)
	{
		format++;
	}

	WRITE_STRING(t, format - t);

	// Wenn keine % in format waren, sind wir fertig.
	if(!*format)
	{
		return bytes_written;
	}

	// Wenn wir hier ankommen, stehen wir mit format auf einem
	// Formatspezifizierer. Wenn positional parameters verwendet werden, muss
	// der erste Formatspezifizierer auch einen positional parameter haben.

	if ('0' <= *(format + 1) && *(format + 1) <= '9') {
		temp = strtol(format + 1, (char**)&t, 10);

		// Wenn das ein gueltiger positional parameter ist, ...
		if (temp != 0 && *t == '$') {
			// ... den format-string einmal komplett parsen.
			pos_params = parse_pos_params(format, ap);
		}
	}

	while(*format)
	{
		t = format;
		while(*format != '%' && *format)
		{
			format++;
		}

		WRITE_STRING(t, format - t);

		if(!*format)
		{
			break;
		}

		format++;
		
		if(*format == '%')
		{
			WRITE_CHAR(*format);
			format++;
			continue;
		}

		// default values
		flags = 0;
		width = 0;
		precision = -1;
		length = 32;
        long_double = 0;

		// wenn wir positional parameter verwenden, den index des arguments lesen
		if (pos_params != NULL) {
			arg_index = read_index(&format);
			//printf("[arg_index=%d, type=%d, v=%x]\n", arg_index, pos_params[arg_index].type, pos_params[arg_index].value.uint_value);
		}

		// flags
		switch(*format)
		{
		case '+':
		case '-':
		case ' ':
		case '#':
		case '0':
			flags = *format;
			format++;
		}

		// width
		switch(*format)
		{
		case '0' ... '9':
			width = strtol(format, (char**)&format, 10);
			break;
		case '*':
			format++;
			if (pos_params != NULL) {
				width = pos_params[read_index(&format)].value.int_value;
			} else {
				width = va_arg(ap, int);
			}
			break;
		}

		// precision
		if(*format == '.')
		{
			format++;
			switch(*format)
			{
			case '0' ... '9':
				precision = strtol(format, (char**)&format, 10);
				break;
			case '*':
				format++;
				if (pos_params != NULL) {
					precision = pos_params[read_index(&format)].value.int_value;
				} else {
					precision = va_arg(ap, int);
				}
				break;
			}
		}

		// length
		switch(*format)
		{
		case 'h':
			// short
			length = 16;
			format++;
            // char
			if(*format == 'h')
			{
                length = 8;
                format++;
			}
			break;
		case 'l':
			// long
            length = 32;
            format++;
            // long long
			if(*format == 'l')
			{
                length = 64;
                format++;
			}
			break;
		case 'L':
			// long double
            long_double = 1;
			format++;
			break;
		}

		// format specifier
		switch(*format)
		{
		    {
				char buf[67];
				int sign;

		case 'o':
        case 'u':
        case 'x':
        case 'p':
        case 'X':
                {
                    unsigned long long value = 0;
                    switch(length)
                    {
                        case 8:
                        case 16:
                        case 32:
                            if (pos_params != NULL) {
                                value = pos_params[arg_index].value.uint_value;;
                            } else {
                                value = va_arg(ap, unsigned int);
                            }
                            break;
                        case 64:
                            if (pos_params != NULL) {
                                value = pos_params[arg_index].value.ulong_long_value;;
                            } else {
                                value = va_arg(ap, unsigned long long);
                            }
                            break;
                    }
                    ulltoa(value, buf, (*format == 'p' || *format == 'x' || *format == 'X') ? 16 : ((*format == 'o') ? 8 : 10) , (*format == 'X' ? 1 : 0));
                    sign = 1;
                }

                if(0) // fall through und den nächsten {}-block nicht ausführen
		case 'd':
		case 'i':
                {
                    signed long long value = 0;
                    
                    switch(length)
                    {
                        case 8:
                        case 16:
                        case 32:
                            if (pos_params != NULL) {
                                value = pos_params[arg_index].value.int_value;;
                            } else {
                                value = va_arg(ap, int);
                            }
                            break;
                        case 64:
                            if (pos_params != NULL) {
                                value = pos_params[arg_index].value.long_long_value;;
                            } else {
                                value = va_arg(ap, long long);
                            }
                            break;
                    }
                    
                    if(value < 0)
                    {
                        buf[0] = '-';
                        ulltoa(-value, buf+1, 10, 0);
                    }
                    else
                    {
                        ulltoa(value, buf, 10, 0);
                    }
                    
                    sign = (value < 0) ? -1 : (value > 0) ? 1 : 0;
                }
    
				if(width != 0)
				{
					int len = strlen(buf);

					switch(flags)
					{
					case ' ':
					case 0: // keine flags angegeben
						for( ; len < width; len++)
						{
							WRITE_CHAR(' ');
						}
						WRITE_STRING(buf, -1);
						break;
					case '-':
						WRITE_STRING(buf, -1);
						for( ; len < width; len++)
						{
							WRITE_CHAR(' ');
						}
						break;
					case '0':
						if(sign < 0)
						{
							WRITE_CHAR('-');
						}
						for( ; len < width; len++)
						{
							WRITE_CHAR('0');
						}
						if(sign < 0)
						{
							WRITE_STRING(buf + 1, -1); // - überspringen
						}
						else
						{
							WRITE_STRING(buf, -1);
						}
						break;
					case '+':
						if(sign >= 0)
						{
							len++;
						}
						for( ; len < width; len++)
						{
							WRITE_CHAR(' ');
						}
						if(sign >= 0)
						{
							WRITE_CHAR('+');
						}
						WRITE_STRING(buf, -1);
						break;
					default:
						WRITE_STRING(buf, -1);
						break;
					}
				}
				else
				{
					switch(flags)
					{
					case '+':
						if(sign >= 0)
						{
							WRITE_CHAR('+');
						}
						break;
					case ' ':
						if(sign >= 0)
						{
							WRITE_CHAR(' ');
						}
						break;
					}
					
					WRITE_STRING(buf, -1);
				}
			}
			format++;
			break;
        case 'f':
        case 'g':
            // FIXME Bei g duerfen keine abschliessenden Nullen vorkommen
            if (precision == -1) {
                precision = 6;
            }
            {
                // 64-Bit-Integer gehen dezimal in maximal 21 Zeichen, dazu
                // Vorzeichen, Komma und abschliessendes Nulbyte
                char buf[precision + 21 + 3];
                long double value;

                if (pos_params != NULL) {
                    if (long_double) {
                        value = pos_params[arg_index].value.long_double_value;
                    } else {
                        value = pos_params[arg_index].value.double_value;
                    }
                } else {
                    if (long_double) {
                        value = va_arg(ap, long double);
                    } else {
                        value = va_arg(ap, double);
                    }
                }

                ldtoa(value, buf, precision);
                WRITE_STRING(buf, -1);
            }
            format++;
            break;
        case 'c':
            {
                char c;
                if (pos_params != NULL) {
                    c = (char)pos_params[arg_index].value.int_value;
                } else {
                    c = (char)va_arg(ap, int);
                }
                WRITE_CHAR(c);
            }
            format++;
            break;
		case 's':
			{
				char *string;
				if (pos_params != NULL) {
					string = pos_params[arg_index].value.ptr_value;
					} else {
					string = va_arg(ap, char*);
				}
				if(width != 0)
				{
					int len = strlen(string), pad;

					switch(flags)
					{
					case '-':
						WRITE_STRING(string, len);
						for(pad = len ; pad < width; pad++)
						{
							WRITE_CHAR(' ');
						}
						break;
					case '0':
						for(pad = len ; pad < width; pad++)
						{
							WRITE_CHAR('0');
						}
						WRITE_STRING(string, len);
						break;
					case ' ':
					case 0: // keine flags angegeben
						for(pad = len ; pad < width; pad++)
						{
							WRITE_CHAR(' ');
						}
						WRITE_STRING(string, len);
						break;
					default:
						WRITE_STRING(string, len);
						break;
					}
				}
				else
				{
					WRITE_STRING(string, -1);
				}
			}
			format++;
			break;
		}
	}
	
	if (pos_params != NULL) {
		free(pos_params);
	}

	return bytes_written;
}
