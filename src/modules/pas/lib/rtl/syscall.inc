procedure syscall_puts(s: PChar); assembler;
asm
    pushl s
    mov SYSCALL_KPRINTF, %eax
    int $0x30
    add $0x4, %esp
end;

procedure puts(s: PChar);
const newline: string[2] = #10#0;
begin
    syscall_puts(s);
    syscall_puts(@newline[1]);
end;

function mem_allocate(size, flags: dword): pointer; assembler;
var
    dummy: longint;
asm
    pushl %eax
    lea dummy, %eax
    pushl %eax
    mov 4(%esp), %eax
    pushl flags
    pushl size
    mov SYSCALL_MEM_ALLOCATE, %eax
    int $0x30
    add $0x10, %esp
end;

procedure yield(); assembler;
asm
    mov SYSCALL_PM_SLEEP, %eax
    int $0x30
end;

procedure p(); assembler;
asm
    mov SYSCALL_PM_P, %eax
    int $0x30
end;

procedure v(); assembler;
asm
    pushl $0
    mov SYSCALL_PM_V, %eax
    int $0x30
    add $0x4, %esp
end;

procedure set_rpc_handler(rpc_handler: TProcedure); assembler;
asm
    pushl rpc_handler
    mov SYSCALL_SET_RPC_HANDLER, %eax
    int $0x30
    add $0x4, %esp
end;

procedure add_intr_handler(intr: dword); assembler;
asm
    pushl intr
    mov SYSCALL_ADD_INTERRUPT_HANDLER, %eax
    int $0x30
    add $0x4, %esp
end;

procedure rpc(pid: pid_t);
begin { TODO } end;
procedure signal(pid: pid_t; signal: dword);
begin { TODO } end;
procedure send_message(pid: pid_t; fn, correlation_id, len: dword; data: Pointer);
begin { TODO } end;


function create_process(initial_eip: dword; uid: uid_t): pid_t;
begin { TODO } exit(0); end;
procedure destroy_process();
begin { TODO } end;
procedure init_child_page (pid: pid_t; dest, src: Pointer; size: size_t);
begin { TODO } end;
procedure unblock_process(pid: pid_t);
begin { TODO } end;

