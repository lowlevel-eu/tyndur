#ifndef CTYPE_H
#define CTYPE_H

/* Das hier hab ich nur für strtol angefangen. Todo: Vervollständigen */

#define isspace(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')

#define tolower(c) (isupper(c) ? (c) - 'A' + 'a' : (c))

#endif /* ndef CTYPE_H */
