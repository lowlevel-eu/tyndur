global hello_start
hello_start:
align 4096
incbin "../../../build/hello.mod"
hello_end: