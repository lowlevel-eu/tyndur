; **********************************************************

;	Name: Task Switcher port-able
;	Autor: Toaster Burger
;	Version: 1.00 port-able
;	Date: Monday, 3rd July 2006
;	last Update: Monday, 3rd July 2006

; **********************************************************



; global the functions
global init_taskswitcher
global Create_Task_System
global Create_Task
global Destroy_Task
global Destroy_Tasks
global Dispatcher
global Switch_Task
global Check_Task_Handle
global Get_Current_Process




; some definitions and macros
%define	Handle_Size		6

%define	User_EFLAG		000000001000000010b
%define	Code_Selector		0000000000001000b
%define	Code_Selector_User	0000000000011011b
%define	Data_Selector		0000000000010000b
%define	Data_Selector_User	0000000000100011b

; API function numbers (using int 80h)
%define	Copy_Process_Data	0
%define	Get_CR3			0
%define	Check_Process_Handle	0

; errors
%define	No_free_Task		34h
%define	Invalid_Handle		02h


; first Param = ebp + 8 [= eip, ebp]
%define	Param(number)	[ebp+8 + 4*(number-1)]
%define	Param1	[ebp+8 + 4*0]
%define	Param2	[ebp+8 + 4*1]
%define	Param3	[ebp+8 + 4*2]
%define	Param4	[ebp+8 + 4*3]
%define	Param5	[ebp+8 + 4*4]
%define	Param6	[ebp+8 + 4*5]


%macro	mpush 1-*.nolist

	%rep %0

		push %1

	%rotate 1
	%endrep

%endmacro


%macro	mpop 1-*.nolist

	%rep %0
	%rotate -1

		pop %1

	%endrep

%endmacro


%macro	API 2-*.nolist

	; API Interrupt, Function, Param1, ParamN

	; push the Params in the C Calling Convention
	%rotate	%0-2
	%rep %0-2
	
	push dword %1
	
	%rotate -1
	%endrep

	mov eax,%2
	int %1

%endmacro


%macro	Enter_MT 1-2.nolist

; parameters:
; %1 = public MT variable (in Kernel memory)

%if %0=1

%%MT_wait:
bts dword [%1],byte 0
jnc %%MT_wait

%else

%%MT_wait:
bts dword [%1],byte %2
jnc %%MT_wait

%endif
%endmacro


%macro	Leave_MT 1-2.nolist

; parameters:
; %1 = public MT variable (in Kernel memory)

%if %0=1

pushfw
btr dword [%1],byte 0
popfw

%else

pushfw
btr dword [%1],byte %2
popfw

%endif
%endmacro






section .data

MT_Task_Switcher	dd	0
dispatcher_state	dd	0

; Current_Task is a direct pointer (into the Task Table) to the current Task
Current_Task		dd	0
FPU_MMX_SSE		dd	"no"






section .bss

; define the Task Table (384 KB)
Task_Table		resb 	(65536 * 6)
%define	Task_Table_End		Task_Table + (65536 * 6)

; the Task Table contains information about the Tasks, the descriptor format:

; Process Handle        (word)
; esp                   (dword)
; ------------------------------
; a Task description    6 Bytes

FPU_MMX_SSE2_State	resb	512





section .text




; initialise initialises the Task Switcher and sets valid (default) values

init_taskswitcher:

; API initialise

; create variables on the stack
enter 0,0

; store registers
mpush esi, edi, ebx, ecx, edx


; command CPUID available (test bit 21 of eflags to set) ?
pushfd
or [esp],dword 1000000000000000000000b
popfd
pushfd
pop eax
test eax,1000000000000000000000b
jz No_FPU_MMX_SSE2_support

; check if the FPU, MMX, SSE (re)store command is available
mov eax,1
CPU PentiumPro
cpuid
CPU 386
test edx,1000000000000000000000000b
jz No_FPU_MMX_SSE2_support
jmp Write_Task_Table


No_FPU_MMX_SSE2_support:
; if here the fxsave/fxrstor opcodes are not supported, so delete them
mov [Store_FPU_MMX_SSE_state],dword 90909090h
mov [Store_FPU_MMX_SSE_state+4],word 9090h
mov [Store_FPU_MMX_SSE_state+6],byte 90h
mov [Restore_FPU_MMX_SSE_state],dword 90909090h
mov [Restore_FPU_MMX_SSE_state+4],word 9090h
mov [Restore_FPU_MMX_SSE_state+6],byte 90h


Write_Task_Table:
mov [Current_Task],dword Task_Table
xor ecx,ecx						; loop counter = 65536
xor eax,eax
mov edi,Task_Table

rep stosw						; erase all words
rep stosd						; erase all dwords


; restore registers & leave the stack
mpop esi, edi, ebx, ecx, edx
leave

ret






; creates a system (for PL0) task

Create_Task_System:

; API Create_Task_System, Handle, Process, Stack, Address, Data, Size

; Handle = Process Handle (of the caller's one)
; Process = Process Handle of the Process to start the Task
; Stack = start stack (esp) of the new task (the value stack is from high to low)
; Address = start address (eip) of the new task
; Size = bytes to copy from Data (source) to the new Tasks stack (destination)

; NOTES:
; The Data is bytewise copied to the stack from low to high, and the new Task's 
; stack pointer will point BEFORE the data.
; This function adds to the data the Task Handle, so esp+0 points to the Handle
; and esp+4 to the user sepcific data.
; The caller won't get the Handle.

; create variables on the stack
enter 0,0

; store registers
mpush esi, edi, ebx, ecx, edx


; set the correct Stack Pointer
mov eax,Param6
sub Param3,eax



; search a free Task Handle
mov esi,Task_Table
xor ecx,ecx						; loop counter = 65536
Enter_MT MT_Task_Switcher

Search_free_System_Task:
add esi,02h

lodsd
or eax,eax
jz Task_System_found
loop Search_free_System_Task

; if here, there is no free Task Handle
Leave_MT MT_Task_Switcher
stc
mov eax,No_free_Task
jmp Create_Task_System_Exit


Task_System_found:					; if comes here, a free Task Handle is found
Leave_MT MT_Task_Switcher
lea edi,[esi-06h]

; store the Process Handle
mov ax,Param2
stosw

; store the Stack Pointer
mov eax,Param3
sub eax,12+(4*5)+32+108
stosd



; set the stack for the new process on the current stack
; push following values in the correct direction: 
;     gs, fs, es, ds, ad, FPU, iret, Handle (4, 4, 4, 4, 32, 108, 12, 4)

sub esp,12+(4*5)+32+108
mov edi,esp
mov ebx,esp

; set gs, fs, es, ds
xor eax,eax
stosd
stosd
mov eax,Data_Selector
stosd
stosd

; set edi, esi, ebp & esp (all points to the start of stack)
mov eax,Param3
stosd
stosd
stosd
stosd

; xor eax, ebx, ecx & edx
xor eax,eax
stosd
stosd
stosd
stosd

; FPU registers (use current)
fsave [edi]
add edi,108

; store the iret (eip, cs, flags)
mov eax,Param4
stosd
mov eax,Code_Selector
stosd
mov eax,User_EFLAG
stosd

; store the Task Handle
xor eax,eax
sub eax,ecx						; 65536 - loop counter
stosd


; check whether the data is to copy (or not)
cmp Param6,dword 0
je Copy_System_finished

; copy the data
API 80h, Copy_Process_Data, Param1, Param2, Param5, Param3, Param6
jc Create_Task_System_Exit

Copy_System_finished:


; copy the (system) stack
sub Param3,dword (12+(4*5)+32+108)
API 80h, Copy_Process_Data, Param1, Param2, ebx, Param3, (12+(4*5)+32+108)



; reset the stack (delete temporary other process stack)
add esp,12+(4*5)+32+108


Create_Task_System_Exit:
; restore registers & leave the stack
mpop esi, edi, ebx, ecx, edx
leave

ret






; creates a user (for PL3) task

Create_Task:

; API Create_Task, Handle, Process, Stack, Address, Data, Size

; Handle = Process Handle (of the caller's one)
; Process = Process Handle of the Process to start the Task
; Stack = start stack (esp) of the new task (the value stack is from high to low)
; Address = start address (eip) of the new task
; Size = bytes to copy from Data (source) to the new Tasks stack (destination)

; NOTES:
; The Data is bytewise copied to the stack from low to high, and the new Task's 
; stack pointer will point BEFORE the data.
; This function adds to the data the Task Handle, so esp+0 points to the Handle
; and esp+4 to the user sepcific data.
; The caller won't get the Handle.

; create variables on the stack
enter 0,0

; store registers
mpush esi, edi, ebx, ecx, edx


; set the correct Stack Pointer
mov eax,Param6
sub Param3,eax



; search a free Task Handle
mov esi,Task_Table
xor ecx,ecx						; loop counter = 65536
Enter_MT MT_Task_Switcher

Search_free_Task:
add esi,02h

lodsd
or eax,eax
jz Task_found
loop Search_free_Task

; if here, there is no free Task Handle
Leave_MT MT_Task_Switcher
stc
mov eax,No_free_Task
jmp Create_Task_Exit


Task_found:						; if comes here, a free Task Handle is found
Leave_MT MT_Task_Switcher
lea edi,[esi-06h]

; store the Process Handle
mov ax,Param2
stosw

; store the Stack Pointer
mov eax,Param3
sub eax,12+(4*5)+32+108
stosd



; set the stack for the new process on the current stack
; push following values in the correct direction: 
;     gs, fs, es, ds, ad, FPU, iret, Handle (4, 4, 4, 4, 32, 108, 12, 4)

sub esp,12+(4*5)+32+108
mov edi,esp
mov ebx,esp

; set gs, fs, es, ds
xor eax,eax
stosd
stosd
mov eax,Data_Selector_User
stosd
stosd

; set edi, esi, ebp & esp (all points to the start of stack)
mov eax,Param3
stosd
stosd
stosd
stosd

; xor eax, ebx, ecx & edx
xor eax,eax
stosd
stosd
stosd
stosd

; FPU registers (use current)
fsave [edi]
add edi,108

; store the iret (eip, cs, flags)
mov eax,Param4
stosd
mov eax,Code_Selector_User
stosd
mov eax,User_EFLAG
stosd

; store the Task Handle
xor eax,eax
sub eax,ecx						; 65536 - loop counter
stosd


; check whether the data is to copy (or not)
cmp Param6,dword 0
je Copy_finished

; copy the data
API 80h, Copy_Process_Data, Param1, Param2, Param5, Param3, Param6
jc Create_Task_Exit

Copy_finished:


; copy the (system) stack
sub Param3,dword (12+(4*5)+32+108)
API 80h, Copy_Process_Data, Param1, Param2, ebx, Param3, (12+(4*5)+32+108)



; reset the stack (delete temporary other process stack)
add esp,12+(4*5)+32+108


Create_Task_Exit:
; restore registers & leave the stack
mpop esi, edi, ebx, ecx, edx
leave

ret







; destroys a task

Destroy_Task:

; API Delete_Task, Handle

; Handle = Handle of the Task to delete; returned (on the stack) by Create_Task

; create variables on the stack
enter 0,0

; store registers
mpush esi, edi, ebx, ecx, edx


; edi = address of the Handle
imul edi,Param1,Handle_Size
add edi,Task_Table

xor eax,eax
stosw
stosd


; restore registers & leave the stack
mpop esi, edi, ebx, ecx, edx
leave

ret







Destroy_Tasks:

; (undocumented)

; create variables on the stack
enter 0,0

; store registers
mpush esi, edi, ebx, ecx, edx


mov esi,Task_Table
xor ecx,ecx						; loop counter = 65536
mov bx,Param1

Destroy_free_Task:
lodsw
cmp ax,bx
jne Destroy_free_Task_nt

mov [esi-2],word 0
mov [esi],dword 0

Destroy_free_Task_nt:
add esi,04h
loop Destroy_free_Task


; restore registers & leave the stack
mpop esi, edi, ebx, ecx, edx
leave

ret







; the dispatcher, switches between Tasks

Dispatcher:

Enter_MT	dispatcher_state


; 1. store the current Task's state

sub esp,108
fsave [esp]
pushad
mpush ds, es, fs, gs

; store special FPU, MMX and SSE state or do nop
Store_FPU_MMX_SSE_state:
CPU PentiumPro
fxsave [FPU_MMX_SSE2_State]
CPU 386

; store the stack
mov ebx,[Current_Task]
mov [ebx+2],esp


; 2. execute the (internal) Scheduler
Scheduler:
add [Current_Task],dword Handle_Size
cmp [Current_Task],dword Task_Table_End
jl Scheduler_Next

mov [Current_Task],dword Task_Table

Scheduler_Next:
mov ebx,[Current_Task]
mov eax,[ebx+2]
or eax,eax
jz Scheduler

; restore the stack
mov esp,[ebx+2]


; 3. restore the new Task's state
API 80h, Get_CR3, [ebx]
mov cr3,eax

; restore special FPU, MMX and SSE state or do nop
Restore_FPU_MMX_SSE_state:
CPU PentiumPro
fxrstor [FPU_MMX_SSE2_State]
CPU 386

mpop ds, es, fs, gs
popad
frstor [esp]
add esp,108


Leave_MT	dispatcher_state

ret







Switch_Task:

; (undocumented)

; create variables on the stack
enter 0,0


; set the return stack (for iret)
pushfd
push cs
push Switch_Task_ret

; switch the task
call Dispatcher

iret


Switch_Task_ret:
; leave the stack
leave

ret







; validates the Task Handle

Check_Task_Handle:

; API Check_Task_Handle, Handle, Process_Handle

; Handle = Handle of the Task to check of validity; returned (on the stack) by Create_Task
; Process Handle = Process Handle of the Task (have to be the same as by Create_Task)

; create variables on the stack
enter 0,0

; store registers
mpush esi, edi, ebx, ecx, edx


; ebx = address of the Handle
imul ebx,Param1,Handle_Size
add edi,Task_Table

; check the boundarys
cmp Param1,dword Task_Table_End
jnc Check_Task_Handle_Invalid

; check if the Task_Handle(current).Process_Handle is euqal to the as parameter's one
mov ax,[ebx]
cmp ax,Param2
jne Check_Task_Handle_Invalid

; esp = 0 ?
cmp [ebx+2],word 0
je Check_Task_Handle_Invalid

; Process Handle valid?
API 80h, Check_Process_Handle, TProcessHandle [Param2]
jc Check_Task_Handle_Invalid

; if here the handle is valid
mov eax,1
jmp Check_Task_Handle_Exit


Check_Task_Handle_Invalid:
xor eax,eax


Check_Task_Handle_Exit:
; restore registers & leave the stack
mpop esi, edi, ebx, ecx, edx
leave

ret






; returns the current Process Handle, internal use only!
;   usefull for the messaging system

Get_Current_Process:

; (undocumented)

; create variables on the stack
enter 0,0

; store registers
mpush esi

mov esi,[Current_Task]
movzx eax,word [esi]

; restore registers & leave the stack
mpop esi
leave

ret
