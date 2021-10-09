/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "program.h"
#include "options.h"
#include "log.h"
#include "io_hid.h"
#include "hid_items.h"

static int opt_listItems;
static int opt_showIdentifiers;

static int opt_forceUSB;
static int opt_forceBluetooth;

static char *opt_vendorIdentifier;
static char *opt_productIdentifier;

static char *opt_manufacturerName;
static char *opt_productDescription;
static char *opt_serialNumber;

static char *opt_macAddress;
static char *opt_deviceName;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "list",
    .letter = 'l',
    .setting.flag = &opt_listItems,
    .description = strtext("List the HID report descriptor.")
  },

  { .word = "identifiers",
    .letter = 'i',
    .setting.flag = &opt_showIdentifiers,
    .description = strtext("Show the vendor and product identifiers.")
  },

  { .word = "usb",
    .letter = 'u',
    .setting.flag = &opt_forceUSB,
    .description = strtext("Look for a USB device.")
  },

  { .word = "bluetooth",
    .letter = 'b',
    .setting.flag = &opt_forceBluetooth,
    .description = strtext("Look for a Bluetooth device.")
  },

  { .word = "vendor",
    .letter = 'v',
    .argument = strtext("identifier"),
    .setting.string = &opt_vendorIdentifier,
    .description = strtext("Match the vendor identifier.")
  },

  { .word = "product",
    .letter = 'p',
    .argument = strtext("identifier"),
    .setting.string = &opt_productIdentifier,
    .description = strtext("Match the product identifier.")
  },

  { .word = "manufacturer",
    .letter = 'm',
    .argument = strtext("string"),
    .setting.string = &opt_manufacturerName,
    .description = strtext("Match the start of the manufacturer name (USB only).")
  },

  { .word = "description",
    .letter = 'd',
    .argument = strtext("string"),
    .setting.string = &opt_productDescription,
    .description = strtext("Match the start of the product description (USB only).")
  },

  { .word = "serial-number",
    .letter = 's',
    .argument = strtext("string"),
    .setting.string = &opt_serialNumber,
    .description = strtext("Match the start of the serial number (USB only).")
  },

  { .word = "address",
    .letter = 'a',
    .argument = strtext("string"),
    .setting.string = &opt_macAddress,
    .description = strtext("Match the full MAC address (Bluetooth only).")
  },

  { .word = "name",
    .letter = 'n',
    .argument = strtext("string"),
    .setting.string = &opt_deviceName,
    .description = strtext("Match the start of the device name (Bluetooth only).")
  },
END_OPTION_TABLE

static int
parseString (const char *value, void *field) {
  const char **string = field;
  *string = value;
  return 1;
}

static int
parseIdentifier (const char *value, void *field) {
  uint16_t *identifier = field;
  return hidParseIdentifier(identifier, value);
}

static int
parseAddress (const char *value, void *field) {
  {
    const char *byte = value;
    unsigned int state = 0;
    unsigned int octets = 0;
    const char *digits = "0123456789ABCDEFabcdef";

    while (*byte) {
      if (!state) octets += 1;

      if (++state < 3) {
        if (!strchr(digits, *byte)) return 0;
      } else {
        if (*byte != ':') return 0;
        state = 0;
      }

      byte += 1;
    }

    if (octets != 6) return 0;
    if (state != 2) return 0;
  }

  return parseString(value, field);
}

static int
openDevice (HidDevice **device) {
  HidDeviceFilter_USB uf;
  hidInitializeDeviceFilter_USB(&uf);

  HidDeviceFilter_Bluetooth bf;
  hidInitializeDeviceFilter_Bluetooth(&bf);

  typedef struct {
    const char *name;
    const char *value;
    void *field;
    int (*parser) (const char *value, void *field);
    int *flag;
  } FilterEntry;

  const FilterEntry filterTable[] = {
    { .name = "vendor identifier",
      .value = opt_vendorIdentifier,
      .field = &uf.vendorIdentifier,
      .parser = parseIdentifier,
    },

    { .name = "product identifier",
      .value = opt_productIdentifier,
      .field = &uf.productIdentifier,
      .parser = parseIdentifier,
    },

    { .name = "manufacturer name",
      .value = opt_manufacturerName,
      .field = &uf.manufacturerName,
      .parser = parseString,
      .flag = &opt_forceUSB,
    },

    { .name = "product description",
      .value = opt_productDescription,
      .field = &uf.productDescription,
      .parser = parseString,
      .flag = &opt_forceUSB,
    },

    { .name = "serial number",
      .value = opt_serialNumber,
      .field = &uf.serialNumber,
      .parser = parseString,
      .flag = &opt_forceUSB,
    },

    { .name = "MAC address",
      .value = opt_macAddress,
      .field = &bf.macAddress,
      .parser = parseAddress,
      .flag = &opt_forceBluetooth,
    },

    { .name = "device name",
      .value = opt_deviceName,
      .field = &bf.deviceName,
      .parser = parseString,
      .flag = &opt_forceBluetooth,
    },
  };

  const FilterEntry *filter = filterTable;
  const FilterEntry *end = filter + ARRAY_COUNT(filterTable);

  while (filter < end) {
    if (filter->value && *filter->value) {
      if (!filter->parser(filter->value, filter->field)) {
        logMessage(LOG_ERR, "invalid %s: %s", filter->name, filter->value);
        return 0;
      }

      if (filter->flag) {
        *filter->flag = 1;

        if (opt_forceUSB && opt_forceBluetooth) {
          logMessage(LOG_ERR, "conflicting filter options");
          return 0;
        }
      }
    }

    filter += 1;
  }

  bf.vendorIdentifier = uf.vendorIdentifier;
  bf.productIdentifier = uf.productIdentifier;

  if (opt_forceBluetooth) {
    *device = hidOpenDevice_Bluetooth(&bf);
  } else {
    *device = hidOpenDevice_USB(&uf);
  }

  return 1;
}

static FILE *outputStream;
static int outputError;

static int
canWriteOutput (void) {
  if (outputError) return 0;
  if (!ferror(outputStream)) return 1;

  outputError = errno;
  return 0;
}

static int
listItem (const char *line, void *data) {
  FILE *stream = outputStream;
  fprintf(stream, "%s\n", line);
  return canWriteOutput();
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "hidtest",
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  if (argc) {
    logMessage(LOG_ERR, "too many parameters");
    return PROG_EXIT_SYNTAX;
  }

  outputStream = stdout;
  outputError = 0;

  HidDevice *device = NULL;
  if (!openDevice(&device)) return PROG_EXIT_SYNTAX;
  ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;

  if (device) {
    if (canWriteOutput()) {
      if (opt_showIdentifiers) {
        uint16_t vendor;
        uint16_t product;

        if (hidGetIdentifiers(device, &vendor, &product)) {
          fprintf(outputStream, "%04X:%04X\n", vendor, product);
        } else {
          logMessage(LOG_WARNING, "identifiers not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        }
      }
    }

    if (canWriteOutput()) {
      if (opt_listItems) {
        HidItemsDescriptor *items = hidGetItems(device);

        if (items) {
          hidListItems(items, listItem, NULL);
          free(items);
        } else {
          logMessage(LOG_ERR, "descriptor not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        } 
      }
    }

    hidCloseDevice(device);
  } else {
    logMessage(LOG_ERR, "device not found");
    exitStatus = PROG_EXIT_SEMANTIC;
  }

  if (outputError) {
    logMessage(LOG_ERR, "output error: %s", strerror(outputError));
    exitStatus = PROG_EXIT_FATAL;
  }

  return exitStatus;
}
