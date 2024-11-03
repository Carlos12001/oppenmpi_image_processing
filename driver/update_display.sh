#!/bin/bash

DEVICE="/dev/my_gpio_driver"
FILE="dato.txt"

# Verificar que el dispositivo existe
if [ ! -e "$DEVICE" ]; then
    echo "Dispositivo $DEVICE no existe. Asegúrate de que el driver esté cargado."
    exit 1
fi

# Verificar que el archivo existe
if [ ! -f "$FILE" ]; then
    echo "Archivo $FILE no encontrado."
    exit 1
fi

# Leer los primeros 3 caracteres del archivo
DIGITS=$(head -c 3 "$FILE")

# Validar que son 3 dígitos entre 0 y 9
if [[ "$DIGITS" =~ ^[0-9]{3}$ ]]; then
    # Escribir los 3 dígitos en el dispositivo
    echo -n "$DIGITS" > "$DEVICE"
    echo "Digits '$DIGITS' escritos en $DEVICE"
else
    echo "El contenido de $FILE no es un conjunto válido de 3 dígitos (000-999)."
    exit 1
fi

