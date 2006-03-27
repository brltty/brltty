/*
** iris-gio.c for Iris-GIO device in /root/lkm/iris-gio
** 
** Made by root
** Login   <root@epita.fr>
** 
** Started on  Thu Sep 23 22:35:21 2004 root
** Last update Thu Sep 23 22:53:21 2004 root
*/

/*
** This module implements the interface bitween userspace and the GIO chip,
** present on Iris, to allow voltage switching, and modem turning on/off.
** It also allows to get battery usage, if ACPI doesn't work correctly.
*/




#include	<linux/kernel.h>
#include	<linux/module.h>
#include	<linux/interrupt.h>
#include	<linux/ioport.h>
#include	<linux/init.h>
#include	<linux/fs.h> /* filesystem interface */
#include	<asm/uaccess.h>
#include	<asm/io.h>

#include	"iris_gio.h"
#include	"gio_ioctl.h" 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yannick PLASSIARD");



#define		DEVICE_NAME "iris-GIO"
static int	device_opened = 0;
static int	major = 0;
static struct file_operations fops = {
  .open = device_open,
  .release = device_release,
  .read = device_read,
  .write = device_write,
  .ioctl = device_ioctl
};

/*
** Init and cleanup
*/

int	init_module(void)
{
  major = register_chrdev(0, DEVICE_NAME, &fops);
  if (major < 0)
    {
      printk("Failed to register character device\n");
      return major;
    }
  printk("Registered device with %d major number.\n", major);
  return 0;
}

void	cleanup_module(void)
{
  int	ret;

  printk("Unregistering character device...\n");
  if (device_opened)
    printk("There are %d open devices.\n", device_opened);
  ret = unregister_chrdev(major, DEVICE_NAME);
  if (ret < 0)
    {
      printk("Unregistering failed: %d\n", ret);
      return ;
    }
  printk("Module unloaded.\n");
}



/*
** Character device IO methods
*/


static int device_open(struct inode *inode, struct file *filp)
{
  try_module_get(THIS_MODULE);
  device_opened++;
  return 0;
}

static int device_release(struct inode *inode, struct file *filp)
{
  device_opened--;
  module_put(THIS_MODULE);
  return 0;
}

static ssize_t	device_read(struct file *filp, char *buf, size_t size, loff_t *loff)
{
  return -EINVAL;
}

static ssize_t device_write(struct file *filp, const char *bkf, size_t size, loff_t *offset)
{
  return -EINVAL;
}


/*
** All is done here, in ioctl call
*/

static int	device_ioctl(struct inode *inode,
			     struct file *filp,
			     unsigned int ioctl_number,
			     unsigned long ioctl_param)
{
  unsigned char	bit_mask;
  bit_mask = ioctl_param;
  switch (ioctl_number)
    {
    case IOCTL_SETBIT_FIRSTBYTE:
      outb(bit_mask, BASE_ADDRESS + OUTPUT_ADDR1);
      break;
    case IOCTL_SETBIT_SECONDBYTE:
      outb_p(bit_mask, BASE_ADDRESS + OUTPUT_ADDR2);
      break;
    case IOCTL_GETBIT:
      bit_mask = inb_p(BASE_ADDRESS + INPUT_ADDR);
      printk("iris_gio: get bit 0x%x\n", bit_mask);
      put_user((bit_mask), (unsigned char *)(ioctl_param));
      break;
    default:
      return -EINVAL;
      break;
    }
  return 0;
}
