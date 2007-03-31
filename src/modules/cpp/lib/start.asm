
extern __libcpp_initialize
extern __libcpp_finalize
extern __libcpp_startup

global start
start:
    call __libcpp_initialize
    
    call __libcpp_startup

    push eax
    call __libcpp_finalize
