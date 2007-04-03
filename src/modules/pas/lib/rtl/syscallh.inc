const
    SYSCALL_KPRINTF = 0;

    SYSCALL_MEM_ALLOCATE = 1;
    SYSCALL_MEM_FREE = 2;

    SYSCALL_PM_CREATE_PROCESS = 3;
    SYSCALL_PM_INIT_PAGE = 13;
    SYSCALL_PM_EXIT_PROCESS = 5;
    SYSCALL_PM_SLEEP = 6;
    SYSCALL_PM_GET_UID = 7;
    SYSCALL_PM_SET_UID = 8;
    SYSCALL_PM_REQUEST_PORT = 9;
    SYSCALL_PM_RELEASE_PORT = 10;

    SYSCALL_PM_P = 11;
    SYSCALL_PM_V = 12;

    SYSCALL_FORTY_TWO = 42;

    SYSCALL_SET_RPC_HANDLER = 50;
    SYSCALL_RPC = 51;
    SYSCALL_ADD_INTERRUPT_HANDLER = 52;

procedure syscall_puts(s: PChar); assembler;
procedure puts(s: PChar);
function mem_allocate(size, flags: dword): pointer; assembler;
procedure yield(); assembler;
procedure p(); assembler;
procedure v(); assembler;
procedure set_rpc_handler(rpc_handler: TProcedure); assembler;
procedure add_intr_handler(intr: dword); assembler;
procedure rpc(pid: pid_t);
procedure signal(pid: pid_t; signal: dword);
procedure send_message(pid: pid_t; fn, correlation_id, len: dword; data: Pointer);
function create_process(initial_eip: dword; uid: uid_t): pid_t;
procedure destroy_process();
procedure init_child_page (pid: pid_t; dest, src: Pointer; size: size_t);
procedure unblock_process(pid: pid_t);
