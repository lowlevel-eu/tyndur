#include "types.h"
#include "syscall.h"
#include "rpc.h"


struct service
{
    unsigned short int group;
    unsigned short int device;
    unsigned short int pid;
    struct service* next;
};

void service_register(dword data_size, void* data);
void service_get(dword data_size, void* data);
bool service_next();


struct service *first = NULL;
struct service *curr = NULL;

void _start() {
    puts("\nSetze RPC-Handler für INIT...");
    init_messaging();

    if (!register_message_handler("SERV_REG", &service_register)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    } else {
        puts("TEST___1 ok\n");
    }

    if (!register_message_handler("SERV_GET", &service_get)) {
        puts("Konnte RPC-Handler nicht registrieren\n");
    } else {
        puts("TEST___2 ok\n");
    }
    
    while(1) { yield(); }
}

void service_register(dword data_size, void* data)
{
    /*
    puts("[RPCRECV] Hello world (aus TEST1)!\n");
    char dat[data_size+1];
    int x = 0;
    for (x = 0; x < data_size; x++)
    {
        dat[x] = *(char*)(data+x);
    }
    dat[data_size] = '\0';
    puts(dat);
    //putn(data_size);
    puts("\n");
    */
    puts("SERVICE_REGISTER START\n");
	struct service *temp;
    temp = mem_allocate(sizeof(struct service),0);
    if (first == NULL)
    {
        first = temp;
	puts("First Element\n");
    }
    else
    {
        // Durch Prozesse iterieren.
        while (service_next())
        {
            // Nur um ans letzte Element zu gelangen    
        }
        (*curr).next = temp;
	puts("New Last Element\n");
    }
}

void service_get(dword data_size, void* data)
{
    puts("[RPCRECV] Hello world (aus TEST2)!\n");
    puts(*(char*)data);
    puts("\n");
}

bool service_next()
{
    if (curr == NULL && first != NULL)
    {
        curr = first;
        return TRUE;
    } 
    else 
    {
        if ((*curr).next != NULL)
        {
            curr = (*curr).next;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

void service_none()
{
    curr = NULL;
}
