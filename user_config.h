#ifndef CONFIG_H
#define CONFIG_H

#define boot_pin_init()                          \
    system("echo 112 > /sys/class/gpio/export"); \
    system("echo out > /sys/class/gpio/gpio112/direction")

#define reset_pin_init()                         \
    system("echo 122 > /sys/class/gpio/export"); \
    system("echo out > /sys/class/gpio/gpio122/direction")

#define boot_pin_high()  system("echo 1 > /sys/class/gpio/gpio112/value")
#define boot_pin_low()   system("echo 0 > /sys/class/gpio/gpio112/value")
#define reset_pin_high() system("echo 1 > /sys/class/gpio/gpio122/value")
#define reset_pin_low()  system("echo 0 > /sys/class/gpio/gpio122/value")

#endif