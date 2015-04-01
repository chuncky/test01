/*
 * linux/arch/arm/mach-w90x900/mach-nuc920evb.c
 *
 * Based on mach-s3c2410/mach-smdk2410.c by Jonas Dietsche
 *
 * Copyright (C) 2008 Nuvoton technology corporation.
 *
 * Wan ZongShun <mcuos.com@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation;version 2 of the License.
 *
 */

#include <linux/platform_device.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <mach/map.h>

#include "nuc920.h"

static void __init nuc920evb_map_io(void)
{
	nuc920_map_io();
	nuc920_init_clocks();
}

static void __init nuc920evb_init(void)
{
	nuc920_board_init();
}

MACHINE_START(W90X900, "W90P920EVB")
	/* Maintainer: Wan ZongShun */
	.phys_io	= W90X900_PA_UART,
	.io_pg_offst	= (((u32)W90X900_VA_UART) >> 18) & 0xfffc,
	.boot_params	= 0x100,
	.map_io		= nuc920evb_map_io,
	.init_irq	= nuc900_init_irq,
	.init_machine	= nuc920evb_init,
	.timer		= &nuc900_timer,
MACHINE_END
