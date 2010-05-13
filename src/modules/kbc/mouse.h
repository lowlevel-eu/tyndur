
#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init(void);


void mouse_wait(uint8_t a_type);
void mouse_write(uint8_t a_write);
uint8_t mouse_read(void);
void mouse_install(void);

void mouse_irq_handler(uint8_t irq);

size_t mouse_read_handler(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount);

size_t mouse_callback_handler(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount, void *data);

void mouse_input_reset(void);

#endif
