#ifndef VOYAGER_H
#define VOYAGER_H

/* Ioctl request codes */
#define VOYAGER_GET_INFO 0
#define VOYAGER_DISPLAY_ON 2
#define VOYAGER_DISPLAY_OFF 3
#define VOYAGER_BUZZ 4

/* Base minor for the char dev */
#define VOYAGER_DEFAULT_MINOR 144        /* some unassigned USB minor */

/* Number of supported devices, and range of covered minors */
#define MAX_NR_VOYAGER_DEVS 16

/* Size of some fields */
#define VOYAGER_HWVER_SIZE 2
#define VOYAGER_FWVER_SIZE 200 /* arbitrary, a long string */
#define VOYAGER_SERIAL_BIN_SIZE 8
#define VOYAGER_SERIAL_SIZE ((2*VOYAGER_SERIAL_BIN_SIZE)+1)

struct voyager_info {
  unsigned char driver_version[12];
  unsigned char driver_banner[200];

  int display_length;
  /* All other char[] fields are strings except this one.
     Hardware version: first byte is major, second byte is minor. */
  unsigned char hwver[VOYAGER_HWVER_SIZE];
  unsigned char fwver[VOYAGER_FWVER_SIZE];
  unsigned char serialnum[VOYAGER_SERIAL_SIZE];
};

#endif
