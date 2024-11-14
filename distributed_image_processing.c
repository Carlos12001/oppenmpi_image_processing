// distributed_image_processing.c

#include <arpa/inet.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver/gpio_lib.h"  // Incluir la biblioteca gpio
#define PORT 8080
#define BUF_SIZE 1024

// Estructura para el encabezado de un archivo BMP
#pragma pack(push, 1)
typedef struct {
  unsigned short
      type;  // Tipo de archivo (debe ser 'BM' para un archivo BMP válido)
  unsigned int size;  // Tamaño del archivo en bytes
  unsigned short reserved1;
  unsigned short reserved2;
  unsigned int offset;  // Desplazamiento a los datos de píxeles
} BMPHeader;

// Estructura para el encabezado de información de imagen BMP
typedef struct {
  unsigned int size;             // Tamaño de esta estructura en bytes
  int width;                     // Ancho de la imagen en píxeles
  int height;                    // Altura de la imagen en píxeles
  unsigned short planes;         // Número de planos de color (debe ser 1)
  unsigned short bitCount;       // Número de bits por píxel
  unsigned int compression;      // Método de compresión utilizado
  unsigned int imageSize;        // Tamaño de los datos de imagen en bytes
  int xPixelsPerMeter;           // Resolución horizontal en píxeles por metro
  int yPixelsPerMeter;           // Resolución vertical en píxeles por metro
  unsigned int colorsUsed;       // Número de colores en la paleta
  unsigned int colorsImportant;  // Número de colores importantes
} BMPInfoHeader;
#pragma pack(pop)
void recibir_archivo(int new_socket, const char *filename) {
  FILE *file = fopen(filename, "wb");
  if (file == NULL) {
    perror("No se pudo crear el archivo");
    exit(EXIT_FAILURE);
  }

  char buffer[BUF_SIZE];
  ssize_t bytes_recibidos;

  // Recibir el archivo en bloques
  while ((bytes_recibidos = recv(new_socket, buffer, BUF_SIZE, 0)) > 0) {
    fwrite(buffer, 1, bytes_recibidos, file);
  }

  printf("Archivo recibido correctamente.\n");

  fclose(file);
}

int client_connect() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addr_len = sizeof(address);

  // Crear socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("No se pudo crear el socket");
    exit(EXIT_FAILURE);
  }

  // Configurar la dirección y puerto
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Vincular el socket a la dirección
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("No se pudo vincular el socket");
    exit(EXIT_FAILURE);
  }

  // Escuchar conexiones entrantes
  if (listen(server_fd, 3) < 0) {
    perror("Escucha fallida");
    exit(EXIT_FAILURE);
  }

  printf("Esperando conexiones...\n");

  // Aceptar la conexión del cliente
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t *)&addr_len)) < 0) {
    perror("Falló la aceptación de la conexión");
    exit(EXIT_FAILURE);
  }

  const char *archivo_recibido = "input.bmp";
  recibir_archivo(new_socket, archivo_recibido);

  close(new_socket);
  close(server_fd);
  return 0;
}

void printHeaders(BMPHeader *header, BMPInfoHeader *infoHeader) {
  printf("BMP Header:\n");
  printf("  Type: %u\n", header->type);
  printf("  Size: %u\n", header->size);
  printf("  Reserved1: %u\n", header->reserved1);
  printf("  Reserved2: %u\n", header->reserved2);
  printf("  Offset: %u\n", header->offset);

  printf("BMP Info Header:\n");
  printf("  Size: %u\n", infoHeader->size);
  printf("  Width: %d\n", infoHeader->width);
  printf("  Height: %d\n", infoHeader->height);
  printf("  Planes: %u\n", infoHeader->planes);
  printf("  BitCount: %u\n", infoHeader->bitCount);
  printf("  Compression: %u\n", infoHeader->compression);
  printf("  ImageSize: %u\n", infoHeader->imageSize);
  printf("  XPixelsPerMeter: %d\n", infoHeader->xPixelsPerMeter);
  printf("  YPixelsPerMeter: %d\n", infoHeader->yPixelsPerMeter);
  printf("  ColorsUsed: %u\n", infoHeader->colorsUsed);
  printf("  ColorsImportant: %u\n", infoHeader->colorsImportant);
}

// Función para cifrar datos usando el cifrado César
void caesar_cipher_encrypt(unsigned char *data, int dataSize, int shift) {
  for (int i = 0; i < dataSize; i++) {
    data[i] = (data[i] + shift + (i % 256)) % 256;
    // data[i] = (data[i] + shift) % 256;
  }
}

// Función para descifrar datos usando el cifrado César
void caesar_cipher_decrypt(unsigned char *data, int dataSize, int shift,
                           int rank) {
  for (int i = 0; i < dataSize; i++) {
    data[i] = (data[i] - shift - ((i + rank * dataSize) % 256) + 256) % 256;
    // data[i] = (data[i] - shift + 256) % 256;
  }
}

// Función para conversión a escala de grises
void gray_conversion(unsigned char *data, BMPInfoHeader infoHeader,
                     unsigned char *newdata, int start, int end) {
  int width = infoHeader.width;
  int rowSize = width * 3;

  for (int y = start; y < end; y++) {
    for (int x = 0; x < width; x++) {
      int pos = y * rowSize + x * 3;
      unsigned char newcolor =
          (unsigned char)(0.3 * data[pos + 2] + 0.59 * data[pos + 1] +
                          0.11 * data[pos]);
      newdata[pos] = newcolor;
      newdata[pos + 1] = newcolor;
      newdata[pos + 2] = newcolor;
    }
  }
}

// Función para desenfoque (blur)
void blur_conversion(unsigned char *data, BMPInfoHeader infoHeader,
                     unsigned char *newdata, int start, int end) {
  float kernel[3][3] = {{1.0 / 16, 2.0 / 16, 1.0 / 16},
                        {2.0 / 16, 4.0 / 16, 2.0 / 16},
                        {1.0 / 16, 2.0 / 16, 1.0 / 16}};

  int width = infoHeader.width;
  int rowSize = width * 3;
  int height = infoHeader.height;

  for (int y = start; y < end; y++) {
    if (y <= 0 || y >= height - 1) continue;  // Saltar bordes
    for (int x = 1; x < width - 1; x++) {
      for (int c = 0; c < 3; c++) {
        float sum = 0.0;
        for (int i = -1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
            int pos = ((y + i) * rowSize) + ((x + j) * 3) + c;
            sum += kernel[i + 1][j + 1] * data[pos];
          }
        }
        int pos = y * rowSize + x * 3 + c;
        newdata[pos] = (unsigned char)sum;
      }
    }
  }
}

// Función para filtro Sobel
void sobel_filter(unsigned char *data, int width, int height,
                  unsigned char *newdata, int start, int end) {
  int rowSize = width * 3;

  // Kernels de Sobel
  int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
  int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

  // Convertir a escala de grises primero
  unsigned char *grayData = (unsigned char *)malloc(width * height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int pos_rgb = y * rowSize + x * 3;
      int pos_gray = y * width + x;
      unsigned char gray =
          (unsigned char)(0.3 * data[pos_rgb + 2] + 0.59 * data[pos_rgb + 1] +
                          0.11 * data[pos_rgb]);
      grayData[pos_gray] = gray;
    }
  }

  for (int y = start; y < end; y++) {
    if (y <= 0 || y >= height - 1) continue;  // Saltar bordes
    for (int x = 1; x < width - 1; x++) {
      double gx = 0.0;
      double gy = 0.0;

      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          int pixel = grayData[(y + i) * width + (x + j)];
          gx += Gx[i + 1][j + 1] * pixel;
          gy += Gy[i + 1][j + 1] * pixel;
        }
      }

      double magnitude = sqrt(gx * gx + gy * gy);
      if (magnitude > 255.0) magnitude = 255.0;
      unsigned char edgeVal = (unsigned char)magnitude;

      int pos_rgb = (y - 1) * rowSize + x * 3;  // Ajuste de índice en newdata
      newdata[pos_rgb] = edgeVal;
      newdata[pos_rgb + 1] = edgeVal;
      newdata[pos_rgb + 2] = edgeVal;
    }
  }
  free(grayData);
}

// Función para filtro rojo
void red_filter(unsigned char *data, BMPInfoHeader infoHeader,
                unsigned char *newdata, int start, int end) {
  int width = infoHeader.width;
  int rowSize = width * 3;

  for (int y = start; y < end; y++) {
    for (int x = 0; x < width; x++) {
      int pos = y * rowSize + x * 3;
      newdata[pos] = 0;                  // Componente azul
      newdata[pos + 1] = 0;              // Componente verde
      newdata[pos + 2] = data[pos + 2];  // Componente rojo
    }
  }
}

// Función para filtro verde
void green_filter(unsigned char *data, BMPInfoHeader infoHeader,
                  unsigned char *newdata, int start, int end) {
  int width = infoHeader.width;
  int rowSize = width * 3;

  for (int y = start; y < end; y++) {
    for (int x = 0; x < width; x++) {
      int pos = y * rowSize + x * 3;
      newdata[pos] = 0;                  // Componente azul
      newdata[pos + 1] = data[pos + 1];  // Componente verde
      newdata[pos + 2] = 0;              // Componente rojo
    }
  }
}

// Función para filtro azul
void blue_filter(unsigned char *data, BMPInfoHeader infoHeader,
                 unsigned char *newdata, int start, int end) {
  int width = infoHeader.width;
  int rowSize = width * 3;

  for (int y = start; y < end; y++) {
    for (int x = 0; x < width; x++) {
      int pos = y * rowSize + x * 3;
      newdata[pos] = data[pos];  // Componente azul
      newdata[pos + 1] = 0;      // Componente verde
      newdata[pos + 2] = 0;      // Componente rojo
    }
  }
}

// Función para calcular la norma de Frobenius local
double compute_local_frobenius_norm(unsigned char *data, int dataSize) {
  double local_sum = 0.0;
  for (int i = 0; i < dataSize; i++) {
    double val = (double)data[i];
    local_sum += val * val;
  }
  return local_sum;
}

int main() {
  MPI_Init(NULL, NULL);  // Se eliminan argc y argv

  int rank, size, shift;  // 'shift' es el desplazamiento para el cifrado César
  shift = 300;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  BMPHeader bmpHeader;
  BMPInfoHeader bmpInfoHeader;
  unsigned char *data = NULL;

  int height, width, rowSize, totalSize;
  int rows_per_process, remaining_rows;
  int localHeight, localSize;
  int *sendcounts = NULL;
  int *displs = NULL;
  double frobenius_norm_processed =
      0.0;  // Variable para almacenar la norma de Frobenius

  // Variables para el driver y el código de filtro
  char input_values[4];  // 3 dígitos + terminador nulo
  int filter_code = 0;   // Código de filtro en entero

  if (rank == 0) {
    // Abrir el driver
    int fd = gpio_open();
    if (fd < 0) {
      printf("Error al abrir el driver GPIO\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Leer el código de filtro desde el driver
    ssize_t ret = gpio_read(fd, input_values, 3);
    if (ret < 0) {
      printf("Error al leer desde el driver GPIO\n");
      gpio_close(fd);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    input_values[3] = '\0';  // Asegurar terminador nulo
    printf("Código de filtro recibido: %s\n", input_values);

    // Validar que los dígitos sean '0' o '1'
    for (int i = 0; i < 3; i++) {
      if (input_values[i] != '0' && input_values[i] != '1') {
        printf(
            "Entrada inválida en el driver GPIO. Debe ser una cadena de 3 "
            "dígitos binarios.\n");
        gpio_close(fd);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }

    // Convertir el código binario a entero
    filter_code = (input_values[0] - '0') * 4 + (input_values[1] - '0') * 2 +
                  (input_values[2] - '0') * 1;

    gpio_close(fd);

    // Verificar si no se ha seleccionado ningún filtro
    if (filter_code == 0) {
      printf("No se ha seleccionado ningún filtro. Saliendo.\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Recibir el archivo de imagen
    client_connect();
    FILE *inputFile = fopen("input.bmp", "rb");
    if (inputFile == NULL) {
      perror("Error abriendo el archivo de entrada");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Leer el encabezado y los datos de la imagen
    fread(&bmpHeader, sizeof(BMPHeader), 1, inputFile);
    if (bmpHeader.type !=
        0x4D42) {  // Verificar que el archivo es un BMP ('BM')
      printf("El archivo no es un BMP válido\n");
      fclose(inputFile);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fread(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, inputFile);
    printHeaders(&bmpHeader, &bmpInfoHeader);
    data = (unsigned char *)malloc(bmpInfoHeader.imageSize);
    if (data == NULL) {
      fprintf(stderr,
              "No se pudo asignar memoria para los datos de la imagen.\n");
      fclose(inputFile);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fread(data, 1, bmpInfoHeader.imageSize, inputFile);
    fclose(inputFile);

    // Preparar los parámetros para la distribución de datos
    height = bmpInfoHeader.height;
    width = bmpInfoHeader.width;
    rowSize = width * 3;
    totalSize = height * rowSize;

    // Calcular el número de filas que cada proceso debe procesar
    rows_per_process = height / size;
    remaining_rows = height % size;

    sendcounts = (int *)malloc(size * sizeof(int));
    displs = (int *)malloc(size * sizeof(int));

    int offset = 0;
    for (int i = 0; i < size; i++) {
      int lh = rows_per_process;
      if (i < remaining_rows) {
        lh += 1;
      }
      sendcounts[i] = lh * rowSize;
      displs[i] = offset;
      offset += sendcounts[i];
    }

    caesar_cipher_encrypt(data, bmpInfoHeader.imageSize, shift);

    FILE *encryptedFile = fopen("Encrypted_Image.bmp", "wb");
    if (!encryptedFile) {
      printf("No se pudo crear el archivo de salida\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fwrite(&bmpHeader, sizeof(BMPHeader), 1, encryptedFile);
    fwrite(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, encryptedFile);
    fwrite(data, 1, bmpInfoHeader.imageSize, encryptedFile);

    fclose(encryptedFile);
  }

  // Difundir el código de filtro a todos los procesos
  MPI_Bcast(&filter_code, 1, MPI_INT, 0, MPI_COMM_WORLD);

  MPI_Bcast(&bmpHeader, sizeof(BMPHeader), MPI_BYTE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&bmpInfoHeader, sizeof(BMPInfoHeader), MPI_BYTE, 0, MPI_COMM_WORLD);

  // Determinar el filtro basado en filter_code
  char filter_name[10];    // Nombre del filtro
  if (filter_code == 1) {  // 001
    strcpy(filter_name, "blur");
  } else if (filter_code == 2) {  // 010
    strcpy(filter_name, "grey");
  } else if (filter_code == 3) {  // 011
    strcpy(filter_name, "sobel");
  } else if (filter_code == 4) {  // 100
    strcpy(filter_name, "red");
  } else if (filter_code == 5) {  // 101
    strcpy(filter_name, "green");
  } else if (filter_code == 7) {  // 111
    strcpy(filter_name, "blue");
  } else {
    if (rank == 0) {
      printf("Código de filtro no reconocido: %s\n", input_values);
    }
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // Si no es el proceso 0, inicializar las variables necesarias
  if (rank != 0) {
    height = bmpInfoHeader.height;
    width = bmpInfoHeader.width;
    rowSize = width * 3;
    totalSize = height * rowSize;

    // Calcular el número de filas que cada proceso debe procesar
    rows_per_process = height / size;
    remaining_rows = height % size;
  }

  sendcounts = (int *)malloc(size * sizeof(int));
  displs = (int *)malloc(size * sizeof(int));

  // Preparar sendcounts y displs para MPI_Scatterv
  int offset = 0;
  for (int i = 0; i < size; i++) {
    int lh = rows_per_process;
    if (i < remaining_rows) {
      lh += 1;
    }
    sendcounts[i] = lh * rowSize;
    displs[i] = offset;
    offset += sendcounts[i];
  }

  localHeight = sendcounts[rank] / rowSize;
  localSize = localHeight * rowSize;

  unsigned char *subData = (unsigned char *)malloc(localSize);
  unsigned char *subDataProcessed = (unsigned char *)malloc(localSize);

  // Difundir 'sendcounts' y 'displs' a todos los procesos
  MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

  // Distribuir los datos cifrados a los procesos
  MPI_Scatterv(data, sendcounts, displs, MPI_UNSIGNED_CHAR, subData, localSize,
               MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

  // Cada proceso descifra sus datos locales
  caesar_cipher_decrypt(subData, localSize, shift, rank);

  // Calcular la norma de Frobenius antes del procesamiento
  double local_sum = compute_local_frobenius_norm(subData, localSize);
  double total_sum = 0.0;
  MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    double frobenius_norm = sqrt(total_sum);
    printf("Norma de Frobenius de la imagen original: %lf\n", frobenius_norm);
  }

  printf("Proceso: %d de %d\n", rank, size);

  int start = 0;
  int end = localHeight;

  // Procesamiento según el filtro seleccionado
  if (!strcmp(filter_name, "grey")) {
    printf("Proceso %d realizando conversión a escala de grises\n", rank);
    gray_conversion(subData, bmpInfoHeader, subDataProcessed, start, end);
  } else if (!strcmp(filter_name, "blur")) {
    printf("Proceso %d realizando desenfoque\n", rank);
    blur_conversion(subData, bmpInfoHeader, subDataProcessed, start, end);
  } else if (!strcmp(filter_name, "sobel")) {
    printf("Proceso %d realizando filtro Sobel\n", rank);

    // Preparar para el intercambio de halos
    int extendedHeight = localHeight + 2;
    unsigned char *subDataWithBorders =
        (unsigned char *)malloc(extendedHeight * rowSize);

    // Inicializar subDataWithBorders
    memset(subDataWithBorders, 0, extendedHeight * rowSize);

    // Copiar subData al centro de subDataWithBorders
    memcpy(subDataWithBorders + rowSize, subData, localHeight * rowSize);

    MPI_Status status;

    // Intercambio de filas superiores
    if (rank != 0) {
      // Enviar primera fila a proceso anterior
      MPI_Send(subData, rowSize, MPI_UNSIGNED_CHAR, rank - 1, 0,
               MPI_COMM_WORLD);
      // Recibir última fila del proceso anterior
      MPI_Recv(subDataWithBorders, rowSize, MPI_UNSIGNED_CHAR, rank - 1, 1,
               MPI_COMM_WORLD, &status);
    } else {
      // Replicar primera fila
      memcpy(subDataWithBorders, subData, rowSize);
    }

    // Intercambio de filas inferiores
    if (rank != size - 1) {
      // Enviar última fila al siguiente proceso
      MPI_Send(subData + (localHeight - 1) * rowSize, rowSize,
               MPI_UNSIGNED_CHAR, rank + 1, 1, MPI_COMM_WORLD);
      // Recibir primera fila del siguiente proceso
      MPI_Recv(subDataWithBorders + (localHeight + 1) * rowSize, rowSize,
               MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD, &status);
    } else {
      // Replicar última fila
      memcpy(subDataWithBorders + (localHeight + 1) * rowSize,
             subData + (localHeight - 1) * rowSize, rowSize);
    }

    // Ahora aplicar el filtro Sobel localmente
    start = 1;
    end = localHeight + 1;  // Procesar hasta localHeight + 1

    sobel_filter(subDataWithBorders, width, extendedHeight, subDataProcessed,
                 start, end);

    free(subDataWithBorders);
  } else if (!strcmp(filter_name, "red")) {
    printf("Proceso %d aplicando filtro rojo\n", rank);
    red_filter(subData, bmpInfoHeader, subDataProcessed, start, end);
  } else if (!strcmp(filter_name, "green")) {
    printf("Proceso %d aplicando filtro verde\n", rank);
    green_filter(subData, bmpInfoHeader, subDataProcessed, start, end);
  } else if (!strcmp(filter_name, "blue")) {
    printf("Proceso %d aplicando filtro azul\n", rank);
    blue_filter(subData, bmpInfoHeader, subDataProcessed, start, end);
  } else {
    printf("Filtro no reconocido. No se aplicará ningún filtro.\n");
    memcpy(subDataProcessed, subData, localSize);
  }

  // Calcular la norma de Frobenius después del procesamiento
  double local_sum_processed =
      compute_local_frobenius_norm(subDataProcessed, localSize);
  double total_sum_processed = 0.0;
  MPI_Reduce(&local_sum_processed, &total_sum_processed, 1, MPI_DOUBLE, MPI_SUM,
             0, MPI_COMM_WORLD);

  if (rank == 0) {
    frobenius_norm_processed = sqrt(total_sum_processed);
    printf("Norma de Frobenius de la imagen procesada: %lf\n",
           frobenius_norm_processed);
  }

  // Recopilar los datos procesados en el proceso 0
  unsigned char *newData = NULL;
  if (rank == 0) {
    newData = (unsigned char *)malloc(bmpInfoHeader.imageSize);
  }

  MPI_Gatherv(subDataProcessed, localSize, MPI_UNSIGNED_CHAR, newData,
              sendcounts, displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    FILE *outputFile = fopen("Processed_Image.bmp", "wb");
    if (!outputFile) {
      printf("No se pudo crear el archivo de salida\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fwrite(&bmpHeader, sizeof(BMPHeader), 1, outputFile);
    fwrite(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, outputFile);
    fwrite(newData, 1, bmpInfoHeader.imageSize, outputFile);

    fclose(outputFile);
    free(newData);
    free(data);
  }

  free(sendcounts);
  free(displs);
  free(subData);
  free(subDataProcessed);

  MPI_Finalize();

  // Después de MPI_Finalize(), escribir los primeros tres dígitos de la norma
  // de Frobenius en el driver
  if (rank == 0) {
    // Abrir el driver
    int fd = gpio_open();
    if (fd < 0) {
      printf("Error al abrir el driver GPIO\n");
      return -1;
    }

    // Convertir la norma de Frobenius a cadena
    char frobenius_str[20];
    snprintf(frobenius_str, sizeof(frobenius_str), "%.0lf",
             frobenius_norm_processed);

    // Extraer los primeros tres dígitos
    char output_values[4];  // 3 dígitos + terminador nulo
    strncpy(output_values, frobenius_str, 3);
    output_values[3] = '\0';

    // Si la norma tiene menos de 3 dígitos, rellenar con ceros
    int len = strlen(frobenius_str);
    if (len < 3) {
      char padded_output[4] = {'0', '0', '0', '\0'};
      strncpy(padded_output + (3 - len), frobenius_str, len);
      strcpy(output_values, padded_output);
    }

    // Asegurarse de que los caracteres son dígitos
    for (int i = 0; i < 3; i++) {
      if (output_values[i] < '0' || output_values[i] > '9') {
        output_values[i] = '0';
      }
    }

    // Escribir en el driver
    ssize_t ret = gpio_write(fd, output_values, 3);
    if (ret < 0) {
      printf("Error al escribir en el driver GPIO\n");
      gpio_close(fd);
      return -1;
    }

    printf("Se han enviado los dígitos '%s' al driver GPIO\n", output_values);

    gpio_close(fd);
  }

  return 0;
}
