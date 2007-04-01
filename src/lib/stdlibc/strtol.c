#include "ctype.h"
#include "types.h"

long strtol(const char *str, char **endptr, int base)
{
    long retval = 0;
    int digit;

    while(isspace(*str))
    {
        str++;
    }

    while(*str)
    {
        if(isdigit(*str) && *str - '0' < base)
        {
            digit = *str - '0';
        }
        else if(isalpha(*str) && tolower(*str) - 'a' + 10 < base)
        {
            digit = tolower(*str) - 'a' + 10;
        }
        else
        {
            break;
        }

        retval = retval * base + digit;
        str++;
    }
    
    if(endptr != NULL)
    {
        *(const char**)endptr = str;
    }
    
    return retval;
}
