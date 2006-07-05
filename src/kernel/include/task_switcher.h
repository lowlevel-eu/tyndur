#ifndef TASK_SWITCHER_H 
#define TASK_SWITCHER_H 

void init_taskswitcher();
void Create_Task_System(dword handle, dword process, dword initial_esp, dword initial_eip, void* data, dword data_size);
void Create_Task(dword handle, dword process, dword initial_esp, dword initial_eip, void* data, dword data_size);
void Destroy_Task(dword handle);
void Dispatcher();
void Switch_Task();
int Check_Task_Handle(dword handle, dword process);
word Get_Current_Process();

#endif 
