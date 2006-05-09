; **********************************************************

;	Name: Task Switcher
;	Autor: Toaster Burger
;	Version: 1.00
;	Date: 26.11.2005
;	last Update: 26.11.2005
;	see document: ToasterOS.pdf/Lost specifications

; **********************************************************

[bits 32]
CPU 386

%include "Kernel modules & buffer storage.asm"
%include "Public Memory Manager Macros.asm"

%include "System.asm"
%include "User.asm"
%include "Errors.asm"

%include "Time.asm"

%define	Handle_Size		06h

org Task_Switcher


; set the gates for the functions to export

global Enable_Scheduler
global Create_Task
global Delete_Task
global Destroy_Task
global Check_Task_Handle
global Dispatcher






Enable_Scheduler:					; enable (start) the Scheduler

; API Enable_Scheduler

Enter_System_Environment

; OBSOLETE mov [Current_Task],dword Task_Table-Handle_Size
mov [Current_Task],dword Task_Table
xor ecx,ecx						; loop counter = 65536
xor eax,eax
mov edi,Task_Table

rep stosw						; erase all words
rep stosd						; erase all dwords

Leave_System_Environment

ret






Create_Task:

; API Create_Task, Handle, Process, Stack, Address, Data, Size

; Handle = Process Handle (of the caller's one)
; Process = Handle of the Process to start the Task
; Stack = Stack Pointer (esp) of the new task, (from low to high, value is low)
; Address = start address (eip) of the new task
; Size = Bytes to copy from Data (source) to the new Tasks stack (destination)

; NOTES:
; The Data is bytewise copied from low to high, and the new Task's stack pointer will point BEFORE the data.
; If the Task is created in an existing Process (with at least one task) allocate the stack from the Heap
; with the size of 1024 bytes at least, excluding user specific data.
; If the Task is created in addition with executing a new Process, let point the stack pointer to the Public
; Buffer in the UDA.
; Don't use the Public Buffer for the data, because it will be used for the stack.
; But remember you can use the Public Buffer for the indirect stack pointer, which points to the other VAS.
; The Task Handle will be pushed directly onto the stack of the new Task.
; So the caller won't get the Handle.

Enter_System_Environment

Search_Task_Handle:					; search for a free Task Handle
mov esi,Task_Table
xor ecx,ecx						; loop counter = 65536
Enter_Multitasking MT_Task_Switcher			; coordinate Multitasking access (enter)

Search_free_Task:
add esi,02h

lodsd
or eax,eax
jz Task_found
loop Search_free_Task

; if here, there is no free Task Handle
Leave_Multitasking MT_Task_Switcher			; coordinate Multitasking access (leave)
stc
mov eax,No_free_Task
jmp Create_Task_Exit


Task_found:						; if comes here, a free Task Handle is found
Leave_Multitasking MT_Task_Switcher			; coordinate Multitasking access (leave)
lea edi,[esi-06h]

; store the Process Handle
mov ax,[Param2]
stosw

; store the Stack Pointer (all values will be pushed after this boundary)
mov eax,[Param3]
stosd



; set the Stack (see the Task Switcher's dispatcher)
; push following values in the correct direction: iret, ds, es, pushad, Handle (12, 4, 4, 32, 4)
; remember, the stack has tu put into the (maybe) other tasks VAS, so use the copy function of the Memory Manager
; as buffer, use the Public Buffer in the UDA

Enter_Multithreading MT_Task_Switcher, 0		; coordinate Multithreading access (enter)
mov edi,Public_Buffer					; edi = pointer to the public buffer

; set for the iret eip
mov eax,[Param4]
stosd

; set for the iret cs (user)
mov eax,Code_Selector_User
stosd

; set for the iret the flags (user)
mov eax,User_EFLAG
stosd

; set ds & es (user)
mov eax,Data_Selector_User
stosd
stosd


; following values points on start to the stack, causes that's the only way to avoid an #GP on an undefinied access
; set edi, esi, ebp & esp (placeholder)
mov eax,[Param3]
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

; store the Task Handle
sub eax,ecx						; 65536 - loop counter
stosd

; copy the stack
API Copy_Process_Data, word [Param1], word [Param2], dword Public_Buffer, dword [Param3], dword 56
Leave_Multithreading MT_Task_Switcher, 0		; coordinate Multithreading access (leave)
jc Create_Task_Exit

; update the (thought) stack pointer
add [Param3],dword 56


; check if data is to copy (or not)
cmp [Param6],dword 00h
je Create_Task_Exit_success

; copy the data
API Copy_Process_Data, word [Param1], word [Param2], dword [Param5], dword [Param3], dword [Param6]
jc Create_Task_Exit


Create_Task_Exit_success:
clc


Create_Task_Exit:
Leave_System_Environment

ret






Delete_Task:

; API Delete_Task, Handle

; Handle = Handle of the Task to delete; returned by Create_Task

Enter_System_Environment

; edi = address of the Handle
imul edi,[Param1],Handle_Size
add edi,Task_Table

xor eax,eax
stosw
stosd

Leave_System_Environment

ret






Destroy_Task:

; API Destroy_Task, Task_ID

; (undocumented)

Enter_System_Environment

mov esi,Task_Table
xor ecx,ecx						; loop counter = 65536
mov bx,[Param1]

Destroy_free_Task:
lodsw
cmp ax,bx
jne Destroy_free_Task_nt

mov [esi-2],word 00h
mov [esi],dword 00h

Destroy_free_Task_nt:
add esi,04h
loop Destroy_free_Task

Leave_System_Environment

ret






Dispatcher:

; 1. store the current Task State
pushad
;pushfd
mpush ds, es

; store esp
mov ebx,[Current_Task]
mov [ebx+2],esp

; 2. execute the (internal) Scheduler
Scheduler:						; the scheduler for the Task Switcher
add [Current_Task],dword Handle_Size
cmp [Current_Task],dword Task_Table_End
jl No_Swap

mov [Current_Task],dword Task_Table

No_Swap:
mov ebx,[Current_Task]
mov eax,[ebx+2]
or eax,eax
jz Scheduler

; restore sp
mov esp,[ebx+2]

; 3. restore the new Task State
API Get_CR3, word [ebx]
mov cr3,eax

mpop ds, es
;popfd
popad

; send the EOI
EOI_Master

iret






; following Scheduler is obsolete

Scheduler_External:						; the scheduler for the Task Switcher
add [Current_Task],dword Handle_Size
cmp [Current_Task],dword Task_Table_End
jne No_Swap_External

mov [Current_Task],dword Task_Table

No_Swap_External:
mov EDI,[Current_Task]
mov EAX,[EDI+2]
cmp EAX,00h
je Scheduler

ret






Check_Task_Handle:

; API Check_Task_Handle, Handle, Process_Handle

; Handle = Handle of the Task to check of validity; returned by Create_Task
; Process Handle = Process Handle of the Task (have to be the same as by Create_Task)

Enter_System_Environment

; ebx = address of the Handle
imul ebx,[Param1],Handle_Size
add edi,Task_Table

; check the boundarys
cmp [Param1],dword Task_Table_End
jnc Check_Task_Handle_Invalid

; check if the Task_Handle(current).Process_Handle is euqal to the as parameter's one
mov ax,[ebx]
cmp ax,[Param2]
jne Check_Task_Handle_Invalid

; esp = 0 ?
cmp [ebx+2],TPointer NULL
je Check_Task_Handle_Invalid

; Process Handle valid?
API Check_Process_Handle, TProcessHandle [Param2]
jc Check_Task_Handle_Exit

xor eax,eax
clc
jmp Check_Task_Handle_Exit


Check_Task_Handle_Invalid:
stc
mov eax,Invalid_Handle


Check_Task_Handle_Exit:
Leave_System_Environment

ret






; some public data
Current_Task	dd	00h


; the Task Table contains information about the Tasks, the descriptor format:

; Process Handle        (word)
; esp                   (dword)
; ------------------------------
; a Task description    6 Bytes
