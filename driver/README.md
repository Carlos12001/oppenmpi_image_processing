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
- v1: Enciende/apaga un led mediante:
```Bash
echo 1 > /dev/my_gpio_driver # Encender LED 
echo 0 > /dev/my_gpio_driver # Apagar LED 
```

## Referencias
Código escrito por:
- Ignacio Grané Rojas
- Carlos Mata Calderón
- Felipe Vargas Jiménez
- Marcelo Truque Montero

Basado en [el código de este repositorio](https://github.com/Johannes4Linux/Linux_Driver_Tutorial_legacy/tree/main)
