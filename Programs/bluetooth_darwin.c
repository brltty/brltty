/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#import <IOBluetooth/objc/IOBluetoothDevice.h>
#import <IOBluetooth/objc/IOBluetoothSDPUUID.h>
#import <IOBluetooth/objc/IOBluetoothSDPServiceRecord.h>
#import <IOBluetooth/objc/IOBluetoothRFCOMMChannel.h>

#include "log.h"
#include "io_misc.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "system_darwin.h"

@interface ServiceQueryResult: AsynchronousResult
- (void) sdpQueryComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status;
@end

@interface BluetoothConnectionDelegate: AsynchronousTask
@property (assign) BluetoothConnectionExtension *bluetoothConnectionExtension;
@end

@interface RfcommChannelDelegate: BluetoothConnectionDelegate
- (void) rfcommChannelData
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  data: (void *) dataPointer
  length: (size_t) dataLength;

- (IOReturn) run;
@end

struct BluetoothConnectionExtensionStruct {
  BluetoothDeviceAddress bluetoothAddress;
  IOBluetoothDevice *bluetoothDevice;

  IOBluetoothRFCOMMChannel *rfcommChannel;
  RfcommChannelDelegate *rfcommDelegate;

  int inputPipe[2];
};

static void
bthSetError (IOReturn result, const char *action) {
  setDarwinSystemError(result);
  logSystemError(action);
}

static void
bthInitializeRfcommChannel (BluetoothConnectionExtension *bcx) {
  bcx->rfcommChannel = nil;
}

static void
bthDestroyRfcommChannel (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommChannel) {
    [bcx->rfcommChannel closeChannel];
    [bcx->rfcommChannel release];
    bthInitializeRfcommChannel(bcx);
  }
}

static void
bthInitializeRfcommDelegate (BluetoothConnectionExtension *bcx) {
  bcx->rfcommDelegate = nil;
}

static void
bthDestroyRfcommDelegate (BluetoothConnectionExtension *bcx) {
  if (bcx->rfcommDelegate) {
    [bcx->rfcommDelegate stop];
    [bcx->rfcommDelegate wait:5];
    [bcx->rfcommDelegate release];
    bthInitializeRfcommDelegate(bcx);
  }
}

static void
bthInitializeBluetoothDevice (BluetoothConnectionExtension *bcx) {
  bcx->bluetoothDevice = nil;
}

static void
bthDestroyBluetoothDevice (BluetoothConnectionExtension *bcx) {
  if (bcx->bluetoothDevice) {
    [bcx->bluetoothDevice closeConnection];
    [bcx->bluetoothDevice release];
    bthInitializeBluetoothDevice(bcx);
  }
}

static void
bthInitializeInputPipe (BluetoothConnectionExtension *bcx) {
  bcx->inputPipe[0] = bcx->inputPipe[1] = INVALID_FILE_DESCRIPTOR;
}

static void
bthDestroyInputPipe (BluetoothConnectionExtension *bcx) {
  int *fileDescriptor = bcx->inputPipe;
  const int *end = fileDescriptor + ARRAY_COUNT(bcx->inputPipe);

  while (fileDescriptor < end) {
    closeFile(fileDescriptor);
    fileDescriptor += 1;
  }
}

static void
bthMakeAddress (BluetoothDeviceAddress *address, uint64_t bda) {
  unsigned int index = sizeof(address->data);

  while (index > 0) {
    address->data[--index] = bda & 0XFF;
    bda >>= 8;
  }
}

static int
bthPerformServiceQuery (BluetoothConnectionExtension *bcx) {
  int ok = 0;
  IOReturn result;
  ServiceQueryResult *target = [ServiceQueryResult new];

  if (target) {
    if ((result = [bcx->bluetoothDevice performSDPQuery:target]) == kIOReturnSuccess) {
      if ([target wait:10]) {
        if ((result = target.finalStatus) == kIOReturnSuccess) {
          ok = 1;
        } else {
          bthSetError(result, "service discovery response");
        }
      }
    } else {
      bthSetError(result, "service discovery request");
    }

    [target release];
  }

  return ok;
}

static int
bthQueryChannel (
  uint8_t *channel, BluetoothConnectionExtension *bcx,
  const void *uuidBytes, size_t uuidLength
) {
  IOReturn result;

  if (bthPerformServiceQuery(bcx)) {
    IOBluetoothSDPUUID *uuid = [IOBluetoothSDPUUID uuidWithBytes:uuidBytes length:uuidLength];

    if (uuid) {
      IOBluetoothSDPServiceRecord *record = [bcx->bluetoothDevice getServiceRecordForUUID:uuid];

      if (record) {
        if ((result = [record getRFCOMMChannelID:channel]) == kIOReturnSuccess) {
          return 1;
        } else {
          bthSetError(result, "RFCOMM channel lookup");
        }
      }
    }
  }

  return 0;
}

static int
bthQuerySerialPortChannel (uint8_t *channel, BluetoothConnectionExtension *bcx) {
  return bthQueryChannel(channel, bcx, uuidBytes_serialPortProfile, uuidLength_serialPortProfile);
}

BluetoothConnectionExtension *
bthConnect (uint64_t bda, uint8_t channel, int timeout) {
  IOReturn result;
  BluetoothConnectionExtension *bcx;

  if ((bcx = malloc(sizeof(*bcx)))) {
    memset(bcx, 0, sizeof(*bcx));
    bthInitializeInputPipe(bcx);
    bthMakeAddress(&bcx->bluetoothAddress, bda);

    if (pipe(bcx->inputPipe) != -1) {
      if (setBlockingIo(bcx->inputPipe[0], 0)) {
        if ((bcx->bluetoothDevice = [IOBluetoothDevice deviceWithAddress:&bcx->bluetoothAddress])) {
          [bcx->bluetoothDevice retain];

          if ((bcx->rfcommDelegate = [RfcommChannelDelegate new])) {
            bcx->rfcommDelegate.bluetoothConnectionExtension = bcx;

            bthQuerySerialPortChannel(&channel, bcx);
            bthLogChannel(channel);

            if ((result = [bcx->bluetoothDevice openRFCOMMChannelSync:&bcx->rfcommChannel withChannelID:channel delegate:nil]) == kIOReturnSuccess) {
              if ([bcx->rfcommDelegate start]) {
                return bcx;
              }

              bthDestroyRfcommChannel(bcx);
            } else {
              bthSetError(result, "RFCOMM channel open");
            }

            bthDestroyRfcommDelegate(bcx);
          }

          bthDestroyBluetoothDevice(bcx);
        }
      }

      bthDestroyInputPipe(bcx);
    } else {
      logSystemError("pipe");
    }

    free(bcx);
  } else {
    logMallocError();
  }

  return NULL;
}

void
bthDisconnect (BluetoothConnectionExtension *bcx) {
  bthDestroyRfcommChannel(bcx);
  bthDestroyRfcommDelegate(bcx);
  bthDestroyBluetoothDevice(bcx);
  bthDestroyInputPipe(bcx);
  free(bcx);
}

int
bthAwaitInput (BluetoothConnection *connection, int milliseconds) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return awaitFileInput(bcx->inputPipe[0], milliseconds);
}

ssize_t
bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return readFile(bcx->inputPipe[0], buffer, size, initialTimeout, subsequentTimeout);
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  BluetoothConnectionExtension *bcx = connection->extension;
  IOReturn result = [bcx->rfcommChannel writeSync:(void *)buffer length:size];

  if (result == kIOReturnSuccess) return size;
  bthSetError(result, "RFCOMM channel write");
  return -1;
}

char *
bthObtainDeviceName (uint64_t bda, int timeout) {
  IOReturn result;
  BluetoothDeviceAddress address;

  bthMakeAddress(&address, bda);

  {
    IOBluetoothDevice *device = [IOBluetoothDevice deviceWithAddress:&address];

    if (device != nil) {
      if ((result = [device remoteNameRequest:nil]) == kIOReturnSuccess) {
        NSString *nsName = device.name;

        if (nsName != nil) {
          const char *utf8Name = [nsName UTF8String];

          if (utf8Name != NULL) {
            char *name = strdup(utf8Name);

            if (name != NULL) {
              return name;
            }
          }
        }
      } else {
        bthSetError(result, "device name query");
      }

      [device closeConnection];
    }
  }

  return NULL;
}

@implementation ServiceQueryResult
- (void) sdpQueryComplete
  : (IOBluetoothDevice *) device
  status: (IOReturn) status
  {
    [self setStatus:status];
  }
@end

@implementation BluetoothConnectionDelegate
@synthesize bluetoothConnectionExtension;
@end

@implementation RfcommChannelDelegate
- (void) rfcommChannelData
  : (IOBluetoothRFCOMMChannel *) rfcommChannel
  data: (void *) dataPointer
  length: (size_t) dataLength
  {
    writeFile(self.bluetoothConnectionExtension->inputPipe[1], dataPointer, dataLength);
  }

- (IOReturn) run
  {
    IOReturn result;
    logMessage(LOG_DEBUG, "RFCOMM channel delegate started");

    {
      BluetoothConnectionExtension *bcx = self.bluetoothConnectionExtension;

      if ((result = [bcx->rfcommChannel setDelegate:self]) == kIOReturnSuccess) {
        CFRunLoopRun();
        result = kIOReturnSuccess;
      } else {
        bthSetError(result, "RFCOMM channel delegate set");
      }
    }

    logMessage(LOG_DEBUG, "RFCOMM channel delegate finished");
    return result;
  }
@end
