#include <ostream>
#include <streambuf>
extern "C"
{
    #include "stdio.h"
    #include "string.h"
}

namespace std
{

    ostream& operator<< (ostream& out, const char* str)
    {
        out.write(str, strlen(str));        
        return out;
    }
} //namespace std

