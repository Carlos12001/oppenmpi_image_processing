#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/signal.h>

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ICMF");
MODULE_DESCRIPTION("GPIO Driver for 3 7-Segment Displays");

/* Variables for device and device class */
static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

/* Definición de pines GPIO para 3 displays */
const int display_pins[3][7] = {
    {2,  3,  4,  5,  6,  7,  8},        // Display 1
    {10, 11, 12, 13, 14, 15, 16},  // Display 2
    {17, 18, 19, 20, 21, 22, 23}  // Display 3
};
const int num_displays = 3;
const int num_pins_per_display = 7;

#define BUTTON_GPIO 24

#define DRIVER_NAME "my_gpio_driver"
#define DRIVER_CLASS "MyModuleClass"

/* Variables para manejo de IRQ y fasync */
static int button_irq = -1;
static struct fasync_struct *async_queue;

/* Tabla de valores para cada dígito '0' a '9' */
/* 1 = segmento encendido, 0 = segmento apagado */
const char gpio_values[10][7] = {
    // a, b, c, d, e, f, g
      {0, 1, 1, 1, 1, 1, 1}, // '0'
      {0, 0, 0, 1, 0, 0, 1}, // '1'
      {1, 0, 1, 1, 1, 1, 0}, // '2'
      {1, 0, 1, 1, 0, 1, 1}, // '3'
      {1, 1, 0, 1, 0, 0, 1}, // '4'
      {1, 1, 1, 0, 0, 1, 1}, // '5'
      {1, 1, 1, 0, 1, 1, 1}, // '6'
      {0, 0, 1, 1, 0, 0, 1}, // '7'
      {1, 1, 1, 1, 1, 1, 1}, // '8'
      {1, 1, 1, 1, 0, 1, 1}  // '9'
};

/**
 * @brief Read data out of the buffer
 */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
    // Por simplicidad, devolveremos una cadena fija. Puedes implementar el almacenamiento del último valor si lo deseas.
    char tmp[4] = "000"; // Inicializar con '000'

    /* Determinar la cantidad a copiar */
    int to_copy = min(count, sizeof(tmp) - 1); // Reservar espacio para el terminador nulo

    /* Copiar datos al espacio de usuario */
    if (copy_to_user(user_buffer, tmp, to_copy)) {
        return -EFAULT;
    }

    return to_copy;
}

/**
 * @brief Write data to buffer
 */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
    char digits[3];

    /* Verificar que se han enviado al menos 3 caracteres */
    if (count < 3) {
        printk(KERN_WARNING "my_gpio_driver: Not enough data. Expected 3 digits.\n");
        return -EINVAL;
    }

    /* Copiar 3 caracteres desde el espacio de usuario */
    if (copy_from_user(digits, user_buffer, 3)) {
        printk(KERN_WARNING "my_gpio_driver: Failed to copy data from user space\n");
        return -EFAULT;
    }

    /* Actualizar cada display */
    for (int i = 0; i < num_displays; i++) {
        if (digits[i] >= '0' && digits[i] <= '9') {
            int digit = digits[i] - '0'; // Convertir carácter a índice (0-9)

            /* Establecer los valores de GPIO para cada segmento */
            for (int j = 0; j < num_pins_per_display; j++) {
                gpio_set_value(display_pins[i][j], gpio_values[digit][j]);
            }
            printk(KERN_INFO "my_gpio_driver: Display %d set to '%c'\n", i+1, digits[i]);
        } else {
            printk(KERN_WARNING "my_gpio_driver: Invalid input for Display %d: '%c'\n", i+1, digits[i]);
        }
    }

    return 3;
}

/**
 * @brief This function handles asynchronous notifications
 */
static int driver_fasync(int fd, struct file *filp, int on) {
    return fasync_helper(fd, filp, on, &async_queue);
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_open(struct inode *device_file, struct file *instance) {
    printk(KERN_INFO "my_gpio_driver: Open was called!\n");
    return 0;
}

/**
 * @brief This function is called, when the device file is closed
 */
static int driver_close(struct inode *device_file, struct file *instance) {
    printk(KERN_INFO "my_gpio_driver: Close was called!\n");
    driver_fasync(-1, instance, 0); // Remover suscripción a fasync
    return 0;
}

/**
 * @brief Interrupt handler for the button
 */
static irqreturn_t button_irq_handler(int irq, void *dev_id) {
    // Notificar al espacio de usuario sobre la presión del botón
    if (async_queue) {
        kill_fasync(&async_queue, SIGIO, POLL_IN);
    }
    return IRQ_HANDLED;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write,
    .fasync = driver_fasync
};

/**
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init ModuleInit(void) {
    int ret;
    printk(KERN_INFO "my_gpio_driver: Initializing the GPIO driver for 3 displays\n");

    /* Asignar un número de dispositivo */
    ret = alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        printk(KERN_ERR "my_gpio_driver: Device Nr. could not be allocated!\n");
        return ret;
    }
    printk(KERN_INFO "my_gpio_driver: Device Nr. Major: %d, Minor: %d was registered!\n", MAJOR(my_device_nr), MINOR(my_device_nr));

    /* Crear clase de dispositivo */
    my_class = class_create(THIS_MODULE, DRIVER_CLASS);
    if (IS_ERR(my_class)) {
        printk(KERN_ERR "my_gpio_driver: Device class cannot be created!\n");
        unregister_chrdev_region(my_device_nr, 1);
        return PTR_ERR(my_class);
    }

    /* Crear archivo de dispositivo */
    if (IS_ERR(device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME))) {
        printk(KERN_ERR "my_gpio_driver: Cannot create device file!\n");
        class_destroy(my_class);
        unregister_chrdev_region(my_device_nr, 1);
        return PTR_ERR(my_class);
    }

    /* Inicializar y registrar cdev */
    cdev_init(&my_device, &fops);
    ret = cdev_add(&my_device, my_device_nr, 1);
    if (ret < 0) {
        printk(KERN_ERR "my_gpio_driver: Registering of device to kernel failed!\n");
        device_destroy(my_class, my_device_nr);
        class_destroy(my_class);
        unregister_chrdev_region(my_device_nr, 1);
        return ret;
    }

    /* Inicializar todos los GPIOs para los 3 displays */
    for (int i = 0; i < num_displays; i++) {
        for (int j = 0; j < num_pins_per_display; j++) {
            if (gpio_request(display_pins[i][j], "rpi-gpio")) {
                printk(KERN_ERR "my_gpio_driver: Cannot allocate GPIO %d for Display %d\n", display_pins[i][j], i+1);
                goto GpioError;
            }

            if (gpio_direction_output(display_pins[i][j], 0)) {
                printk(KERN_ERR "my_gpio_driver: Cannot set GPIO %d to output for Display %d!\n", display_pins[i][j], i+1);
                goto GpioError;
            }
        }
    }
    
    /* Configurar GPIO del botón */
    if (gpio_request(BUTTON_GPIO, "rpi-button")) {
        printk(KERN_ERR "my_gpio_driver: Cannot allocate GPIO %d for button\n", BUTTON_GPIO);
        goto GpioError;
    }

    if (gpio_direction_input(BUTTON_GPIO)) {
        printk(KERN_ERR "my_gpio_driver: Cannot set GPIO %d as input for button\n", BUTTON_GPIO);
        goto ButtonError;
    }

    /* Obtener el número de IRQ para el GPIO del botón */
    button_irq = gpio_to_irq(BUTTON_GPIO);
    if (button_irq < 0) {
        printk(KERN_ERR "my_gpio_driver: Failed to get IRQ for GPIO %d\n", BUTTON_GPIO);
        goto ButtonError;
    }

    /* Solicitar la IRQ */
    ret = request_irq(button_irq, button_irq_handler, IRQF_TRIGGER_RISING | IRQF_SHARED, "my_gpio_driver_button", (void *)(button_irq_handler));
    if (ret) {
        printk(KERN_ERR "my_gpio_driver: Failed to request IRQ for GPIO %d\n", BUTTON_GPIO);
        goto ButtonError;
    }

    printk(KERN_INFO "my_gpio_driver: All GPIOs initialized successfully for 3 displays\n");
    return 0;

GpioError:
    // Liberar GPIOs previamente asignados en caso de error
    for (int j = 0; j < num_displays; j++) {
        for (int k = 0; k < num_pins_per_display; k++) {
            gpio_free(display_pins[j][k]);
        }
    }
ButtonError:
    gpio_free(BUTTON_GPIO);
    for (int j = 0; j < num_displays; j++) {
        for (int k = 0; k < num_pins_per_display; k++) {
            gpio_free(display_pins[j][k]);
        }
    }
    
    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);
    return -1;
}

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit ModuleExit(void) {
    /* Liberar GPIOs */
    for (int i = 0; i < num_displays; i++) {
        for (int j = 0; j < num_pins_per_display; j++) {
            gpio_set_value(display_pins[i][j], 0); // Apagar segmentos
            gpio_free(display_pins[i][j]);
        }
    }
    
	/* Liberar IRQ y GPIO del botón */
    if (button_irq >= 0) {
        free_irq(button_irq, (void *)(button_irq_handler));
        gpio_free(BUTTON_GPIO);
    }

    /* Eliminar dispositivo y clase */
    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);
    printk(KERN_INFO "my_gpio_driver: Goodbye, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

