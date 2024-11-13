// gpio_driver.c
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

/* Información Meta */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ICMF y Johannes 4 GNU/Linux");
MODULE_DESCRIPTION(
    "Driver GPIO combinado para displays de 7 segmentos y lectura de entradas");

/* Variables para el dispositivo y clase de dispositivo */
static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

/* Definición de pines GPIO para 3 displays */
const int display_pins[3][7] = {
    {2, 3, 4, 5, 6, 7, 8},         // Display 1
    {10, 11, 12, 13, 14, 15, 16},  // Display 2
    {17, 18, 19, 20, 21, 22, 23}   // Display 3
};
const int num_displays = 3;
const int num_pins_per_display = 7;

#define DRIVER_NAME "my_gpio_driver"
#define DRIVER_CLASS "MyModuleClass"
#define GPIO_INPUT_PIN1 9
#define GPIO_INPUT_PIN2 24
#define GPIO_INPUT_PIN3 25

/* Tabla de valores para cada dígito '0' a '9' */
const char gpio_values[10][7] = {
    // a, b, c, d, e, f, g
    {0, 1, 1, 1, 1, 1, 1},  // '0'
    {0, 0, 0, 1, 0, 0, 1},  // '1'
    {1, 0, 1, 1, 1, 1, 0},  // '2'
    {1, 0, 1, 1, 0, 1, 1},  // '3'
    {1, 1, 0, 1, 0, 0, 1},  // '4'
    {1, 1, 1, 0, 0, 1, 1},  // '5'
    {1, 1, 1, 0, 1, 1, 1},  // '6'
    {0, 0, 1, 1, 0, 0, 1},  // '7'
    {1, 1, 1, 1, 1, 1, 1},  // '8'
    {1, 1, 1, 1, 0, 1, 1}   // '9'
};

/**
 * @brief Leer datos de los pines GPIO de entrada y devolverlos al espacio de
 * usuario
 */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count,
                           loff_t *offs) {
  char tmp[4];  // Espacio para 3 caracteres y el terminador nulo
  int to_copy;
  int not_copied;
  int delta;

  /* Leer valores de los botones */
  tmp[0] = gpio_get_value(GPIO_INPUT_PIN1) + '0';
  tmp[1] = gpio_get_value(GPIO_INPUT_PIN2) + '0';
  tmp[2] = gpio_get_value(GPIO_INPUT_PIN3) + '0';
  tmp[3] = '\0';  // Terminador nulo

  printk(KERN_INFO "gpio_driver: Valores de entrada: %s\n", tmp);

  /* Verificar si ya se han leído todos los datos */
  if (*offs > 0) {
    return 0;  // Indicar EOF
  }

  /* Determinar la cantidad a copiar */
  to_copy = min(count, (size_t)3);

  /* Copiar datos al espacio de usuario */
  not_copied = copy_to_user(user_buffer, tmp, to_copy);

  /* Calcular la cantidad de datos copiados */
  delta = to_copy - not_copied;

  /* Actualizar el desplazamiento */
  *offs += delta;

  return delta;
}

/**
 * @brief Escribir datos en los displays
 */
static ssize_t driver_write(struct file *File, const char *user_buffer,
                            size_t count, loff_t *offs) {
  char digits[3];

  /* Verificar que se han enviado al menos 3 caracteres */
  if (count < 3) {
    printk(KERN_WARNING
           "gpio_driver: Datos insuficientes. Se esperaban 3 dígitos.\n");
    return -EINVAL;
  }

  /* Copiar 3 caracteres desde el espacio de usuario */
  if (copy_from_user(digits, user_buffer, 3)) {
    printk(KERN_WARNING
           "gpio_driver: Error al copiar datos desde el espacio de usuario\n");
    return -EFAULT;
  }

  /* Actualizar cada display */
  for (int i = 0; i < num_displays; i++) {
    if (digits[i] >= '0' && digits[i] <= '9') {
      int digit = digits[i] - '0';  // Convertir carácter a índice (0-9)

      /* Establecer los valores de GPIO para cada segmento */
      for (int j = 0; j < num_pins_per_display; j++) {
        gpio_set_value(display_pins[i][j], gpio_values[digit][j]);
      }
      printk(KERN_INFO "gpio_driver: Display %d configurado a '%c'\n", i + 1,
             digits[i]);
    } else {
      printk(KERN_WARNING
             "gpio_driver: Entrada inválida para Display %d: '%c'\n",
             i + 1, digits[i]);
    }
  }

  return 3;
}

/**
 * @brief Esta función se llama cuando el archivo de dispositivo se abre
 */
static int driver_open(struct inode *device_file, struct file *instance) {
  printk(KERN_INFO "gpio_driver: Open fue llamado!\n");
  return 0;
}

/**
 * @brief Esta función se llama cuando el archivo de dispositivo se cierra
 */
static int driver_close(struct inode *device_file, struct file *instance) {
  printk(KERN_INFO "gpio_driver: Close fue llamado!\n");
  return 0;
}

static struct file_operations fops = {.owner = THIS_MODULE,
                                      .open = driver_open,
                                      .release = driver_close,
                                      .read = driver_read,
                                      .write = driver_write};

/**
 * @brief Esta función se llama cuando el módulo se carga en el kernel
 */
static int __init ModuleInit(void) {
  int ret;
  printk(KERN_INFO "gpio_driver: Inicializando el driver GPIO combinado\n");

  /* Asignar un número de dispositivo */
  ret = alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME);
  if (ret < 0) {
    printk(KERN_ERR
           "gpio_driver: No se pudo asignar el número de dispositivo!\n");
    return ret;
  }
  printk(KERN_INFO
         "gpio_driver: Dispositivo Nr. Major: %d, Minor: %d fue registrado!\n",
         MAJOR(my_device_nr), MINOR(my_device_nr));

  /* Crear clase de dispositivo */
  my_class = class_create(THIS_MODULE, DRIVER_CLASS);
  if (IS_ERR(my_class)) {
    printk(KERN_ERR "gpio_driver: No se pudo crear la clase de dispositivo!\n");
    unregister_chrdev_region(my_device_nr, 1);
    return PTR_ERR(my_class);
  }

  /* Crear archivo de dispositivo */
  if (IS_ERR(device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME))) {
    printk(KERN_ERR
           "gpio_driver: No se pudo crear el archivo de dispositivo!\n");
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);
    return PTR_ERR(my_class);
  }

  /* Inicializar y registrar cdev */
  cdev_init(&my_device, &fops);
  ret = cdev_add(&my_device, my_device_nr, 1);
  if (ret < 0) {
    printk(KERN_ERR
           "gpio_driver: Falló el registro del dispositivo en el kernel!\n");
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);
    return ret;
  }

  /* Inicializar todos los GPIOs para los 3 displays */
  for (int i = 0; i < num_displays; i++) {
    for (int j = 0; j < num_pins_per_display; j++) {
      if (gpio_request(display_pins[i][j], "rpi-gpio")) {
        printk(KERN_ERR
               "gpio_driver: No se pudo asignar GPIO %d para Display %d\n",
               display_pins[i][j], i + 1);
        goto GpioError;
      }

      if (gpio_direction_output(display_pins[i][j], 0)) {
        printk(KERN_ERR
               "gpio_driver: No se pudo configurar GPIO %d como salida para "
               "Display %d!\n",
               display_pins[i][j], i + 1);
        goto GpioError;
      }
    }
  }

  /* Inicializar GPIO_INPUT_PIN1 */
  if (gpio_request(GPIO_INPUT_PIN1, "rpi-gpio-input1")) {
    printk(KERN_ERR "gpio_driver: No se pudo asignar GPIO %d\n",
           GPIO_INPUT_PIN1);
    goto GpioError;
  }

  /* Configurar dirección de GPIO_INPUT_PIN1 */
  if (gpio_direction_input(GPIO_INPUT_PIN1)) {
    printk(KERN_ERR
           "gpio_driver: No se pudo configurar GPIO %d como entrada!\n",
           GPIO_INPUT_PIN1);
    goto GpioError;
  }

  /* Inicializar GPIO_INPUT_PIN2 */
  if (gpio_request(GPIO_INPUT_PIN2, "rpi-gpio-input2")) {
    printk(KERN_ERR "gpio_driver: No se pudo asignar GPIO %d\n",
           GPIO_INPUT_PIN2);
    goto GpioError;
  }

  /* Configurar dirección de GPIO_INPUT_PIN2 */
  if (gpio_direction_input(GPIO_INPUT_PIN2)) {
    printk(KERN_ERR
           "gpio_driver: No se pudo configurar GPIO %d como entrada!\n",
           GPIO_INPUT_PIN2);
    goto GpioError;
  }

  /* Inicializar GPIO_INPUT_PIN3 */
  if (gpio_request(GPIO_INPUT_PIN3, "rpi-gpio-input3")) {
    printk(KERN_ERR "gpio_driver: No se pudo asignar GPIO %d\n",
           GPIO_INPUT_PIN3);
    goto GpioError;
  }

  /* Configurar dirección de GPIO_INPUT_PIN3 */
  if (gpio_direction_input(GPIO_INPUT_PIN3)) {
    printk(KERN_ERR
           "gpio_driver: No se pudo configurar GPIO %d como entrada!\n",
           GPIO_INPUT_PIN3);
    goto GpioError;
  }

  printk(KERN_INFO
         "gpio_driver: Todos los GPIOs se han inicializado correctamente\n");
  return 0;

GpioError:
  /* Liberar GPIOs en caso de error */
  for (int i = 0; i < num_displays; i++) {
    for (int j = 0; j < num_pins_per_display; j++) {
      gpio_free(display_pins[i][j]);
    }
  }
  gpio_free(GPIO_INPUT_PIN1);
  gpio_free(GPIO_INPUT_PIN2);
  gpio_free(GPIO_INPUT_PIN3);

  cdev_del(&my_device);
  device_destroy(my_class, my_device_nr);
  class_destroy(my_class);
  unregister_chrdev_region(my_device_nr, 1);
  return -1;
}

/**
 * @brief Esta función se llama cuando el módulo se elimina del kernel
 */
static void __exit ModuleExit(void) {
  /* Apagar displays y liberar GPIOs */
  for (int i = 0; i < num_displays; i++) {
    for (int j = 0; j < num_pins_per_display; j++) {
      gpio_set_value(display_pins[i][j], 0);  // Apagar segmentos
      gpio_free(display_pins[i][j]);
    }
  }

  /* Liberar GPIOs de entrada */
  gpio_free(GPIO_INPUT_PIN1);
  gpio_free(GPIO_INPUT_PIN2);
  gpio_free(GPIO_INPUT_PIN3);

  /* Eliminar dispositivo y clase */
  cdev_del(&my_device);
  device_destroy(my_class, my_device_nr);
  class_destroy(my_class);
  unregister_chrdev_region(my_device_nr, 1);
  printk(KERN_INFO "gpio_driver: Adiós, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);
