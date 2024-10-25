#include <stdio.h>
#include <MagickWand/MagickWand.h>

void ConvertImage(const char *inputFile, const char *outputFile) {
    MagickWand *magick_wand;

    // Inicializar la biblioteca MagickWand
    MagickWandGenesis();

    // Crear un nuevo MagickWand
    magick_wand = NewMagickWand();

    // Cargar la imagen BMP
    if (MagickReadImage(magick_wand, inputFile) == MagickFalse) {
        fprintf(stderr, "Error: No se pudo leer la imagen BMP.\n");
        return;
    }

    // Guardar la imagen en formato PNG
    if (MagickWriteImage(magick_wand, outputFile) == MagickFalse) {
        fprintf(stderr, "Error: No se pudo guardar la imagen en formato PNG.\n");
        return;
    }

    printf("La imagen se ha convertido correctamente de BMP a PNG.\n");

    // Limpiar y liberar la memoria
    magick_wand = DestroyMagickWand(magick_wand);
    MagickWandTerminus();
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <input.bmp> <output.png>\n", argv[0]);
        return 1;
    }

    const char *inputFile = argv[1];   // Archivo BMP de entrada
    const char *outputFile = argv[2];  // Archivo PNG de salida

    ConvertImage(inputFile, outputFile);
    return 0;
}
