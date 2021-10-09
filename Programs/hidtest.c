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

static char *opt_vendorIdentifier;
static char *opt_productIdentifier;

static char *opt_usbManufacturerName;
static char *opt_usbProductDescription;
static char *opt_usbSerialNumber;

static char *opt_bluetoothAddress;
static char *opt_bluetoothName;

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
    .setting.string = &opt_usbManufacturerName,
    .description = strtext("USB - Match the start of the manufacturer name.")
  },

  { .word = "description",
    .letter = 'd',
    .argument = strtext("string"),
    .setting.string = &opt_usbProductDescription,
    .description = strtext("USB - Match the start of the product description.")
  },

  { .word = "serial-number",
    .letter = 's',
    .argument = strtext("string"),
    .setting.string = &opt_usbSerialNumber,
    .description = strtext("USB - Match the start of the serial number.")
  },

  { .word = "address",
    .letter = 'a',
    .argument = strtext("string"),
    .setting.string = &opt_bluetoothAddress,
    .description = strtext("Bluetooth - Match the full MAC address.")
  },

  { .word = "name",
    .letter = 'n',
    .argument = strtext("string"),
    .setting.string = &opt_bluetoothName,
    .description = strtext("Bluetooth - Match the start of the device name.")
  },
END_OPTION_TABLE

static FILE *outputStream;
int outputError;

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

  HidDeviceFilter_USB filter;
  hidInitializeDeviceFilter_USB(&filter);

  filter.manufacturerName = opt_usbManufacturerName;
  filter.productDescription = opt_usbProductDescription;
  filter.serialNumber = opt_usbSerialNumber;

  {
    int ok = 1;

    if (*opt_vendorIdentifier) {
      if (!hidParseIdentifier(&filter.vendorIdentifier, opt_vendorIdentifier)) {
        logMessage(LOG_ERR, "invalid vendor identifier: %s", opt_vendorIdentifier);
        ok = 0;
      }
    }

    if (*opt_productIdentifier) {
      if (!hidParseIdentifier(&filter.productIdentifier, opt_productIdentifier)) {
        logMessage(LOG_ERR, "invalid product identifier: %s", opt_productIdentifier);
        ok = 0;
      }
    }

    if (!ok) return PROG_EXIT_SYNTAX;
  }

  outputStream = stdout;
  outputError = 0;

  ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;
  HidDevice *device = hidOpenDevice_USB(&filter);

  if (device) {
    if (canWriteOutput()) {
      if (opt_showIdentifiers) {
        uint16_t vendor;
        uint16_t product;

        if (hidGetIdentifiers(device, &vendor, &product)) {
          fprintf(outputStream, "%04X:%04X\n", vendor, product);
        } else {
          logMessage(LOG_WARNING, "identifiers not available");
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
