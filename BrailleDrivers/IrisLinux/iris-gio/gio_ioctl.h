/*
** gio_ioctl.h for Iris GIO Ioctls in /root/lkm/iris-gio
** 
** Made by root
** Login   <root@epita.fr>
** 
** Started on  Thu Sep 23 22:47:36 2004 root
** Last update Thu Sep 23 22:49:21 2004 root
*/

#ifndef		__GIO_IOCTL_H__
# define	__GIO_IOCTL_H__

#include	<linux/ioctl.h>

#define IO_D0	0x01	/* Apply operation on bit 0. */
#define IO_D1	0x02	/* Apply operation on bit 1. */
#define IO_D2	0x04	/* Apply operation on bit 2. */
#define IO_D3	0x08	/* Apply operation on bit 3. */
#define IO_D4	0x10	/* Apply operation on bit 4. */
#define IO_D5	0x20	/* Apply operation on bit 5. */
#define IO_D6	0x40	/* Apply operation on bit 6. */
#define IO_D7	0x80	/* Apply operation on bit 7. */
#define IO_DX	0xFF	/* Apply operation on all bits. */
#define		BASE_ADDRESS	0x340

#define		INPUT_ADDR	0
#define		OUTPUT_ADDR1	1
#define		OUTPUT_ADDR2	2

/*
** Next are IOCTL defines 
*/

#define		IOCTL_SETBIT_FIRSTBYTE _IOR('b', 0x10, unsigned char)
#define		IOCTL_SETBIT_SECONDBYTE _IOR('b', 0x11, unsigned char)
#define		IOCTL_GETBIT	_IOR('b', 0x12, unsigned char *)

#endif
