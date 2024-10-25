#include <stdio.h>
#include <stdlib.h>

// Función para aplicar el cifrado/descifrado de César solo a los datos de píxeles
void cifradoCesarByte(unsigned char *data, int start, int end, int desplazamiento) {
    for (int i = start; i < end; i++) {
        data[i] = (data[i] + desplazamiento) % 256;  // Asegurar que esté en rango 0-255
    }
}

int main() {
    unsigned char *data = NULL;  // Datos a cifrar o descifrar (archivo binario)
    int desplazamiento;          // Desplazamiento para el cifrado/descifrado
    int dataSize = 0;            // Tamaño de los datos
    int opcion;                  // Opción para elegir entre cifrar o descifrar
    int headerSize = 54;         // Tamaño del encabezado de la imagen BMP (primeros 54 bytes)
    
    FILE *inputFile, *outputFile;

    // Solicitar si desea cifrar o descifrar
    printf("Elige una opción: (1) Cifrar, (2) Descifrar: ");
    fflush(stdout);
    scanf("%d", &opcion);

    // Solicitar el desplazamiento
    printf("Introduce el valor del desplazamiento (entre 1 y 255): ");
    fflush(stdout);
    scanf("%d", &desplazamiento);

    // Validar desplazamiento para que esté dentro del rango de 1 a 255
    if (desplazamiento < 1 || desplazamiento > 255) {
        printf("Desplazamiento fuera de rango. Debe estar entre 1 y 255.\n");
        return 1;
    }

    // Si se selecciona la opción de descifrar, se usa un desplazamiento negativo
    if (opcion == 2) {
        desplazamiento = -desplazamiento;
        inputFile = fopen("goku_cifrada.bmp", "rb");
        if (inputFile == NULL) {
            printf("Error al abrir la imagen cifrada.\n");
            return 1;
        }
    } else {
        inputFile = fopen("goku.bmp", "rb");  // Abrir el archivo original
        if (inputFile == NULL) {
            printf("Error al abrir la imagen.\n");
            return 1;
        }
    }

    // Obtener el tamaño del archivo
    fseek(inputFile, 0, SEEK_END);
    dataSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);  // Regresar al inicio del archivo

    // Asignar memoria para los datos
    data = (unsigned char *)malloc(dataSize * sizeof(unsigned char));
    if (data == NULL) {
        printf("Error al asignar memoria.\n");
        fclose(inputFile);
        return 1;
    }

    // Leer el archivo completo
    fread(data, sizeof(unsigned char), dataSize, inputFile);
    fclose(inputFile);

    // Preservar el encabezado BMP sin modificaciones
    // Aplicar cifrado/descifrado solo a los píxeles
    cifradoCesarByte(data, headerSize, dataSize, desplazamiento);

    // Guardar los datos cifrados/descifrados en un archivo nuevo
    if (opcion == 1) {
        outputFile = fopen("goku_cifrada.bmp", "wb");  // Abrir en modo binario para escribir
        if (outputFile == NULL) {
            printf("Error al crear el archivo cifrado.\n");
            free(data);
            return 1;
        }
        fwrite(data, sizeof(unsigned char), dataSize, outputFile);
        printf("Imagen cifrada guardada como 'goku_cifrada.bmp'.\n");
    } else {
        outputFile = fopen("goku_descifrada.bmp", "wb");  // Abrir en modo binario para descifrar
        if (outputFile == NULL) {
            printf("Error al crear el archivo descifrado.\n");
            free(data);
            return 1;
        }
        fwrite(data, sizeof(unsigned char), dataSize, outputFile);
        printf("Imagen descifrada guardada como 'goku_descifrada.bmp'.\n");
    }

    fclose(outputFile);
    free(data);  
    return 0;
}
