// src/escribir_dato.h

#include "../include/mi_biblioteca.h"
#include <stdio.h>

int escribir_en_dato(const char *contenido) {
    FILE *archivo = fopen("dato.txt", "w");
    if (archivo == NULL) {
        perror("Error al abrir dato.txt para escribir");
        return -1;
    }

    if (fprintf(archivo, "%s\n", contenido) < 0) {
        perror("Error al escribir en dato.txt");
        fclose(archivo);
        return -1;
    }

    fclose(archivo);
    return 0;
}
