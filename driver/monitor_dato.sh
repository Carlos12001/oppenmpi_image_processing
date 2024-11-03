#!/bin/bash

FILE="dato.txt"
SCRIPT="./update_display.sh"

if [ ! -f "$FILE" ]; then
	echo "Archivo $FILE no encontrado. Creándolo"
	echo "0" > "$FILE"
fi

while inotifywait -e modify "$FILE"; do
	$SCRIPT
done
