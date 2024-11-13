// gpio_lib.c
#include "gpio_lib.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define DEVICE "/dev/my_gpio_driver"

int gpio_open(void) {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Error al abrir el dispositivo");
    }
    return fd;
}

void gpio_close(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

ssize_t gpio_read(int fd, char *buffer, size_t count) {
    if (fd < 0) {
        fprintf(stderr, "Descriptor de archivo inválido\n");
        return -1;
    }
    ssize_t ret = read(fd, buffer, count);
    if (ret < 0) {
        perror("Error al leer desde el dispositivo");
    }
    return ret;
}

ssize_t gpio_write(int fd, const char *buffer, size_t count) {
    if (fd < 0) {
        fprintf(stderr, "Descriptor de archivo inválido\n");
        return -1;
    }
    ssize_t ret = write(fd, buffer, count);
    if (ret < 0) {
        perror("Error al escribir en el dispositivo");
    }
    return ret;
}
