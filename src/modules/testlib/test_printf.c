#include "stdio.h"

// toter code. tests.c entsprechend modifizieren.
void test_printf(void)
{
    printf("TESTE PRINTF \n");
    
    printf("printf: ");
    printf("a");
    printf("%c", 'b');
    printf("%s", "c1");
    printf("%d", 23);
    printf("%o", 045);
    printf("%x (erwartet: abc1234567890)\n", 0x67890);

    unsigned long long ull = 12345678901234567890ULL;
    printf("llu:\t\t%llu%llu\n   (erwartet:\t1234567890123456789012345678901234567890)\n", ull, ull);
    printf("lld:\t\t%lld%lld\n   (erwartet:\t-6101065172474983726-6101065172474983726)\n", ull, ull);

    printf("[%d], [%8d], [% 8d], [%08d], [%-8d] (erwartet: ->\n[-123], [    -123], [    -123], [-0000123], [-123    ])\n", -123, -123, -123, -123, -123);
    
    printf("[%s], [%5s], [%-5s] (erwartet [abc], [  abc], [abc  ])\n", "abc", "abc", "abc");

#define STRINGIFY(x...) #x
#define TOSTRING(x) STRINGIFY(x)

#define FORMAT "a%c%s%d%o%x"
#define ARGS 'b', "c1", 23, 045, 0x67890

    char buf[1000];
    int ret, n, i, len[]={0, 8, 13, 14};
    char* expected[]={"OOPS", "abc1234", "abc123456789", "abc1234567890"};
    printf("sprintf: \n");
    ret = sprintf(buf, FORMAT, ARGS);
    printf("\tsprintf(buf, \"%s\", %s)=%d\n\tbuf=%s, strlen(buf)=%u\n", FORMAT, TOSTRING(ARGS), ret, buf, strlen(buf));

    printf("snprintf: snprintf(buf, n, \"%s\", %s)\n", FORMAT, TOSTRING(ARGS));
    for(i = 0; i < 4; i++)
    {
        n = len[i];
        buf[n] = 'O'; buf[n+1] = 'O'; buf[n+2] = 'P'; buf[n+3] = 'S'; buf[n+4] = 0;
        ret = snprintf(buf, n, FORMAT, ARGS);
        printf("\tsnprintf(...,n=%d,...)=%d, buf=%s, strlen(buf)=%u (erwartet %s)\n", n, ret, buf, strlen(buf), expected[i]);
    }
    
    printf("asprintf: ");
    char * retbuf;
    ret = asprintf(&retbuf, FORMAT, ARGS);
    printf(" %p %s %u\n (erwartet: irgendwas!=0, abc1234567890, 13)", retbuf, retbuf, strlen(retbuf));

}
