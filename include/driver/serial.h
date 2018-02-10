#ifndef SERIAL_H
#define SERIAL_H

extern void serial_initialize();
extern void serial_write(char a);
extern void serial_writes(const char* data);
extern char serial_read();

#endif