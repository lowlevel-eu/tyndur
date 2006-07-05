#ifndef TASK_SWITCHER_H 
#define TASK_SWITCHER_H 

void init_taskswitcher();
void Create_Task_System(dword pid, dword parent_pid, dword initial_esp, dword initial_eip, void* data, dword data_size);
void Create_Task(dword pid, dword parent_pid, dword initial_esp, dword initial_eip, void* data, dword data_size);
void Destroy_Task(dword pid);
void Dispatcher();
void Switch_Task();
int Check_Task_Handle(dword pid, dword parent_pid);
word Get_Current_Process();

#endif 
