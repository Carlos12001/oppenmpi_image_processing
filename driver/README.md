# Driver
Se utiliza una Raspberry Pi 4 Model B, con las siguientes características:
- OS: Debian GNU/Linux 11 (bullseye) aarch64
- Kernel: 6.1.21-v8+

## Construcción
```Bash
make
```

## Cargar driver
```Bash
sudo insmod gpio_driver.ko
```

## Quitar driver
```Bash
sudo rmmod gpio_driver
```

## Versiones del código:
- v1: Enciende/apaga un led mediante: [Deprecado (Como los talleres de Arqui :v)]
  ```Bash
  echo 1 > /dev/my_gpio_driver # Encender LED 
  echo 0 > /dev/my_gpio_driver # Apagar LED 
  ```

- v2: Enciende/apaga 3 displays de 7 segmentos. Para ello, debe de: [Versión al 03/11/2024]
  - Dar permisos al usuario para ejecutar el script `monitor_dato.sh`:
    ```Bash
    chmod +x monitor_dato.sh
    ```
    este script será el encargado de estar revisando activamente el archivo `dato.txt` para actualizar el valor de `/dev/my_gpio_driver` mediante llamadas continuas al script `update_display.sh`.
    
  - Escribir en el archivo `dato.txt`:
    ```Bash
    echo "123" > dato.txt 
    ```
    Esto deberá de conseguir que se despliegue el valor "123" en el display de 7 segmentos, tal y como se muestra en la figura a continuación:
    
  - Hola
  - Adios

## Idea
El driver se debe de ejecutar en modo kernel, por lo que una manera de poder interactuar con este desde el modo usuario es escribiendo en su archivo `/dev/my_gpio_driver` (creado al cargar el driver mediante ```sudo insmod gpio_driver.ko```). Para esto, se ejecuta, en una terminal, el script de **Bash** llamado `monitor_dato.sh`, que será quien se encargue de estar revisando activamente el estado del archivo `dato.txt` para escribir su valor en `/dev/my_gpio_driver`. 

Mi idea es entonces que utilicen esta base para, desde un programa de C, ejecutar instrucciones tales como ```echo "123" > dato.txt```, donde "123" corresponde al número que reporte la norma de Frobenius. **NOTA**: De momento, el display no es capaz de mostrar el punto decimal. Esto porque no alcanzaban los pines. Por lo que si el número a desplegar es "2.45", este deberá de aparecer en el display como "245". Esto planeo hablarlo con Jason en cuanto regrese (yo lo veo el lunes 11 en persona porque me tiene que aplicar el examen, entonces aprovecho para sacarle el tema). 


## TODOSs:
- Cambiar el segundo y tercer display de 7 segmentos (un segmento no ilumina correctamente)
- Agregar punto decimal

## Referencias
Código escrito por:
- Ignacio Grané Rojas
- Carlos Mata Calderón
- Marcelo Truque Montero
- Felipe Vargas Jiménez

Basado en [el código de este repositorio](https://github.com/Johannes4Linux/Linux_Driver_Tutorial_legacy/tree/main)
