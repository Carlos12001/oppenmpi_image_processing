#!/bin/bash

FILE="dato.txt"
SCRIPT="./update_display.sh"

# Crear dato.txt si no existe
if [ ! -f "$FILE" ]; then
    echo "000" > "$FILE"
fi

# Monitorear cambios en dato.txt
while inotifywait -e modify "$FILE"; do
    $SCRIPT
done

