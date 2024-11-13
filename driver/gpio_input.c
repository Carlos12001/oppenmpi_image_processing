#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

/* Información Meta */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Johannes 4 GNU/Linux");
MODULE_DESCRIPTION("Un driver simple GPIO para configurar un LED y leer un botón");

/* Variables para el dispositivo y clase de dispositivo */
static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "my_gpio_driver"
#define DRIVER_CLASS "MyModuleClass"
#define GPIO_OUTPUT_PIN 4
#define GPIO_INPUT_PIN1 9
#define GPIO_INPUT_PIN2 24
#define GPIO_INPUT_PIN3 25

/**
 * @brief Leer datos del buffer
 */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;
    char tmp[4]; // Espacio para 3 caracteres y el terminador nulo

    /* Verificar si ya se han leído todos los datos */
    if (*offs > 0) {
        return 0; // Indicar EOF
    }

    /* Obtener la cantidad de datos a copiar */
    to_copy = min(count, sizeof(tmp));

    /* Leer valores de los botones */
    printk("Valor del botón GPIO %d: %d\n", GPIO_INPUT_PIN1, gpio_get_value(GPIO_INPUT_PIN1));
    printk("Valor del botón GPIO %d: %d\n", GPIO_INPUT_PIN2, gpio_get_value(GPIO_INPUT_PIN2));
    printk("Valor del botón GPIO %d: %d\n", GPIO_INPUT_PIN3, gpio_get_value(GPIO_INPUT_PIN3));
    tmp[0] = gpio_get_value(GPIO_INPUT_PIN1) + '0';
    tmp[1] = gpio_get_value(GPIO_INPUT_PIN2) + '0';
    tmp[2] = gpio_get_value(GPIO_INPUT_PIN3) + '0';
    tmp[3] = '\0'; // Terminador nulo

    /* Copiar datos al espacio de usuario */
    not_copied = copy_to_user(user_buffer, tmp, to_copy);

    /* Calcular datos */
    delta = to_copy - not_copied;

    /* Actualizar el desplazamiento */
    *offs += delta;

    return delta;
}

/**
 * @brief Escribir datos en el buffer
 */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;
    char value;

    /* Obtener la cantidad de datos a copiar */
    to_copy = min(count, sizeof(value));

    /* Copiar datos del espacio de usuario */
    not_copied = copy_from_user(&value, user_buffer, to_copy);

    /* Configurar el LED */
    switch(value) {
        case '0':
            gpio_set_value(GPIO_OUTPUT_PIN, 0);
            break;
        case '1':
            gpio_set_value(GPIO_OUTPUT_PIN, 1);
            break;
        default:
            printk("Entrada inválida!\n");
            break;
    }

    /* Calcular datos */
    delta = to_copy - not_copied;

    return delta;
}

/**
 * @brief Esta función se llama cuando el archivo de dispositivo se abre
 */
static int driver_open(struct inode *device_file, struct file *instance) {
    printk("dev_nr - open fue llamado!\n");
    return 0;
}

/**
 * @brief Esta función se llama cuando el archivo de dispositivo se cierra
 */
static int driver_close(struct inode *device_file, struct file *instance) {
    printk("dev_nr - close fue llamado!\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write
};

/**
 * @brief Esta función se llama cuando el módulo se carga en el kernel
 */
static int __init ModuleInit(void) {
    printk("Hola, Kernel!\n");

    /* Asignar un número de dispositivo */
    if( alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME) < 0) {
        printk("No se pudo asignar el número de dispositivo!\n");
        return -1;
    }
    printk("read_write - Dispositivo Nr. Major: %d, Minor: %d fue registrado!\n", my_device_nr >> 20, my_device_nr && 0xfffff);

    /* Crear clase de dispositivo */
    if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
        printk("No se puede crear la clase de dispositivo!\n");
        goto ClassError;
    }

    /* Crear archivo de dispositivo */
    if(device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME) == NULL) {
        printk("No se puede crear el archivo de dispositivo!\n");
        goto FileError;
    }

    /* Inicializar archivo de dispositivo */
    cdev_init(&my_device, &fops);

    /* Registrar dispositivo en el kernel */
    if(cdev_add(&my_device, my_device_nr, 1) == -1) {
        printk("El registro del dispositivo en el kernel falló!\n");
        goto AddError;
    }

    /* Inicializar GPIO_OUTPUT_PIN */
    if(gpio_request(GPIO_OUTPUT_PIN, "rpi-gpio-output")) {
        printk("No se puede asignar GPIO %d\n", GPIO_OUTPUT_PIN);
        goto AddError;
    }

    /* Establecer dirección de GPIO_OUTPUT_PIN */
    if(gpio_direction_output(GPIO_OUTPUT_PIN, 0)) {
        printk("No se puede configurar GPIO %d como salida!\n", GPIO_OUTPUT_PIN);
        goto GpioOutputError;
    }

    /* Inicializar GPIO_INPUT_PIN1 */
    if(gpio_request(GPIO_INPUT_PIN1, "rpi-gpio-input1")) {
        printk("No se puede asignar GPIO %d\n", GPIO_INPUT_PIN1);
        goto GpioInput1Error;
    }

    /* Establecer dirección de GPIO_INPUT_PIN1 */
    if(gpio_direction_input(GPIO_INPUT_PIN1)) {
        printk("No se puede configurar GPIO %d como entrada!\n", GPIO_INPUT_PIN1);
        goto GpioInput1Error;
    }

    /* Inicializar GPIO_INPUT_PIN2 */
    if(gpio_request(GPIO_INPUT_PIN2, "rpi-gpio-input2")) {
        printk("No se puede asignar GPIO %d\n", GPIO_INPUT_PIN2);
        goto GpioInput2Error;
    }

    /* Establecer dirección de GPIO_INPUT_PIN2 */
    if(gpio_direction_input(GPIO_INPUT_PIN2)) {
        printk("No se puede configurar GPIO %d como entrada!\n", GPIO_INPUT_PIN2);
        goto GpioInput2Error;
    }

    /* Inicializar GPIO_INPUT_PIN3 */
    if(gpio_request(GPIO_INPUT_PIN3, "rpi-gpio-input3")) {
        printk("No se puede asignar GPIO %d\n", GPIO_INPUT_PIN3);
        goto GpioInput3Error;
    }

    /* Establecer dirección de GPIO_INPUT_PIN3 */
    if(gpio_direction_input(GPIO_INPUT_PIN3)) {
        printk("No se puede configurar GPIO %d como entrada!\n", GPIO_INPUT_PIN3);
        goto GpioInput3Error;
    }

    return 0;

GpioInput3Error:
    gpio_free(GPIO_INPUT_PIN3);
GpioInput2Error:
    gpio_free(GPIO_INPUT_PIN2);
GpioInput1Error:
    gpio_free(GPIO_INPUT_PIN1);
GpioOutputError:
    gpio_free(GPIO_OUTPUT_PIN);
AddError:
    device_destroy(my_class, my_device_nr);
FileError:
    class_destroy(my_class);
ClassError:
    unregister_chrdev_region(my_device_nr, 1);
    return -1;
}

/**
 * @brief Esta función se llama cuando el módulo se elimina del kernel
 */
static void __exit ModuleExit(void) {
    gpio_set_value(GPIO_OUTPUT_PIN, 0);
    gpio_free(GPIO_INPUT_PIN1);
    gpio_free(GPIO_INPUT_PIN2);
    gpio_free(GPIO_INPUT_PIN3);
    gpio_free(GPIO_OUTPUT_PIN);
    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);
    printk("Adiós, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);
