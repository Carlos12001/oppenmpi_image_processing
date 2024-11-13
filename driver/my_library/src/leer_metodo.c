// src/leer_metodo.c

#include "../include/mi_biblioteca.h"
#include <stdio.h>

int leer_de_metodo(char *buffer, size_t tamaño) {
    FILE *archivo = fopen("metodo.txt", "r");
    if (archivo == NULL) {
        perror("Error al abrir metodo.txt para leer");
        return -1;
    }

    if (fgets(buffer, tamaño, archivo) == NULL) {
        perror("Error al leer de metodo.txt");
        fclose(archivo);
        return -1;
    }

    fclose(archivo);
    return 0;
}

