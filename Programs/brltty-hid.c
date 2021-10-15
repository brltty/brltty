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
#include <stdarg.h>
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

static int opt_matchUSBDevices;
static int opt_matchBluetoothDevices;

static char *opt_matchVendorIdentifier;
static char *opt_matchProductIdentifier;

static char *opt_matchManufacturerName;
static char *opt_matchProductDescription;
static char *opt_matchSerialNumber;

static char *opt_matchDeviceAddress;
static char *opt_matchDeviceName;

static int opt_showDeviceIdentifiers;
static int opt_showDeviceAddress;
static int opt_showDeviceName;
static int opt_showHostPath;
static int opt_showHostDevice;

static int opt_listItems;
static int opt_listReports;

static char *opt_readReport;
static char *opt_readFeature;

static char *opt_writeReport;
static char *opt_writeFeature;

static int opt_echoInput;
static char *opt_inputTimeout;

static const char *parseBytesHelp[] = {
  strtext("Bytes may be separated by whitespace."),
  strtext("Each byte is either two hexadecimal digits or [zero or more braille dot numbers within brackets]."),
  strtext("A byte may optionally be followed by an asterisk [*] and a decimal count (1 if not specified)."),
  strtext("The first byte is the report number (00 for no report number)."),
};

static
STR_BEGIN_FORMATTER(formatParseBytesHelp, unsigned int index)
  switch (index) {
    case 0: {
      const char *const *sentence = parseBytesHelp;
      const char *const *end = sentence + ARRAY_COUNT(parseBytesHelp);

      while (sentence < end) {
        if (STR_LENGTH) STR_PRINTF(" ");
        STR_PRINTF("%s", gettext(*sentence));
        sentence += 1;
      }

      break;
    }

    default:
      break;
  }
STR_END_FORMATTER

BEGIN_OPTION_TABLE(programOptions)
  { .word = "match-usb-devices",
    .letter = 'u',
    .setting.flag = &opt_matchUSBDevices,
    .description = strtext("Filter for a USB device (the default if not ambiguous).")
  },

  { .word = "match-bluetooth-devices",
    .letter = 'b',
    .setting.flag = &opt_matchBluetoothDevices,
    .description = strtext("Filter for a Bluetooth device.")
  },

  { .word = "match-vendor-identifier",
    .letter = 'v',
    .argument = strtext("identifier"),
    .setting.string = &opt_matchVendorIdentifier,
    .description = strtext("Match the vendor identifier (four hexadecimal digits).")
  },

  { .word = "match-product-identifier",
    .letter = 'p',
    .argument = strtext("identifier"),
    .setting.string = &opt_matchProductIdentifier,
    .description = strtext("Match the product identifier (four hexadecimal digits).")
  },

  { .word = "match-manufacturer-name",
    .letter = 'm',
    .argument = strtext("string"),
    .setting.string = &opt_matchManufacturerName,
    .description = strtext("Match the start of the manufacturer name (USB only).")
  },

  { .word = "match-product-description",
    .letter = 'd',
    .argument = strtext("string"),
    .setting.string = &opt_matchProductDescription,
    .description = strtext("Match the start of the product description (USB only).")
  },

  { .word = "match-serial-number",
    .letter = 's',
    .argument = strtext("string"),
    .setting.string = &opt_matchSerialNumber,
    .description = strtext("Match the start of the serial number (USB only).")
  },

  { .word = "match-device-address",
    .letter = 'a',
    .argument = strtext("octets"),
    .setting.string = &opt_matchDeviceAddress,
    .description = strtext("Match the full device address (Bluetooth only - all six two-digit, hexadecimal octets separated by a colon [:]).")
  },

  { .word = "match-device-name",
    .letter = 'n',
    .argument = strtext("string"),
    .setting.string = &opt_matchDeviceName,
    .description = strtext("Match the start of the device name (Bluetooth only).")
  },

  { .word = "show-device-identifiers",
    .letter = 'I',
    .setting.flag = &opt_showDeviceIdentifiers,
    .description = strtext("Show the vendor and product identifiers.")
  },

  { .word = "show-device-address",
    .letter = 'A',
    .setting.flag = &opt_showDeviceAddress,
    .description = strtext("Show the device address (USB serial number, Bluetooth device address, etc).")
  },

  { .word = "show-device-name",
    .letter = 'N',
    .setting.flag = &opt_showDeviceName,
    .description = strtext("Show the device name (USB manufacturer and/or product strings, Bluetooth device name, etc).")
  },

  { .word = "show-host-path",
    .letter = 'P',
    .setting.flag = &opt_showHostPath,
    .description = strtext("Show the host path (USB topology, Bluetooth host controller address, etc).")
  },

  { .word = "show-host-device",
    .letter = 'D',
    .setting.flag = &opt_showHostDevice,
    .description = strtext("Show the host device (usually its absolute path).")
  },

  { .word = "list-items",
    .letter = 'l',
    .setting.flag = &opt_listItems,
    .description = strtext("List the HID report descriptor's items.")
  },

  { .word = "list-reports",
    .letter = 'L',
    .setting.flag = &opt_listReports,
    .description = strtext("List each report's identifier and sizes.")
  },

  { .word = "read-report",
    .letter = 'r',
    .argument = strtext("identifier"),
    .setting.string = &opt_readReport,
    .description = strtext("Read (get) an input report (two hexadecimal digits).")
  },

  { .word = "read-feature",
    .letter = 'R',
    .argument = strtext("identifier"),
    .setting.string = &opt_readFeature,
    .description = strtext("Read (get) a feature report (two hexadecimal digits).")
  },

  { .word = "write-report",
    .letter = 'w',
    .argument = strtext("bytes"),
    .setting.string = &opt_writeReport,
    .flags = OPT_Format,
    .description = strtext("Write (set) an output report. %s"),
    .strings.format = formatParseBytesHelp
  },

  { .word = "write-feature",
    .letter = 'W',
    .argument = strtext("bytes"),
    .setting.string = &opt_writeFeature,
    .flags = OPT_Format,
    .description = strtext("Write (set) a feature report. %s"),
    .strings.format = formatParseBytesHelp
  },

  { .word = "echo-input",
    .letter = 'e',
    .setting.flag = &opt_echoInput,
    .description = strtext("Echo (in hexadecimal) input received from the device.")
  },

  { .word = "input-timeout",
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

static int writeBytesLine (
  const char *format,
  const unsigned char *from, size_t count,
  ...
) PRINTF(1, 4);

static int
writeBytesLine (const char *format, const unsigned char *from, size_t count, ...) {
  const unsigned char *to = from + count;

  char bytes[(count * 3) + 1];
  STR_BEGIN(bytes, sizeof(bytes));

  while (from < to) {
    STR_PRINTF(" %02X", *from++);
  }
  STR_END;

  char label[0X100];
  {
    va_list arguments;
    va_start(arguments, count);
    vsnprintf(label, sizeof(label), format, arguments);
    va_end(arguments);
  }

  fprintf(outputStream, "%s:%s\n", label, bytes);
  if (!canWriteOutput()) return 0;

  fflush(outputStream);
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
parseMACAddress (const char *value, void *field) {
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
      .value = opt_matchVendorIdentifier,
      .field = &huf.vendorIdentifier,
      .parser = parseIdentifier,
    },

    { .name = "product identifier",
      .value = opt_matchProductIdentifier,
      .field = &huf.productIdentifier,
      .parser = parseIdentifier,
    },

    { .name = "manufacturer name",
      .value = opt_matchManufacturerName,
      .field = &huf.manufacturerName,
      .parser = parseString,
      .flag = &opt_matchUSBDevices,
    },

    { .name = "product description",
      .value = opt_matchProductDescription,
      .field = &huf.productDescription,
      .parser = parseString,
      .flag = &opt_matchUSBDevices,
    },

    { .name = "serial number",
      .value = opt_matchSerialNumber,
      .field = &huf.serialNumber,
      .parser = parseString,
      .flag = &opt_matchUSBDevices,
    },

    { .name = "MAC address",
      .value = opt_matchDeviceAddress,
      .field = &hbf.deviceAddress,
      .parser = parseMACAddress,
      .flag = &opt_matchBluetoothDevices,
    },

    { .name = "device name",
      .value = opt_matchDeviceName,
      .field = &hbf.deviceName,
      .parser = parseString,
      .flag = &opt_matchBluetoothDevices,
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

        if (opt_matchUSBDevices && opt_matchBluetoothDevices) {
          logMessage(LOG_ERR, "conflicting filter options");
          return 0;
        }
      }
    }

    filter += 1;
  }

  hbf.vendorIdentifier = huf.vendorIdentifier;
  hbf.productIdentifier = huf.productIdentifier;

  if (opt_matchBluetoothDevices) {
    *device = hidOpenBluetoothDevice(&hbf);
  } else {
    *device = hidOpenUSBDevice(&huf);
  }

  return 1;
}

static int
getReportSize (HidDevice *device, unsigned char identifier, HidReportSize *size) {
  const HidItemsDescriptor *items = hidGetItems(device);
  if (!items) return 0;
  return hidGetReportSize(items, identifier, size);
}

static int
performShowDeviceIdentifiers (HidDevice *device) {
  uint16_t vendor;
  uint16_t product;

  if (!hidGetIdentifiers(device, &vendor, &product)) {
    logMessage(LOG_WARNING, "vendor/product identifiers not available");
    return 0;
  }

  fprintf(outputStream,
    "Device Identifiers: %04X:%04X\n",
    vendor, product
  );

  return 1;
}

static int
performShowDeviceAddress (HidDevice *device) {
  const char *address = hidGetDeviceAddress(device);

  if (!address) {
    logMessage(LOG_WARNING, "device address not available");
    return 0;
  }

  fprintf(outputStream, "Device Address: %s\n", address);
  return 1;
}

static int
performShowDeviceName (HidDevice *device) {
  const char *name = hidGetDeviceName(device);

  if (!name) {
    logMessage(LOG_WARNING, "device name not available");
    return 0;
  }

  fprintf(outputStream, "Device Name: %s\n", name);
  return 1;
}

static int
performShowHostPath (HidDevice *device) {
  const char *path = hidGetHostPath(device);

  if (!path) {
    logMessage(LOG_WARNING, "host path not available");
    return 0;
  }

  fprintf(outputStream, "Host Path: %s\n", path);
  return 1;
}

static int
performShowHostDevice (HidDevice *device) {
  const char *hostDevice = hidGetHostDevice(device);

  if (!hostDevice) {
    logMessage(LOG_WARNING, "host device not available");
    return 0;
  }

  fprintf(outputStream, "Host Device: %s\n", hostDevice);
  return 1;
}

static int
listItem (const char *line, void *data) {
  fprintf(outputStream, "%s\n", line);
  return canWriteOutput();
}

static int
performListItems (HidDevice *device) {
  const HidItemsDescriptor *items = hidGetItems(device);
  if (!items) return 0;

  hidListItems(items, listItem, NULL);
  return 1;
}

static int
performListReports (HidDevice *device) {
  const HidItemsDescriptor *items = hidGetItems(device);
  if (!items) return 0;

  HidReports *reports = hidGetReports(items);
  if (!reports) return 0;

  for (unsigned int index=0; index<reports->count; index+=1) {
    unsigned char identifier = reports->identifiers[index];
    HidReportSize size;

    char line[0X40];
    STR_BEGIN(line, sizeof(line));
    STR_PRINTF("Report %02X:", identifier);

    if (hidGetReportSize(items, identifier, &size)) {
      typedef struct {
        const char *label;
        const size_t value;
      } SizeEntry;

      const SizeEntry sizeTable[] = {
        { .value = size.input,
          .label = "In",
        },

        { .value = size.output,
          .label = "Out",
        },

        { .value = size.feature,
          .label = "Ftr",
        },
      };

      const SizeEntry *size = sizeTable;
      const SizeEntry *end = size + ARRAY_COUNT(sizeTable);

      while (size < end) {
        if (size->value) {
          STR_PRINTF(" %s:%"PRIsize, size->label, size->value);
        }

        size += 1;
      }
    }

    STR_END;
    fprintf(outputStream, "%s\n", line);
    if (!canWriteOutput()) return 0;
  }

  free(reports);
  return 1;
}

static int
isReportIdentifier (unsigned char *identifier, const char *string, unsigned char minimum) {
  if (strlen(string) != 2) return 0;

  char *end;
  unsigned long int value = strtoul(string, &end, 0X10);
  if (*end) return 0;

  if (value > UINT8_MAX) return 0;
  *identifier = value;
  return 1;
}

static int
isReportDefined (
  HidDevice *device, const char *what, unsigned char identifier,
  HidReportSize *reportSize, size_t *size
) {
  if (getReportSize(device, identifier, reportSize)) {
    if (*size) {
      return 1;
    }
  }

  logMessage(LOG_ERR, "%s report not defined: %02X", what, identifier);
  return 0;
}

static int
verifyRead (
  HidDevice *device, const char *what, unsigned char identifier,
  HidReportSize *reportSize, size_t *size
) {
  int isDefined = isReportDefined(
    device, what, identifier,
    reportSize, size
  );

  return isDefined;
}

static unsigned char readReportIdentifier;

static int
parseReadReport (void) {
  const char *operand = opt_readReport;
  if (!*operand) return 1;
  if (isReportIdentifier(&readReportIdentifier, operand, 0)) return 1;

  logMessage(LOG_ERR, "invalid input report identifier: %s", operand);
  return 0;
}

static int
performReadReport (HidDevice *device) {
  unsigned char identifier = readReportIdentifier;

  HidReportSize reportSize;
  size_t *size = &reportSize.input;

  int verified = verifyRead(
    device, "input", identifier,
    &reportSize, size
  );

  if (!verified) return 0;
  unsigned char report[*size];
  report[0] = identifier;

  if (!hidGetReport(device, report, *size)) return 0;
  writeBytesLine("Input Report: %02X", report, *size, identifier);
  return 1;
}

static unsigned char readFeatureIdentifier;

static int
parseReadFeature (void) {
  const char *operand = opt_readFeature;
  if (!*operand) return 1;
  if (isReportIdentifier(&readFeatureIdentifier, operand, 1)) return 1;

  logMessage(LOG_ERR, "invalid feature report identifier: %s", operand);
  return 0;
}

static int
performReadFeature (HidDevice *device) {
  unsigned char identifier = readFeatureIdentifier;

  HidReportSize reportSize;
  size_t *size = &reportSize.feature;

  int verified = verifyRead(
    device, "feature", identifier,
    &reportSize, size
  );

  if (!verified) return 0;
  unsigned char feature[*size];
  feature[0] = identifier;

  if (!hidGetFeature(device, feature, *size)) return 0;
  writeBytesLine("Feature Report: %02X", feature, *size, identifier);
  return 1;
}

static int
isHexadecimal (unsigned char *digit, char character) {
  const char string[] = {character, 0};
  char *end;
  long int value = strtol(string, &end, 0X10);

  if (*end) {
    logMessage(LOG_ERR, "invalid hexadecimal digit: %c", character);
    return 0;
  }

  *digit = value;
  return 1;
}

static int
parseBytes (
  const char *bytes, const char *what, unsigned char *buffer,
  size_t bufferSize, size_t *bufferUsed
) {
  unsigned char *out = buffer;
  const unsigned char *end = out + bufferSize;

  if (*bytes) {
    const char *in = bytes;
    unsigned char byte = 0;
    unsigned int count = 1;

    enum {NEXT, HEX, DOTS, COUNT};
    unsigned int state = NEXT;

    while (*in) {
      char character = *in++;

      switch (state) {
        case NEXT: {
          if (iswspace(character)) continue;

          if (character == '[') {
            state = DOTS;
            continue;
          }

          unsigned char digit;
          if (!isHexadecimal(&digit, character)) return 0;

          byte = digit << 4;
          state = HEX;
          continue;
        }

        case HEX: {
          unsigned char digit;
          if (!isHexadecimal(&digit, character)) return 0;

          byte |= digit;
          state = NEXT;
          break;
        }

        case DOTS: {
          if (character == ']') {
            state = NEXT;
            break;
          }

          if ((character < '1') || (character > '8')) {
            logMessage(LOG_ERR, "invalid dot number: %c", character);
            return 0;
          }
          unsigned char bit = 1 << (character - '1');

          if (byte & bit) {
            logMessage(LOG_ERR, "duplicate dot number: %c", character);
            return 0;
          }

          byte |= bit;
          continue;
        }

        case COUNT: {
          if (iswspace(character)) break;
          int digit = character - '0';

          if ((digit < 0) || (digit > 9)) {
            logMessage(LOG_ERR, "invalid count digit: %c", character);
            return 0;
          }

          if (!digit) {
            if (!count) {
              logMessage(LOG_ERR, "first digit of count can't be 0");
              return 0;
            }
          }

          count *= 10;
          count += digit;

          if (!*in) break;
          continue;
        }

        default:
          logMessage(LOG_ERR, "unexpected bytes parser state: %u", state);
          return 0;
      }

      if (state == COUNT) {
        if (!count) {
          logMessage(LOG_ERR, "missing count");
          return 0;
        }

        state = NEXT;
      } else if (*in == '*') {
        in += 1;
        state = COUNT;
        count = 0;
        continue;
      }

      while (count--) {
        if (out == end) {
          logMessage(LOG_ERR, "%s buffer too small", what);
          return 0;
        }

        *out++ = byte;
      }

      byte = 0;
      count = 1;
    }

    if (state != NEXT) {
      logMessage(LOG_ERR, "incomplete %s specification", what);
      return 0;
    }
  }

  *bufferUsed = out - buffer;
  return 1;
}

static int
verifyWrite (
  HidDevice *device, const char *what, unsigned char *identifier,
  HidReportSize *reportSize, size_t *expectedSize,
  const unsigned char *buffer, size_t actualSize
) {
  *identifier = buffer[0];

  int isDefined = isReportDefined(
    device, what, *identifier,
    reportSize, expectedSize
  );

  if (!isDefined) return 0;
  if (!*identifier) *expectedSize += 1;

  if (actualSize != *expectedSize) {
    logMessage(LOG_ERR,
      "incorrect %s report size: %02X:"
      " Expected:%"PRIsize " Actual:%"PRIsize,
      what, *identifier, *expectedSize, actualSize
    );

    return 0;
  }

  return 1;
}

static unsigned char writeReportBuffer[0X1000];
static size_t writeReportLength;

static int
parseWriteReport (void) {
  return parseBytes(
    opt_writeReport, "output report", writeReportBuffer,
    ARRAY_COUNT(writeReportBuffer), &writeReportLength
  );
}

static int
performWriteReport (HidDevice *device) {
  const unsigned char *report = writeReportBuffer;
  size_t length = writeReportLength;

  unsigned char identifier;
  HidReportSize reportSize;

  int verified = verifyWrite(
    device, "output", &identifier,
    &reportSize, &reportSize.output,
    report, length
  );

  if (!verified) return 0;
  writeBytesLine("Output Report: %02X", report, length, identifier);
  return hidSetReport(device, report, length);
}

static unsigned char writeFeatureBuffer[0X1000];
static size_t writeFeatureLength;

static int
parseWriteFeature (void) {
  return parseBytes(
    opt_writeFeature, "feature report", writeFeatureBuffer,
    ARRAY_COUNT(writeFeatureBuffer), &writeFeatureLength
  );
}

static int
performWriteFeature (HidDevice *device) {
  const unsigned char *feature = writeFeatureBuffer;
  size_t length = writeFeatureLength;

  unsigned char identifier;
  HidReportSize reportSize;

  int verified = verifyWrite(
    device, "feature", &identifier,
    &reportSize, &reportSize.feature,
    feature, length
  );

  if (!verified) return 0;
  writeBytesLine("Feature Report: %02X", feature, length, identifier);
  return hidSetFeature(device, feature, length);
}

static int inputTimeout;

static int
parseInputTimeout (void) {
  inputTimeout = 10;

  static const int minimum = 1;
  static const int maximum = 99;

  if (!validateInteger(&inputTimeout, opt_inputTimeout, &minimum, &maximum)) {
    logMessage(LOG_ERR, "invalid input timeout: %s", opt_inputTimeout);
    return 0;
  }

  inputTimeout *= 1000;
  return 1;
}

static int
performEchoInput (HidDevice *device) {
  HidReportSize reportSize;
  const size_t *inputSize = &reportSize.input;

  unsigned char reportIdentifier = 0;
  int hasReportIdentifiers = !getReportSize(device, reportIdentifier, &reportSize);

  unsigned char buffer[0X1000];
  size_t bufferSize = sizeof(buffer);

  unsigned char *from = buffer;
  unsigned char *to = from;
  const unsigned char *end = from + bufferSize;

  while (hidAwaitInput(device, inputTimeout)) {
    ssize_t result = hidReadData(device, to, (end - to), 1000, 100);

    if (result == -1) {
      logMessage(LOG_ERR, "input error: %s", strerror(errno));
      return 0;
    }

    to += result;

    while (from < to) {
      if (hasReportIdentifiers) {
        reportIdentifier = *from;

        if (!getReportSize(device, reportIdentifier, &reportSize)) {
          logMessage(LOG_ERR, "input report not defined: %02X", reportIdentifier);
          return 0;
        }
      }

      if (!*inputSize) {
        logMessage(LOG_ERR, "input report size is zero: %02X", reportIdentifier);
        return 0;
      }

      size_t count = to - from;

      if (*inputSize > count) {
        if (from == buffer) {
          logMessage(LOG_ERR,
            "input report too large: %02X: %"PRIsize " > %"PRIsize,
            reportIdentifier, *inputSize, count
          );

          return 0;
        }

        memmove(buffer, from, count);
        from = buffer;
        to = from + count;

        break;
      }

      if (!writeBytesLine("Input Report", from, *inputSize)) return 0;
      from += *inputSize;
    }
  }

  return 1;
}

static int
parseOperands (void) {
  typedef struct {
    int (*parse) (void);
  } OperandEntry;

  static const OperandEntry operandTable[] = {
    { .parse = parseReadReport,
    },

    { .parse = parseReadFeature,
    },

    { .parse = parseWriteReport,
    },

    { .parse = parseWriteFeature,
    },

    { .parse = parseInputTimeout,
    },
  };

  const OperandEntry *operand = operandTable;
  const OperandEntry *end = operand + ARRAY_COUNT(operandTable);

  while (operand < end) {
    if (!operand->parse()) return 0;
    operand += 1;
  }

  return 1;
}

static int
performActions (HidDevice *device) {
  typedef struct {
    int (*perform) (HidDevice *device);
    unsigned char isFlag:1;

    union {
      char **string;
      int *flag;
    } option;
  } ActionEntry;

  static const ActionEntry actionTable[] = {
    { .perform = performShowDeviceIdentifiers,
      .isFlag = 1,
      .option.flag = &opt_showDeviceIdentifiers,
    },

    { .perform = performShowDeviceAddress,
      .isFlag = 1,
      .option.flag = &opt_showDeviceAddress,
    },

    { .perform = performShowDeviceName,
      .isFlag = 1,
      .option.flag = &opt_showDeviceName,
    },

    { .perform = performShowHostPath,
      .isFlag = 1,
      .option.flag = &opt_showHostPath,
    },

    { .perform = performShowHostDevice,
      .isFlag = 1,
      .option.flag = &opt_showHostDevice,
    },

    { .perform = performListItems,
      .isFlag = 1,
      .option.flag = &opt_listItems,
    },

    { .perform = performListReports,
      .isFlag = 1,
      .option.flag = &opt_listReports,
    },

    { .perform = performReadReport,
      .option.string = &opt_readReport,
    },

    { .perform = performReadFeature,
      .option.string = &opt_readFeature,
    },

    { .perform = performWriteReport,
      .option.string = &opt_writeReport,
    },

    { .perform = performWriteFeature,
      .option.string = &opt_writeFeature,
    },

    { .perform = performEchoInput,
      .isFlag = 1,
      .option.flag = &opt_echoInput,
    },
  };

  const ActionEntry *action = actionTable;
  const ActionEntry *end = action + ARRAY_COUNT(actionTable);

  while (action < end) {
    int perform = 0;

    if (action->isFlag) {
      if (*action->option.flag) perform = 1;
    } else {
      if (**action->option.string) perform = 1;
    }

    if (perform) {
      if (!action->perform(device)) return 0;
      if (!canWriteOutput()) return 0;
    }

    action += 1;
  }

  return 1;
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "brltty-hid",
    };

    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  outputStream = stdout;
  outputError = 0;

  if (argc) {
    logMessage(LOG_ERR, "too many parameters");
    return PROG_EXIT_SYNTAX;
  }

  if (!parseOperands()) return PROG_EXIT_SYNTAX;
  ProgramExitStatus exitStatus = PROG_EXIT_SUCCESS;
  HidDevice *device = NULL;

  if (!openDevice(&device)) {
    exitStatus = PROG_EXIT_SYNTAX;
  } else if (!device) {
    logMessage(LOG_ERR, "device not found");
    exitStatus = PROG_EXIT_SEMANTIC;
  } else {
    if (!performActions(device)) exitStatus = PROG_EXIT_FATAL;
    hidCloseDevice(device);
  }

  if (outputError) {
    logMessage(LOG_ERR, "output error: %s", strerror(outputError));
    exitStatus = PROG_EXIT_FATAL;
  }

  return exitStatus;
}
