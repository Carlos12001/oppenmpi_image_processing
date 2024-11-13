// ejemplos/ejemplo.c

#include "../include/mi_biblioteca.h"
#include <stdio.h>

int main() {
    // Escribir en dato.txt
    if (escribir_en_dato("123") != 0) {
        fprintf(stderr, "Error al escribir en dato.txt\n");
        return 1;
    }
    printf("Se escribió '123' en dato.txt correctamente.\n");

    // Leer de metodo.txt
    char buffer[256];
    if (leer_de_metodo(buffer, sizeof(buffer)) != 0) {
        fprintf(stderr, "Error al leer de metodo.txt\n");
        return 1;
    }
    printf("Contenido de metodo.txt: %s", buffer);

    return 0;
}

