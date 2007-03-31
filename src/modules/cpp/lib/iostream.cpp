#include <ostream>
#include <streambuf>
#include <iostream>
extern "C"
{
    #include "string.h"
    #include "stdio.h"
}

namespace std
{
    const int stdout_buffer_size = 1024;
    
    class stdout_streambuf : virtual public basic_streambuf<char, char>
    {
        public:
            explicit stdout_streambuf()
            {
                char* buffer = new char[stdout_buffer_size + 1];
                setp(buffer, buffer + stdout_buffer_size);
            }

            
            ~stdout_streambuf()
            {
                sync();
            }
        protected:
            ///
            virtual int sync()
            {
                *this->pptr() = '\0';
                printf(pbase());
                
                this->setp(pbase(), epptr());
    
                return 0;
            }
    
            ///Mehrere Zeichen auf einmal schreiben
            virtual streamsize xsputn(const char_type* str, streamsize size)
            {
                while(size != 0)
                {
                    if(size < stdout_buffer_size)
                    {
                        memcpy(pbase(), str, size % stdout_buffer_size);
                        this->pbump(size % stdout_buffer_size);
                        size = 0;
                    }
                    else
                    {
                        memcpy(pbase(), str, stdout_buffer_size);
                        this->pbump(stdout_buffer_size);
                        size -= stdout_buffer_size;
                    }
    
                    sync();
                }
    
                return size;
            }
    };
    
    stdout_streambuf stdout_sb;
    
    ostream cout(&stdout_sb);
    ostream cerr(&stdout_sb);
} //namespace std


