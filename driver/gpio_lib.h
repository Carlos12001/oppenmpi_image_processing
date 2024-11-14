// gpio_lib.h
#ifndef GPIO_LIB_H
#define GPIO_LIB_H

#include <stddef.h>
#include <unistd.h>

int gpio_open(void);
void gpio_close(int fd);
ssize_t gpio_read(int fd, char *buffer, size_t count);
ssize_t gpio_write(int fd, const char *buffer, size_t count);

#endif  // GPIO_LIB_H
