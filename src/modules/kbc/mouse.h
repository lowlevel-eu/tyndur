
#ifndef MOUSE_H
#define MOUSE_H

void mouse_init(void);


void mouse_wait(byte a_type);
void mouse_write(byte a_write);
byte mouse_read();
void mouse_install();

void mouse_irq_handler();

size_t mouse_read_handler(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount);

size_t mouse_callback_handler(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount, void *data);

void mouse_input_reset();

#endif
