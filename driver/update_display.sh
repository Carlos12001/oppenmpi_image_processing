#!/bin/bash

DEVICE="/dev/my_gpio_driver"
FILE="dato.txt"

if [ ! -e "$DEVICE" ]; then
	echo "Dispositivo $DEVICE no existe. Asegúrate de que el driver esté cargado."
	exit 1
fi

if [ ! -f "$FILE" ]; then
	echo "Archivo $FILE no encontrado."
	exit 1
fi

# Leer el primer caracter del archivo
DIGIT=$(head -c 1 "$FILE")

# Validar que es un dígito entre 0 y 9
if [[ "$DIGIT" =~ ^[0-9]$ ]]; then
	# Escribir el dígito en el dispositivo
	echo -n "$DIGIT" > "$DEVICE"
	echo "Digit $DIGIT escrito en $DEVICE"
else
	echo "El contenido de $FILE no es un dígito válido (0-9)."
	exit 1
fi
