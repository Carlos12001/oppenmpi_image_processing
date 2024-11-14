#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 1024

// Función para enviar el archivo al servidor
void enviar_archivo(int sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("No se pudo abrir el archivo");
        exit(EXIT_FAILURE);
    }

    char buffer[BUF_SIZE];
    size_t bytes_leidos;

    // Leer el archivo y enviarlo en fragmentos
    while ((bytes_leidos = fread(buffer, 1, BUF_SIZE, file)) > 0) {
        send(sock, buffer, bytes_leidos, 0);
    }

    printf("Archivo enviado correctamente.\n");

    fclose(file);
}

int main() {
    int sock;
    struct sockaddr_in server_address;
    char archivo[256];  // Buffer para la ruta del archivo

    printf("Ingrese el nombre o la ruta del archivo a enviar (default: goku.bmp): ");
    fgets(archivo, sizeof(archivo), stdin);
    archivo[strcspn(archivo, "\n")] = '\0';  // Eliminar el salto de línea de la entrada

    // Crear el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("No se pudo crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Dirección inválida o no soportada");
        exit(EXIT_FAILURE);
    }

    // Conectarse al servidor
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Conexión fallida");
        exit(EXIT_FAILURE);
    }

    // Enviar el archivo al servidor
    enviar_archivo(sock, archivo);

    close(sock);
    return 0;
}
