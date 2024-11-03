#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define DEVICE "/dev/my_gpio_driver"
#define MODE_FILE "modo.txt"

/* Variable para almacenar el descriptor del dispositivo */
int device_fd = -1;

/* Manejador de señales */
void signal_handler(int signo) {
    if (signo == SIGIO) {
        // Abrir el archivo modo.txt
        FILE *file = fopen(MODE_FILE, "r+");
        if (file == NULL) {
            perror("Error al abrir modo.txt");
            return;
        }

        // Leer el valor actual
        int value = 0;
        if (fscanf(file, "%d", &value) != 1) {
            // Si el archivo está vacío o contiene datos no válidos, inicializar a 0
            value = 0;
        }

        // Incrementar el valor
        value++;

        // Volver al inicio del archivo
        fseek(file, 0, SEEK_SET);

        // Escribir el nuevo valor
        fprintf(file, "%d\n", value);

        // Cerrar el archivo
        fclose(file);

        printf("Botón presionado. modo.txt actualizado a %d\n", value);
    }
}

int main() {
    /* Abrir el dispositivo en modo no bloqueante y para recibir señales */
    device_fd = open(DEVICE, O_RDONLY | O_NONBLOCK);
    if (device_fd < 0) {
        perror("Error al abrir el dispositivo");
        return EXIT_FAILURE;
    }

    /* Configurar el proceso para recibir señales SIGIO */
    if (fcntl(device_fd, F_SETOWN, getpid()) < 0) {
        perror("Error al establecer propietario de la señal");
        close(device_fd);
        return EXIT_FAILURE;
    }

    /* Configurar las banderas de archivo para recibir SIGIO */
    int flags = fcntl(device_fd, F_GETFL);
    if (fcntl(device_fd, F_SETFL, flags | O_ASYNC) < 0) {
        perror("Error al establecer O_ASYNC");
        close(device_fd);
        return EXIT_FAILURE;
    }

    /* Registrar el manejador de señales */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGIO, &sa, NULL) == -1) {
        perror("Error al registrar el manejador de señales");
        close(device_fd);
        return EXIT_FAILURE;
    }

    printf("Programa iniciado. Esperando presiones de botón...\n");

    /* Mantener el programa en ejecución */
    while (1) {
        pause(); // Esperar a que llegue una señal
    }

    /* Nunca se alcanza, pero por buenas prácticas */
    close(device_fd);
    return EXIT_SUCCESS;
}

