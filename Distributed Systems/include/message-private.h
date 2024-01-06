#ifndef MESSAGE_H
#define MESSAGE_H

#include <unistd.h>

ssize_t read_all(int socket, void *buffer, size_t size);
ssize_t write_all(int socket, const void *buffer, size_t size);

#endif

