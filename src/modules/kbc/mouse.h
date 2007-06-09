
#ifndef MOUSE_H
#define MOUSE_H

void mouse_init(void);


void mouse_wait(byte a_type);
void mouse_write(byte a_write);
byte mouse_read();
void mouse_install();

void mouse_irq_handler();

read_hdl_reply  mouse_read_handler(lostio_filehandle_t* filehandle,
                                   size_t blocksize,
                                   size_t blockcount);

#endif
