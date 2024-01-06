/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/
#include "message-private.h"
#include <unistd.h>

ssize_t read_all(int socket, void *buffer, size_t size) {
    size_t bytes_read = 0;

    while (bytes_read < size) {
        size_t result = read(socket, buffer + bytes_read, size - bytes_read);
        if (result == -1) {
            return -1; // ERROR READING
        } else if (result == 0) {
            return bytes_read; // CONNECTION CLOSED
        }
        bytes_read += result;
    }
    return bytes_read;
}

ssize_t write_all(int socket, const void *buffer, size_t size) {
    size_t bytes_written = 0;

    while (bytes_written < size) {
       ssize_t result = write(socket, buffer + bytes_written, size - bytes_written);
        if (result == -1) {
            return -1; // ERROR WRITING
        }
        bytes_written += result;
    }
    return bytes_written;
    }