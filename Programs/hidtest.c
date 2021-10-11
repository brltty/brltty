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
#include "strfmt.h"
#include "parse.h"
#include "io_hid.h"
#include "hid_items.h"
#include "hid_inspect.h"

static int opt_forceUSB;
static int opt_forceBluetooth;

static char *opt_vendorIdentifier;
static char *opt_productIdentifier;

static char *opt_manufacturerName;
static char *opt_productDescription;
static char *opt_serialNumber;

static char *opt_macAddress;
static char *opt_deviceName;

static int opt_showIdentifiers;
static int opt_showDeviceIdentifier;
static int opt_showDeviceName;
static int opt_showHostPath;
static int opt_showHostDevice;
static int opt_listItems;

static int opt_echoInput;
static char *opt_inputTimeout;

BEGIN_OPTION_TABLE(programOptions)
  { .word = "usb",
    .letter = 'u',
    .setting.flag = &opt_forceUSB,
    .description = strtext("Filter for a USB device.")
  },

  { .word = "bluetooth",
    .letter = 'b',
    .setting.flag = &opt_forceBluetooth,
    .description = strtext("Filter for a Bluetooth device.")
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

  { .word = "identifiers",
    .letter = 'i',
    .setting.flag = &opt_showIdentifiers,
    .description = strtext("Show the vendor and product identifiers.")
  },

  { .word = "device-identifier",
    .letter = 'I',
    .setting.flag = &opt_showDeviceIdentifier,
    .description = strtext("Show the device identifier.")
  },

  { .word = "device-name",
    .letter = 'N',
    .setting.flag = &opt_showDeviceName,
    .description = strtext("Show the device name.")
  },

  { .word = "host-path",
    .letter = 'P',
    .setting.flag = &opt_showHostPath,
    .description = strtext("Show the host path.")
  },

  { .word = "host-device",
    .letter = 'D',
    .setting.flag = &opt_showHostDevice,
    .description = strtext("Show the host Device.")
  },

  { .word = "list",
    .letter = 'l',
    .setting.flag = &opt_listItems,
    .description = strtext("List the HID report descriptor.")
  },

  { .word = "echo",
    .letter = 'e',
    .setting.flag = &opt_echoInput,
    .description = strtext("Echo (in hexadecimal) input received from the device.")
  },

  { .word = "timeout",
    .letter = 't',
    .argument = strtext("integer"),
    .setting.string = &opt_inputTimeout,
    .description = strtext("The input timeout (in seconds).")
  },
END_OPTION_TABLE

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
  HidUSBFilter huf;
  hidInitializeUSBFilter(&huf);

  HidBluetoothFilter hbf;
  hidInitializeBluetoothFilter(&hbf);

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
      .field = &huf.vendorIdentifier,
      .parser = parseIdentifier,
    },

    { .name = "product identifier",
      .value = opt_productIdentifier,
      .field = &huf.productIdentifier,
      .parser = parseIdentifier,
    },

    { .name = "manufacturer name",
      .value = opt_manufacturerName,
      .field = &huf.manufacturerName,
      .parser = parseString,
      .flag = &opt_forceUSB,
    },

    { .name = "product description",
      .value = opt_productDescription,
      .field = &huf.productDescription,
      .parser = parseString,
      .flag = &opt_forceUSB,
    },

    { .name = "serial number",
      .value = opt_serialNumber,
      .field = &huf.serialNumber,
      .parser = parseString,
      .flag = &opt_forceUSB,
    },

    { .name = "MAC address",
      .value = opt_macAddress,
      .field = &hbf.macAddress,
      .parser = parseAddress,
      .flag = &opt_forceBluetooth,
    },

    { .name = "device name",
      .value = opt_deviceName,
      .field = &hbf.deviceName,
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

  hbf.vendorIdentifier = huf.vendorIdentifier;
  hbf.productIdentifier = huf.productIdentifier;

  if (opt_forceBluetooth) {
    *device = hidOpenBluetoothDevice(&hbf);
  } else {
    *device = hidOpenUSBDevice(&huf);
  }

  return 1;
}

static HidItemsDescriptor *
getItems (HidDevice *device) {
  static HidItemsDescriptor *items = NULL;
  if (!items) items = hidGetItems(device);
  return items;
}

static int
getInputReportSize (HidDevice *device, uint8_t identifier, size_t *size) {
  HidItemsDescriptor *items = getItems(device);
  if (!items) return 0;

  static uint8_t isInitialized;
  static uint8_t reportIdentifier;
  static HidReportSize reportSize;

  if (!isInitialized || (identifier != reportIdentifier)) {
    if (!hidGetReportSize(items, identifier, &reportSize)) return 0;
    reportIdentifier = identifier;
    isInitialized = 1;
  }

  *size = reportSize.input;
  return 1;
}

static int
writeInput (const unsigned char *from, size_t count) {
  const unsigned char *to = from + count;

  char line[(count * 3) + 1];
  STR_BEGIN(line, sizeof(line));

  while (from < to) {
    STR_PRINTF(" %02X", *from++);
  }

  STR_END;
  fprintf(outputStream, "input:%s\n", line);
  if (!canWriteOutput()) return 0;

  fflush(outputStream);
  return canWriteOutput();
}

static int
echoInput (HidDevice *device, int timeout) {
  size_t reportSize;
  unsigned char reportIdentifier = 0;
  int hasReportIdentifiers = !getInputReportSize(device, reportIdentifier, &reportSize);

  unsigned char buffer[0X1000];
  size_t bufferSize = sizeof(buffer);

  unsigned char *from = buffer;
  unsigned char *to = from;
  const unsigned char *end = from + bufferSize;

  while (hidAwaitInput(device, timeout)) {
    ssize_t result = hidReadData(device, to, (end - to), 1000, 100);

    if (result == -1) {
      logMessage(LOG_ERR, "input error: %s", strerror(errno));
      return 0;
    }

    to += result;

    while (from < to) {
      if (hasReportIdentifiers) {
        reportIdentifier = *from;

        if (!getInputReportSize(device, reportIdentifier, &reportSize)) {
          logMessage(LOG_ERR, "unexpected input report: %02X", reportIdentifier);
          return 0;
        }
      }

      size_t count = to - from;

      if (reportSize > count) {
        if (from == buffer) {
          logMessage(LOG_ERR,
            "input report too large: %02X: %"PRIsize " > %"PRIsize,
            reportIdentifier, reportSize, count
          );

          return 0;
        }

        memmove(buffer, from, count);
        from = buffer;
        to = from + count;

        break;
      }

      if (!writeInput(from, reportSize)) return 0;
      from += reportSize;
    }
  }

  return 1;
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

  int inputTimeout = 10;
  {
    static const int minimum = 1;
    static const int maximum = 99;

    if (!validateInteger(&inputTimeout, opt_inputTimeout, &minimum, &maximum)) {
      logMessage(LOG_ERR, "invalid input timeout: %s", opt_inputTimeout);
      return PROG_EXIT_SYNTAX;
    }
  }
  inputTimeout *= 1000;

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
          fprintf(outputStream, "Vendor Identifier: %04X\nProduct Identifier: %04X\n", vendor, product);
        } else {
          logMessage(LOG_WARNING, "vendor/product identifiers not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        }
      }
    }

    if (canWriteOutput()) {
      if (opt_showDeviceIdentifier) {
        const char *identifier = hidGetDeviceIdentifier(device);

        if (identifier) {
          fprintf(outputStream, "Device Identifier: %s\n", identifier);
        } else {
          logMessage(LOG_WARNING, "device reference not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        }
      }
    }

    if (canWriteOutput()) {
      if (opt_showDeviceName) {
        const char *name = hidGetDeviceName(device);

        if (name) {
          fprintf(outputStream, "Device Name: %s\n", name);
        } else {
          logMessage(LOG_WARNING, "device name not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        }
      }
    }

    if (canWriteOutput()) {
      if (opt_showHostPath) {
        const char *path = hidGetHostPath(device);

        if (path) {
          fprintf(outputStream, "Host Path: %s\n", path);
        } else {
          logMessage(LOG_WARNING, "host path not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        }
      }
    }

    if (canWriteOutput()) {
      if (opt_showHostDevice) {
        const char *hostDevice = hidGetHostDevice(device);

        if (hostDevice) {
          fprintf(outputStream, "Host Device: %s\n", hostDevice);
        } else {
          logMessage(LOG_WARNING, "host device not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        }
      }
    }

    if (canWriteOutput()) {
      if (opt_listItems) {
        HidItemsDescriptor *items = getItems(device);

        if (items) {
          hidListItems(items, listItem, NULL);
        } else {
          logMessage(LOG_ERR, "descriptor not available");
          exitStatus = PROG_EXIT_SEMANTIC;
        } 
      }
    }

    if (canWriteOutput()) {
      if (opt_echoInput) {
        if (!echoInput(device, inputTimeout)) {
          exitStatus = PROG_EXIT_FATAL;
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
