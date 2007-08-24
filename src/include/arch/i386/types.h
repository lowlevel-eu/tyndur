#ifndef _TYPES_H_
#define _TYPES_H_

#define NULL (void*) 0

#ifndef __cplusplus

// FIXME: HACK
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

typedef enum { FALSE = 0, TRUE } bool;
#endif

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long qword;

typedef signed char sbyte;
typedef signed short sword;
typedef signed int sdword;
typedef signed long long sqword;

typedef __SIZE_TYPE__ size_t;
typedef __SIZE_TYPE__ ssize_t;

typedef void * paddr_t;
typedef void * vaddr_t;

typedef dword pid_t;
typedef dword uid_t;
typedef dword gid_t;

typedef qword timestamp_t;

typedef dword syscall_id_t;

#endif
