#ifndef CTYPE_H
#define CTYPE_H

/* Das hier hab ich nur für strtol angefangen. Todo: Vervollständigen */

#define isspace(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isxdigit(c) (isdigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))

#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define islower(c) ((c) >= 'a' && (c) <= 'z')

//FIXME
#define isprint(c) ((c) >= (char) 0x20)
#define iscntrl(c) ((c) < (char) 0x20)

#define tolower(c) (isupper(c) ? (c) - 'A' + 'a' : (c))
#define toupper(c) (islower(c) ? (c) - 'a' + 'A' : (c))

#endif /* ndef CTYPE_H */
