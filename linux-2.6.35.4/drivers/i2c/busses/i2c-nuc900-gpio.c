/* linux/drivers/i2c/busses/i2c-gpio-nuc900.c
 *
 * Copyright (c) 2008 Nuvoton technology corporation
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Changelog:
 *
 *   2008/11/10     First version.
 *	 2008/11/26     Add group check
 */

//#include <linux/config.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <asm/io.h>

#include <mach/nuc900_gpio.h>

#define NAME "nuc900_gpio_i2c"

static int active_group_sda, active_group_scl;
static int active_pin_sda, active_pin_scl;

MODULE_AUTHOR("nuvoton");
MODULE_DESCRIPTION("Nuvoton NUC900 GPIO I2C Driver");
MODULE_LICENSE("GPL");

static void nuc900_i2c_setscl(void *data, int state)
{
        nuc900_gpio_set(active_group_scl, active_pin_scl, state);
}

static void nuc900_i2c_setsda(void *data, int state)
{
        if (state == 254)	//set to input mode
                nuc900_gpio_set_input(active_group_sda, active_pin_sda);
        else if (state == 253)	//set to output mode
                nuc900_gpio_set_output(active_group_sda, active_pin_sda);
        else
                nuc900_gpio_set(active_group_sda, active_pin_sda, state);
}

static int nuc900_i2c_getscl(void *data)
{
        return nuc900_gpio_get(active_group_scl, active_pin_scl);
}

static int nuc900_i2c_getsda(void *data)
{
        return nuc900_gpio_get(active_group_sda, active_pin_sda);
}

/* ------------------------------------------------------------------------
 * Encapsulate the above functions in the correct operations structure.
 * This is only done when more than one hardware adapter is supported.
 */

static struct i2c_algo_bit_data nuc900_i2c_data = {
        .setsda		= nuc900_i2c_setsda,
        .setscl		= nuc900_i2c_setscl,
        .getsda		= nuc900_i2c_getsda,
        .getscl		= nuc900_i2c_getscl,
        .udelay		= 10,
        .timeout	= HZ,
};

static struct i2c_adapter nuc900_i2c_ops = {
        .owner		   = THIS_MODULE,
        .class             = I2C_CLASS_HWMON | I2C_CLASS_SPD,
        .algo_data	   = &nuc900_i2c_data,
        .name	= "Nuvoton NUC900 I2C GPIO",
};

static int nuc900_i2c_init(void)
{
        int ret;

        /* Configure GPIOs for SCL pin*/
#ifdef CONFIG_I2C_GPIO_NUC900_SCL_GROUP_C
        active_group_scl = GPIO_GROUP_C;
        active_pin_scl = CONFIG_I2C_GPIO_NUC900_SCL_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SCL_GROUP_D)
        active_group_scl = GPIO_GROUP_D;
        active_pin_scl = CONFIG_I2C_GPIO_NUC900_SCL_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SCL_GROUP_E)
        active_group_scl = GPIO_GROUP_E;
        active_pin_scl = CONFIG_I2C_GPIO_NUC900_SCL_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SCL_GROUP_F)
        active_group_scl = GPIO_GROUP_F;
        active_pin_scl = CONFIG_I2C_GPIO_NUC900_SCL_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SCL_GROUP_G)
        active_group_scl = GPIO_GROUP_G;
        active_pin_scl = CONFIG_I2C_GPIO_NUC900_SCL_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SCL_GROUP_H)
        active_group_scl = GPIO_GROUP_H;
        active_pin_scl = CONFIG_I2C_GPIO_NUC900_SCL_PIN;

#endif

        /* Configure GPIOs for SDA pin*/
#ifdef CONFIG_I2C_GPIO_NUC900_SDA_GROUP_C
        active_group_sda = GPIO_GROUP_C;
        active_pin_sda = CONFIG_I2C_GPIO_NUC900_SDA_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SDA_GROUP_D)
        active_group_sda = GPIO_GROUP_D;
        active_pin_sda = CONFIG_I2C_GPIO_NUC900_SDA_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SDA_GROUP_E)
        active_group_sda = GPIO_GROUP_E;
        active_pin_sda = CONFIG_I2C_GPIO_NUC900_SDA_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SDA_GROUP_F)
        active_group_sda = GPIO_GROUP_F;
        active_pin_sda = CONFIG_I2C_GPIO_NUC900_SDA_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SDA_GROUP_G)
        active_group_sda = GPIO_GROUP_G;
        active_pin_sda = CONFIG_I2C_GPIO_NUC900_SDA_PIN;

#elif defined(CONFIG_I2C_GPIO_NUC900_SDA_GROUP_H)
        active_group_sda = GPIO_GROUP_H;
        active_pin_sda = CONFIG_I2C_GPIO_NUC900_SDA_PIN;

#endif

        //pull high the both pin
        nuc900_i2c_setscl(NULL, 1);
        nuc900_i2c_setsda(NULL, 1);

        ret = nuc900_gpio_configure(active_group_scl, active_pin_scl);
        ret = nuc900_gpio_configure(active_group_sda, active_pin_sda);

        if (!ret) {
                printk(KERN_ERR NAME ": adapter %s registration failed\n",
                       nuc900_i2c_ops.name);
                return -ENODEV;
        }

        if (i2c_bit_add_bus(&nuc900_i2c_ops) < 0) {
                printk(KERN_ERR NAME ": adapter %s registration failed\n",
                       nuc900_i2c_ops.name);
                return -ENODEV;
        }

        return 0;
}

static void nuc900_i2c_cleanup(void)
{
        i2c_del_adapter(&nuc900_i2c_ops);
}

module_init(nuc900_i2c_init);
module_exit(nuc900_i2c_cleanup);

