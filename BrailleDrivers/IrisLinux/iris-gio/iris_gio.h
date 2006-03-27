/*
** iris-gio.h for Chardevice in /root/lkm/iris-gio
** 
** Made by root
** Login   <root@epita.fr>
** 
** Started on  Thu Sep 23 10:53:34 2004 root
** Last update Thu Sep 23 22:50:03 2004 root
*/

#ifndef		__HELLO3_H__
# define	__HELLO3_H__


/*
** Bits definition
*/


/*
** headers
*/

static int	device_open(struct inode *, struct file *);
static int	device_release(struct inode *, struct file *);
static ssize_t	device_read(struct file *, char *, size_t, loff_t *);
static ssize_t	device_write(struct file *, const char *, size_t, loff_t *);
static int device_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

#endif
