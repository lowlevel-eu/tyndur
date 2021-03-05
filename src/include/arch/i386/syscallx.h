#ifndef SYSCALLS_H
#define SYSCALLS_H

#define SYSCALL0_TWO_RESULTS(SYSCALL_NO, RESULT1, RESULT2) \
    asm volatile (                  \
        "mov %2, %%eax;"            \
        "int $0x30;"                \
      : "=a" (RESULT1),             \
        "=d" (RESULT2)              \
      : "i" (SYSCALL_NO) : "memory")

#define SYSCALL0(SYSCALL_NO, RESULT) \
    asm volatile (                  \
        "mov %1, %%eax;"            \
        "int $0x30;"                \
      : "=a" (RESULT)               \
      : "i" (SYSCALL_NO) : "memory")

#define SYSCALL1(SYSCALL_NO, ARG1, RESULT) \
    asm volatile (                  \
        "pushl %2;"                 \
        "mov %1, %%eax;"            \
        "int $0x30;"                \
        "add $0x4, %%esp;"          \
      : "=a" (RESULT)               \
      : "i" (SYSCALL_NO),           \
        "r" (ARG1) : "memory")

#define SYSCALL2(SYSCALL_NO, ARG1, ARG2, RESULT) \
    asm volatile (                  \
        "push %3;"                  \
        "push %2;"                  \
        "mov %1, %%eax;"            \
        "int $0x30;"                \
        "add $0x8, %%esp;"          \
      : "=a" (RESULT)               \
      : "i" (SYSCALL_NO),           \
        "r" (ARG1),                 \
        "r" (ARG2) : "memory")

#define SYSCALL3(SYSCALL_NO, ARG1, ARG2, ARG3, RESULT) \
    asm volatile (                  \
        "push %4;"                  \
        "push %3;"                  \
        "push %2;"                  \
        "mov %1, %%eax;"            \
        "int $0x30;"                \
        "add $0xc, %%esp;"          \
      : "=a" (RESULT)               \
      : "i" (SYSCALL_NO),           \
        "r" (ARG1),                 \
        "r" (ARG2),                 \
        "r" (ARG3) : "memory")

#define SYSCALL4(SYSCALL_NO, ARG1, ARG2, ARG3, ARG4, RESULT) \
    asm volatile (                  \
        "push %5;"                  \
        "push %4;"                  \
        "push %3;"                  \
        "push %2;"                  \
        "mov %1, %%eax;"            \
        "int $0x30;"                \
        "add $0x10, %%esp;"         \
      : "=a" (RESULT)               \
      : "i" (SYSCALL_NO),           \
        "r" (ARG1),                 \
        "r" (ARG2),                 \
        "r" (ARG3),                 \
        "r" (ARG4) : "memory")

#define SYSCALL5(SYSCALL_NO, ARG1, ARG2, ARG3, ARG4, ARG5, RESULT) \
    asm volatile (                  \
        "push %6;"                  \
        "push %5;"                  \
        "push %4;"                  \
        "push %3;"                  \
        "push %2;"                  \
        "mov %1, %%eax;"            \
        "int $0x30;"                \
        "add $0x14, %%esp;"         \
      : "=a" (RESULT)               \
      : "i" (SYSCALL_NO),           \
        "r" (ARG1),                 \
        "r" (ARG2),                 \
        "r" (ARG3),                 \
        "r" (ARG4),                 \
        "r" (ARG5) : "memory")

#define SYSCALL6(SYSCALL_NO, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, RESULT) \
    asm volatile (                  \
        "push %7;"                  \
        "push %6;"                  \
        "push %5;"                  \
        "push %4;"                  \
        "push %3;"                  \
        "push %2;"                  \
        "mov %1, %%eax;"            \
        "int $0x30;"                \
        "add $0x18, %%esp;"         \
      : "=a" (RESULT)               \
      : "i" (SYSCALL_NO),           \
        "r" (ARG1),                 \
        "r" (ARG2),                 \
        "r" (ARG3),                 \
        "r" (ARG4),                 \
        "r" (ARG5),                 \
        "g" (ARG6) : "memory")

#endif /* SYSCALLS_H */
