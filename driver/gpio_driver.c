#include <linux/module.h>  // Necesario para cualquier módulo de kernel
#include <linux/init.h>    // Define macros para funciones de inicialización
#include <linux/fs.h>	   // Proporciona estructuras y funciones para manejar el sistema de archivos
#include <linux/cdev.h>    // Soporte para dispositivos de caracteres
#include <linux/uaccess.h> // Funciones para copiar datos entre espacio de usuario y kernel
#include <linux/gpio.h>	   // Funciones para manejar GPIO en Linux

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ICMF"); // Ignacio Grane - Carlos Mata - Marcelo Truque - Felipe Vargas
MODULE_DESCRIPTION("GPIO Driver");

/* Variables for device and device class */
static dev_t my_device_nr; 		// Numero de dispositivo asignado dinamicamente
static struct class *my_class;  // Clase del dispositivo para crear nodos en /dev
static struct cdev my_device;   // Estructura cdev que representa el dispositivo

const int gpio_pins[] = {2, 3, 4, 5, 6, 7, 8};
const int num_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);

#define DRIVER_NAME "my_gpio_driver"
#define DRIVER_CLASS "MyModuleClass"

/**
 * @brief Read data out of the buffer
 */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;
	char tmp[3] = " \n";

	/* Get amount of data to copy */
	to_copy = min(count, sizeof(tmp));

	/* Read value of button */
	//printk("Value of button: %d\n", gpio_get_value(17));
	//tmp[0] = gpio_get_value(17) + '0';

	/* Copy data to user */
	not_copied = copy_to_user(user_buffer, &tmp, to_copy);

	/* Calculate data */
	delta = to_copy - not_copied;

	return delta;
}

/**
 * @brief Write data to buffer
 */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;
	char value;

	/* Tabla de valores para cada dígito '0' a '9' */
	const char gpio_values[10][7] = {
		// GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8
		// '0'
		{0, 1, 1, 1, 1, 1, 1},
		// '1'
		{0, 0, 0, 1, 0, 0, 1},
		// '2'
		{1, 0, 1, 1, 1, 1, 0},
		// '3'
		{1, 0, 1, 1, 0, 1, 1},
		// '4'
		{1, 1, 0, 1, 0, 0, 1},
		// '5'
		{1, 1, 1, 0, 0, 1, 1},
		// '6'
		{1, 1, 1, 0, 1, 1, 1},
		// '7'
		{0, 0, 1, 1, 0, 0, 1},
		// '8'
		{1, 1, 1, 1, 1, 1, 1},
		// '9'
		{1, 1, 1, 1, 0, 1, 1}
	};

	/* Get amount of data to copy */
	to_copy = min(count, sizeof(value));

	/* Copy data to user */
	not_copied = copy_from_user(&value, user_buffer, to_copy);

	if (not_copied != 0) {
		printk(KERN_WARNING "Failed to copy data from user space\n");
		return EFAULT;
	}

	/* Setting the LED */
	/* Verificar si el valor está entre '0' y '9' */
	if (value >= '0' && value <= '9') {
		int digit = value - '0'; // Convertir carácter a índice (0-9)

		/* Iterar sobre cada pin GPIO y establecer su valor correspondiente */
		for (int i = 0; i < num_pins; i++) {
			gpio_set_value(gpio_pins[i], gpio_values[digit][i]);
		}
		printk(KERN_INFO "Displayed digit: %c\n", value);
	} else {
		printk(KERN_WARNING "Entrada inválida: %c\n", value);
	}

	/* Calculate data */
	delta = to_copy - not_copied;

	return delta;
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_open(struct inode *device_file, struct file *instance) {
	printk("dev_nr - open was called!\n");
	return 0;
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_close(struct inode *device_file, struct file *instance) {
	printk("dev_nr - close was called!\n");
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
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init ModuleInit(void) {
	printk("Hello, Kernel!\n");

	/* Allocate a device nr */
	if( alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME) < 0) {
		printk("Device Nr. could not be allocated!\n");
		return -1;
	}
	printk("read_write - Device Nr. Major: %d, Minor: %d was registered!\n", my_device_nr >> 20, my_device_nr && 0xfffff);

	/* Create device class */
	if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		printk("Device class can not be created!\n");
		goto ClassError;
	}

	/* create device file */
	if(device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME) == NULL) {
		printk("Can not create device file!\n");
		goto FileError;
	}

	/* Initialize device file */
	cdev_init(&my_device, &fops);

	/* Regisering device to kernel */
	if(cdev_add(&my_device, my_device_nr, 1) == -1) {
		printk("Registering of device to kernel failed!\n");
		goto AddError;
	}
	
	
	
	
	
	
	// GPIOS
	/* GPIO 2 init */
	if(gpio_request(2, "rpi-gpio-2")) {
		printk("Can not allocate GPIO 2\n");
		goto AddError;
	}

	/* Set GPIO 2 direction */
	if(gpio_direction_output(2, 0)) {
		printk("Can not set GPIO 2 to output!\n");
		goto Gpio2Error;
	}
	
	
	/* GPIO 3 init */
	if(gpio_request(3, "rpi-gpio-3")) {
		printk("Can not allocate GPIO 3\n");
		goto AddError;
	}

	/* Set GPIO 3 direction */
	if(gpio_direction_output(3, 0)) {
		printk("Can not set GPIO 3 to output!\n");
		goto Gpio3Error;
	}
	

	/* GPIO 4 init */
	if(gpio_request(4, "rpi-gpio-4")) {
		printk("Can not allocate GPIO 4\n");
		goto AddError;
	}

	/* Set GPIO 4 direction */
	if(gpio_direction_output(4, 0)) {
		printk("Can not set GPIO 4 to output!\n");
		goto Gpio4Error;
	}
	
	
	/* GPIO 5 init */
	if(gpio_request(5, "rpi-gpio-5")) {
		printk("Can not allocate GPIO 5\n");
		goto AddError;
	}

	/* Set GPIO 5 direction */
	if(gpio_direction_output(5, 0)) {
		printk("Can not set GPIO 5 to output!\n");
		goto Gpio5Error;
	}
	
	
	/* GPIO 6 init */
	if(gpio_request(6, "rpi-gpio-6")) {
		printk("Can not allocate GPIO 6\n");
		goto AddError;
	}

	/* Set GPIO 6 direction */
	if(gpio_direction_output(6, 0)) {
		printk("Can not set GPIO 6 to output!\n");
		goto Gpio6Error;
	}
	
	
	/* GPIO 7 init */
	if(gpio_request(7, "rpi-gpio-7")) {
		printk("Can not allocate GPIO 7\n");
		goto AddError;
	}

	/* Set GPIO 7 direction */
	if(gpio_direction_output(7, 0)) {
		printk("Can not set GPIO 7 to output!\n");
		goto Gpio7Error;
	}
	
	
	/* GPIO 8 init */
	if(gpio_request(8, "rpi-gpio-8")) {
		printk("Can not allocate GPIO 8\n");
		goto AddError;
	}

	/* Set GPIO 8 direction */
	if(gpio_direction_output(8, 0)) {
		printk("Can not set GPIO 8 to output!\n");
		goto Gpio8Error;
	}
	
	
	









	return 0;

Gpio2Error:
	gpio_free(2);
Gpio3Error:
	gpio_free(3);
Gpio4Error:
	gpio_free(4);
Gpio5Error:
	gpio_free(5);
Gpio6Error:
	gpio_free(6);
Gpio7Error:
	gpio_free(7);
Gpio8Error:
	gpio_free(8);
AddError:
	device_destroy(my_class, my_device_nr);
FileError:
	class_destroy(my_class);
ClassError:
	unregister_chrdev_region(my_device_nr, 1);
	return -1;
}

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit ModuleExit(void) {
	for (int i = 0; i < num_pins; i++) {
		gpio_set_value(gpio_pins[i], 0);
		gpio_free(gpio_pins[i]);
	}
	
	cdev_del(&my_device);
	device_destroy(my_class, my_device_nr);
	class_destroy(my_class);
	unregister_chrdev_region(my_device_nr, 1);
	printk("Goodbye, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);
