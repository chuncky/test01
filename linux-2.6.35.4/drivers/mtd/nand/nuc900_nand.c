/*
 * Copyright © 2009 Nuvoton technology corporation.
 *
 * Wan ZongShun <mcuos.com@gmail.com>
 * ZM.Song <zmsong001@gmail.com> modified - 2011.01.07
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;version 2 of the License.
 *
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/blkdev.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <mach/map.h>
#include <mach/regs-clock.h>

#define REG_FMICSR   	0x00
#define REG_SMCSR    	0xa0
#define REG_SMISR    	0xac
#define REG_SMCMD    	0xb0
#define REG_SMADDR   	0xb4
#define REG_SMDATA   	0xb8
#define REG_SMTCR     0xa4

#define RESET_FMI	 0x01
#define NAND_EN		 0x08
#define READYBUSY	(0x01 << 18)

#define SWRST		   0x01
#define PSIZE		  (0x01 << 3)
#define DMARWEN		(0x03 << 1)
#define BUSWID		(0x01 << 4)
#define ECC4EN		(0x01 << 5)
#define WP		    (0x01 << 24)
#define NANDCS		(0x01 << 25)
#define ENDADDR		(0x01 << 31)


extern struct semaphore  fmi_sem;

#define read_data_reg(dev)		\
	__raw_readl((dev)->reg + REG_SMDATA)

#define write_data_reg(dev, val)	\
	__raw_writel((val), (dev)->reg + REG_SMDATA)

#define write_cmd_reg(dev, val)		\
	__raw_writel((val), (dev)->reg + REG_SMCMD)

#define write_addr_reg(dev, val)	\
	__raw_writel((val), (dev)->reg + REG_SMADDR)

struct nuc900_nand {
	struct mtd_info mtd;
	struct nand_chip chip;
	void __iomem *reg;
	struct clk *clk;
	spinlock_t lock;
};



static void dump_regs(struct nuc900_nand *dev)
{
	printk("==========================\n");
	printk("REG_FMICSR[00] : 0x%08X\n",__raw_readl((dev)->reg + REG_FMICSR));
	printk("REG_SMCSR[A0] : 0x%08X\n",__raw_readl((dev)->reg + REG_SMCSR));
	printk("REG_SMISR[AC] : 0x%08X\n",__raw_readl((dev)->reg + REG_SMISR));
	printk("REG_SMCMD[B0] : 0x%08X\n",__raw_readl((dev)->reg + REG_SMCMD));
	printk("REG_SMADDR[B4] : 0x%08X\n",__raw_readl((dev)->reg + REG_SMADDR));
	printk("REG_SMDATA[B8] : 0x%08X\n",__raw_readl((dev)->reg + REG_SMDATA));
	printk("REG_SMTCR[A4] : 0x%08X\n",__raw_readl((dev)->reg + REG_SMTCR));
	printk("==========================\n");
	
	
}

static const struct mtd_partition partitions[] = {
	{
	 .name = "NAND FS 0",
	 .offset = 0,
	 .size = 8 * 1024 * 1024
	},
	{
	 .name = "NAND FS 1",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL
	}
};

static unsigned char nuc900_nand_read_byte(struct mtd_info *mtd)
{
	unsigned char ret;
	struct nuc900_nand *nand;

	nand = container_of(mtd, struct nuc900_nand, mtd);

  if(down_interruptible(&fmi_sem)) //jhe+ 2010.12.21
	{
		printk("nuc900 mtd nand driver read_byte sem error\n");
		return -1;
	}
	
	if(__raw_readl(nand->reg + REG_FMICSR) != 0x8)
	   __raw_writel(0x08, (nand->reg + REG_FMICSR));
	
	ret = (unsigned char)read_data_reg(nand);
	up(&fmi_sem);

	return ret;
}



static void nuc900_nand_read_buf(struct mtd_info *mtd,
				 unsigned char *buf, int len)
{
	int i;
	struct nuc900_nand *nand;
	

	nand = container_of(mtd, struct nuc900_nand, mtd);
	if(down_interruptible(&fmi_sem)) //jhe+ 2010.12.21
	{
		printk("nuc900 mtd nand driver read buf sem error\n");
		return;
	}
	if(__raw_readl(nand->reg + REG_FMICSR) != 0x8)
	   __raw_writel(0x08, (nand->reg + REG_FMICSR));

	for (i = 0; i < len; i++)
		buf[i] = (unsigned char)read_data_reg(nand);
		
  up(&fmi_sem);
}

static void nuc900_nand_write_buf(struct mtd_info *mtd,
				  const unsigned char *buf, int len)
{
	int i;
	struct nuc900_nand *nand;

	nand = container_of(mtd, struct nuc900_nand, mtd);
	
	if(down_interruptible(&fmi_sem)) //jhe+ 2010.12.21
	{
		printk("nuc900 mtd nand driver write buf sem error\n");
		return;
	}
	if(__raw_readl(nand->reg + REG_FMICSR) != 0x8)
	   __raw_writel(0x08, (nand->reg + REG_FMICSR));

	for (i = 0; i < len; i++)
		write_data_reg(nand, buf[i]);
		
  up(&fmi_sem);
}

static int nuc900_verify_buf(struct mtd_info *mtd,
			     const unsigned char *buf, int len)
{
	int i;
	struct nuc900_nand *nand;

	nand = container_of(mtd, struct nuc900_nand, mtd);
	if(down_interruptible(&fmi_sem)) //jhe+ 2010.12.21
	{
		printk("nuc900 mtd nand driver verify_buf sem error\n");
		return -EFAULT;
	}
	if(__raw_readl(nand->reg + REG_FMICSR) != 0x8)
	   __raw_writel(0x08, (nand->reg + REG_FMICSR));
	
	for (i = 0; i < len; i++) {
		if (buf[i] != (unsigned char)read_data_reg(nand))
		{
			up(&fmi_sem);
			return -EFAULT;
		}
	}

  up(&fmi_sem);
	return 0;
}

/* select chip */
static void nuc900_nand_select_chip(struct mtd_info *mtd, int chip)
{
  struct nuc900_nand *nand;
	nand = container_of(mtd, struct nuc900_nand, mtd);
		
	if(down_interruptible(&fmi_sem)) //jhe+ 2010.12.21
	{
		printk("nuc900 mtd nand driver select_chip sem error\n");
		return ;
	}

	if(__raw_readl(nand->reg + REG_FMICSR) != 0x8)
	   __raw_writel(0x08, (nand->reg + REG_FMICSR));

  if(mtd->writesize==0x800)//2K page
  	__raw_writel((__raw_readl(nand->reg + REG_SMCSR)&0xfffffff0)|0x00000008, (nand->reg + REG_SMCSR));
  
  up(&fmi_sem);

}

static int nuc900_check_rb(struct nuc900_nand *nand)
{
	unsigned int val;

	
	spin_lock(&nand->lock);
	val = __raw_readl(nand->reg + REG_SMISR);
	val &= READYBUSY;
	spin_unlock(&nand->lock);

  
	return val;
}


static int nuc900_nand_devready(struct mtd_info *mtd)
{
	struct nuc900_nand *nand;
	int ready;

	nand = container_of(mtd, struct nuc900_nand, mtd);

	ready = (nuc900_check_rb(nand)) ? 1 : 0;
	return ready;
}

/* functions */
int fmiSMCheckRB(struct nuc900_nand *nand)
{

	while(1)
	{
		if (inl(nand->reg + REG_SMISR) & 0x400)
		{
			outl(0x400,nand->reg + REG_SMISR);
			return 1;
		}
	}

	return 0;
}

int ebiCheckRB(struct nuc900_nand *nand)
{
	int ret;

	ret = inl(nand->reg + REG_SMISR)& 0x40000;
	
  return ret;
}




static int  nuc900_nand_reset(struct nuc900_nand *nand)
{

	unsigned int  volatile i;

	outl(0xff,nand->reg + REG_SMCMD);	
	for (i=100; i>0; i--);
	//while(!ebiCheckRB(nand));
	
	while(!nuc900_check_rb(nand));

}


static unsigned char pre_nand_command;

static void nuc900_nand_command_lp (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	register struct nand_chip *this = mtd->priv;
	struct nuc900_nand *nand;
	nand = container_of(mtd, struct nuc900_nand, mtd);
	
	//printk("===enter %s, writesize=%x, command = %x\n",__FUNCTION__,mtd->writesize, command);
	
	if(down_interruptible(&fmi_sem)) //jhe+ 2010.12.21
	{
		printk("nuc900 mtd nand driver command sem error\n");
		return ;
	}

	if(__raw_readl(nand->reg + REG_FMICSR) != 0x8)
	   __raw_writel(0x08, (nand->reg + REG_FMICSR));

	if(pre_nand_command == NAND_CMD_READOOB && mtd->writesize == 0x200)
	{	
		pre_nand_command = 0;
		nuc900_nand_reset(nand);
	}
	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		if(mtd->writesize == 0x200){
				//column = 5;
				column = 0;
				pre_nand_command = NAND_CMD_READOOB;
		}	else	{
			command = NAND_CMD_READ0;
		}
	}
	if(command == NAND_CMD_READOOB || command == NAND_CMD_READ0)
	{

		write_cmd_reg(nand, 0x70);//jhe
		
		while(!this->dev_ready);
  	outl(0x400,nand->reg + REG_SMISR);//jhe
  }	

	if(command == NAND_CMD_SEQIN)
	{
		unsigned readcommand;
		if(mtd->writesize == 0x200)
		{
			if(column < 0x100)
  		{
				readcommand = NAND_CMD_READ0;
			}
			else if(column >= 0x200)
			{
  			column -= 0x200;
				readcommand = NAND_CMD_READOOB;
				pre_nand_command = NAND_CMD_READOOB;
			}
			else
			{
				column -= 0x100;
				readcommand = NAND_CMD_READ1;
			}
	    
			write_cmd_reg(nand, readcommand);
		}
	}
	
	
	write_cmd_reg(nand, command);
	

	if (column != -1 || page_addr != -1) {

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;

			write_addr_reg(nand, (unsigned char)column);
			if(mtd->writesize == 0x800) //2K page	
		  	write_addr_reg(nand, (unsigned char)(column >> 8)&0x0f);	
	
		}
		if (page_addr != -1) 
		{
			write_addr_reg(nand, (unsigned char)(page_addr )& 0xff);	

			/* One more address cycle for devices > 128MiB */
			if (this->chipsize > (128 << 20))
			{
				write_addr_reg(nand, (unsigned char) (page_addr>>8 )& 0xff);
				write_addr_reg(nand, (unsigned char) ((page_addr>>16 )& 0xff)|0x80000000);
			}
			else
				write_addr_reg(nand, (unsigned char) ((page_addr>>8 )& 0xff)|0x80000000);
		} 
		else
			write_addr_reg(nand, (unsigned char) ((page_addr>>8 )& 0xff)|0x80000000);

	}
	

	/*
	 * program and erase have their own busy handlers
	 * status, sequential in, and deplete1 need no delay
	 */
	switch (command) {
		case NAND_CMD_CACHEDPROG:
		case NAND_CMD_PAGEPROG:
		case NAND_CMD_ERASE1:
		case NAND_CMD_ERASE2:
		case NAND_CMD_SEQIN:
		case NAND_CMD_RNDIN:
		case NAND_CMD_STATUS:
		case NAND_CMD_DEPLETE1:
			goto out;
	
		/*
		 * read error status commands require only a short delay
		 */
		case NAND_CMD_STATUS_ERROR:
		case NAND_CMD_STATUS_ERROR0:
		case NAND_CMD_STATUS_ERROR1:
		case NAND_CMD_STATUS_ERROR2:
		case NAND_CMD_STATUS_ERROR3:
					udelay(this->chip_delay);
					goto out;
		case NAND_CMD_RESET:
					if (this->dev_ready)
						break;
					udelay(this->chip_delay);		
					nuc900_nand_reset(nand);
					goto out;
	 
	 case NAND_CMD_RNDOUT:
	 	     if(mtd->writesize == 0x800)
		     write_cmd_reg(nand, NAND_CMD_RNDOUTSTART);

		     goto out;

		case NAND_CMD_READ0:
		case NAND_CMD_READ1:
			/* Begin command latch cycle */
			if(mtd->writesize == 0x800)		
				  write_cmd_reg(nand, NAND_CMD_READSTART);

			
			if (!fmiSMCheckRB(nand))
					printk("check RB error\n");
		/* This applies to read commands */
			if (!this->dev_ready) {
				udelay (this->chip_delay);
			}
				break;
		case NAND_CMD_READOOB:
				if (!fmiSMCheckRB(nand))
					printk("check RB error\n");
					
				if (!this->dev_ready) {
					udelay (this->chip_delay);
				}
				break;
		default:
			/*
			 * If we don't have access to the busy pin, we apply the given
			 * command delay
			*/
			if (!this->dev_ready) {
				udelay (this->chip_delay);
				break;
			}
		}

out:		
		up(&fmi_sem);
	
		/* Apply this short delay always to ensure that we do wait tWB in
		 * any case on any machine. */
		ndelay (100);
	
		//nand_wait_ready(mtd);
}



static void nuc900_nand_enable(struct nuc900_nand *nand)
{
	unsigned int val;
	
	if(down_interruptible(&fmi_sem)) //jhe+ 2010.12.21
	{
		printk("nuc900 mtd nand driver nand_enable sem error\n");
		return;
	}
	
	spin_lock(&nand->lock);
	
	__raw_writel( __raw_readl(REG_CLKEN) | 0x30,REG_CLKEN);
	
	//__raw_writel(RESET_FMI, (nand->reg + REG_FMICSR));

	val = __raw_readl(nand->reg + REG_FMICSR);

	//if (!(val & NAND_EN))
		__raw_writel(NAND_EN, nand->reg + REG_FMICSR);

  __raw_writel(0x3050b,nand->reg + REG_SMTCR);
  
	val = __raw_readl(nand->reg + REG_SMCSR);

	//val &= ~(SWRST|PSIZE|DMARWEN|BUSWID|ECC4EN|NANDCS);
	//val |= WP;

	//__raw_writel(val, nand->reg + REG_SMCSR);

	__raw_writel((val&0xf8ffffc0)|0x01000020, nand->reg + REG_SMCSR);

	spin_unlock(&nand->lock);
	
	up(&fmi_sem);
}

struct nuc900_nand *nuc900_nand;
static int __devinit nuc900_nand_probe(struct platform_device *pdev)
{
	
	struct nand_chip *chip;
	int retval;
	struct resource *res;

	retval = 0;

	nuc900_nand = kzalloc(sizeof(struct nuc900_nand), GFP_KERNEL);
	if (!nuc900_nand)
		return -ENOMEM;
	chip = &(nuc900_nand->chip);

	nuc900_nand->mtd.priv	= chip;
	nuc900_nand->mtd.owner	= THIS_MODULE;
	spin_lock_init(&nuc900_nand->lock);



	chip->cmdfunc		= nuc900_nand_command_lp;
	chip->dev_ready		= nuc900_nand_devready;
	chip->read_byte		= nuc900_nand_read_byte;
	chip->write_buf		= nuc900_nand_write_buf;
	chip->read_buf		= nuc900_nand_read_buf;
	chip->select_chip  = nuc900_nand_select_chip;
	chip->verify_buf	= nuc900_verify_buf;
	chip->chip_delay	= 50;
	chip->options		= 0;
	chip->ecc.mode		= NAND_ECC_SOFT;



	nuc900_nand->reg = ioremap(W90X900_PA_FMI, W90X900_SZ_FMI);
	if (!nuc900_nand->reg) {
		retval = -ENOMEM;
		goto fail2;
	}

	nuc900_nand_enable(nuc900_nand);

	if (nand_scan(&(nuc900_nand->mtd), 1)) {
		retval = -ENXIO;
		//dump_regs(nuc900_nand);
		goto fail3;
	}

	add_mtd_partitions(&(nuc900_nand->mtd), partitions,
						ARRAY_SIZE(partitions));



	return retval;


fail3:	iounmap(nuc900_nand->reg);
fail2:	
fail1:	kfree(nuc900_nand);
printk("Nand init fail\n");
	return retval;
}

static int __devexit nuc900_nand_remove(struct platform_device *pdev)
{
	iounmap(nuc900_nand->reg);
	kfree(nuc900_nand);

	return 0;
}



static int __init nuc900_nand_init(void)
{
	nuc900_nand_probe(NULL);
	return 0;
}

static void __exit nuc900_nand_exit(void)
{
	nuc900_nand_remove(NULL);
}

module_init(nuc900_nand_init);
module_exit(nuc900_nand_exit);

MODULE_AUTHOR("Wan ZongShun <mcuos.com@gmail.com>");
MODULE_DESCRIPTION("w90p910/NUC9xx nand driver!");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:nuc900-fmi");
