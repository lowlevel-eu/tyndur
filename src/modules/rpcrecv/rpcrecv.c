#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#define _USE_START_
#include "init.h"


void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data);

void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data);

int main(int argc, char* argv[])
{
    /*puts("\nSetze RPC-Handler...");

    init_service_register("rpcrecv");



    if (!register_message_handler("HELOWRLD", &hello_world)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    }*/

    init_service_register("rot");
    register_message_handler("IO_OPEN ", &rpc_io_open);
    register_message_handler("IO_CLOSE", &rpc_io_close);
    register_message_handler("IO_READ ", &rpc_io_read);
register_message_handler("IO_WRITE", &rpc_io_write);
    while(1) yield();
}

void hello_world(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    rpc_send_dword_response(pid, correlation_id, 1337);
}








//Aus LightOS
dword atoi(const char *s)
{
	size_t digit = 0;
	size_t highest = 0;
    size_t i;
	for (i=0;s[i] != '\0';i++)
	{
		if (s[i] < '0' ||
			s[i] > '9')break;
		if (highest == 0)highest = 1;
		else highest *= 10;
		digit++;
	}
	dword result = 0;
	for (i=0;i < digit;i++)
	{
		result += (s[i] - '0') * highest;
		highest /= 10;
	}
	return result;
}


char rot(char c, byte n)
{
    if((c >= 'a') && (c <= 'z'))
    {
        if((c + n) > 'z')
        {
            return 'a' + (n + c - 'z');
        }
        else
        {
            return c + n;
        }
    }
    else if((c >= 'A') && (c <= 'Z'))
    {
        if((c + n) > 'Z')
        {
            return 'A' + (n + c - 'Z');
        }
        else
        {
            return c + n;
        }
    }
    return c;
}






io_resource_t testpipe_source;
dword rot_count;


void rpc_io_open(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_resource_t io_res;
    //Wird bei pipes als quelle verwendet
    testpipe_source = *((io_resource_t*) ((dword)data + 1));
    //printf("source pid:%d id:%d\n", testpipe_source.pid, testpipe_source.id);
    //Modus zu oeffnen
    //byte* attr = (byte*) data;
    char* path = (char*) ((dword)data + 1 + sizeof(io_resource_t));
    
    rot_count = atoi(&(path[1]));
    io_res.pid = init_service_get("rot");
    io_res.id = 1;
    
    rpc_send_response(pid, correlation_id, sizeof(io_resource_t), (char*) &io_res);
}


void rpc_io_close(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    fclose(&testpipe_source);
    rpc_send_dword_response(pid, correlation_id, 0);
}


void rpc_io_read(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_read_request_t* read_request = (io_read_request_t*) data;
    size_t size = read_request->blocksize * read_request->blockcount;
    char* buffer = malloc(size);
 
    size_t read_bytes = fread(buffer, read_request->blocksize, read_request->blockcount, &testpipe_source);
    //puts(buffer);
    int n;
    for(n = 0; n < (read_bytes - 1); n++)
    {
        buffer[n] = rot(buffer[n], rot_count);
    }
    //puts(buffer);
    rpc_send_response(pid, correlation_id, read_bytes, buffer);
}


void rpc_io_write(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    io_write_request_t* write_request = (io_write_request_t*) data;

    size_t size = fwrite(write_request->data, write_request->blocksize, write_request->blockcount, &testpipe_source);
    /*if(size > (testfile_size - testfile_position))
    {
        size = testfile_size - testfile_position;
    }

    memcpy((void*)((dword)testfile + testfile_position), write_request->data, size);*/

    rpc_send_dword_response(pid, correlation_id, size);
}



