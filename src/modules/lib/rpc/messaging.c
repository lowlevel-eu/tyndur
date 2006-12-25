#include "types.h"
#include "rpc.h"
#include "syscall.h"
#include "string.h"

#define MAX_MESSAGE_HANDLERS 10

// Diese Deklaration ist so eigentlich nicht richtig, der RPC-Handler nimmt
// durchaus Parameter. Allerdings folgt das ganze sowieso nicht dem
// C-Aufrufschema, so daﬂ jede andere Deklaration genauso falsch w‰re.
extern void rpc_handler();
void putn(unsigned long long x, int radix, int pad, char padchar);

typedef struct {
    char name[8];
    void (*handler)(int, void*);
} msg_handler_t;

void (*intr_handler[256])() = { NULL };
msg_handler_t message_handler[MAX_MESSAGE_HANDLERS];

void init_messaging()
{
    dword i;

    set_rpc_handler(&rpc_handler);
    for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
        message_handler[i].name[0] = '\0';
        message_handler[i].handler = NULL;
    }
}

bool register_message_handler(char* fnname, void (*handler)(int, void*))
{
    char padded_fnname[8];
    dword i, index;

    // Es sind genau acht Byte f¸r den Funktionsnamen reserviert
    strncpy(padded_fnname, fnname, 8);

    // Freien Index im message_handler-Array besorgen und pr¸fen,
    // ob nicht bereits eine Funktion mit demselben Namen
    // existiert.
    index = MAX_MESSAGE_HANDLERS;
    for (i= 0; i < MAX_MESSAGE_HANDLERS; i++) {
        if (message_handler[i].name == NULL) {
            index = i;
        } else if (strncmp(padded_fnname, message_handler[i].name, 8) == 0) {
            return FALSE;
        }
    }
    
    if (index == MAX_MESSAGE_HANDLERS) {
        return FALSE;
    }

    strncpy(message_handler[index].name, padded_fnname, 8);
    message_handler[index].handler = handler;

    return TRUE;
}

void register_intr_handler(byte intr, void (*handler)())
{
    intr_handler[intr] = handler;
    add_intr_handler(intr);
}

void c_rpc_handler(dword data_size, char* data)
{
    dword function = *((dword*) data);

    if (function < 256) {
        intr_handler[function]();
    } else if (function == 512) {
        // RPC by function name
        // TODO
    }
}
