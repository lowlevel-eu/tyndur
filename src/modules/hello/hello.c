void _start() {
  int i;

  while(1) {
    asm("int $0x30");
    for (i = 0; i < 100000; i++);
  }
}
