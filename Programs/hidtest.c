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

static char *opt_manufacturerName;
static char *opt_productDescription;
static char *opt_serialNumber;
static char *opt_vendorIdentifier;
static char *opt_productIdentifier;

static int opt_listItems;
static int opt_showIdentifiers;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "manufacturer-name",
    .letter = 'm',
    .argument = strtext("string"),
    .setting.string = &opt_manufacturerName,
    .description = strtext("The name of the device's manufacturer.")
  },

  { .word = "product-description",
    .letter = 'd',
    .argument = strtext("string"),
    .setting.string = &opt_productDescription,
    .description = strtext("The device's product description.")
  },

  { .word = "serial-number",
    .letter = 's',
    .argument = strtext("string"),
    .setting.string = &opt_serialNumber,
    .description = strtext("The device's serial number.")
  },

  { .word = "vendor-identifier",
    .letter = 'v',
    .argument = strtext("hex"),
    .setting.string = &opt_vendorIdentifier,
    .description = strtext("The device's vendor identifier.")
  },

  { .word = "product-identifier",
    .letter = 'p',
    .argument = strtext("hex"),
    .setting.string = &opt_productIdentifier,
    .description = strtext("The device's product identifier.")
  },

  { .word = "list-items",
    .letter = 'l',
    .setting.flag = &opt_listItems,
    .description = strtext("List the report descriptor.")
  },

  { .word = "show-identifiers",
    .letter = 'i',
    .setting.flag = &opt_showIdentifiers,
    .description = strtext("Show the vendor and product identifiers.")
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

  filter.manufacturerName = opt_manufacturerName;
  filter.productDescription = opt_productDescription;
  filter.serialNumber = opt_serialNumber;

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
