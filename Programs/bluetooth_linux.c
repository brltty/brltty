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

#include <errno.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>

#include "log.h"
#include "io_bluetooth.h"
#include "bluetooth_internal.h"
#include "io_misc.h"

#define ASYNCHRONOUS_NAME_QUERY
#define ASYNCHRONOUS_CHANNEL_DISCOVERY

struct BluetoothConnectionExtensionStruct {
  SocketDescriptor socket;
  struct sockaddr_rc local;
  struct sockaddr_rc remote;
};

static void
bthMakeAddress (bdaddr_t *address, uint64_t bda) {
  unsigned int index;

  for (index=0; index<BDA_SIZE; index+=1) {
    address->b[index] = bda & 0XFF;
    bda >>= 8;
  }
}

BluetoothConnectionExtension *
bthNewConnectionExtension (uint64_t bda) {
  BluetoothConnectionExtension *bcx;

  if ((bcx = malloc(sizeof(*bcx)))) {
    memset(bcx, 0, sizeof(*bcx));

    bcx->local.rc_family = AF_BLUETOOTH;
    bcx->local.rc_channel = 0;
    bacpy(&bcx->local.rc_bdaddr, BDADDR_ANY); /* Any HCI. No support for explicit
                                               * interface specification yet.
                                                 */

    bcx->remote.rc_family = AF_BLUETOOTH;
    bcx->remote.rc_channel = 0;
    bthMakeAddress(&bcx->remote.rc_bdaddr, bda);

    bcx->socket = INVALID_SOCKET_DESCRIPTOR;
    return bcx;
  } else {
    logMallocError();
  }

  return NULL;
}

void
bthReleaseConnectionExtension (BluetoothConnectionExtension *bcx) {
  closeSocket(&bcx->socket);
  free(bcx);
}

int
bthOpenChannel (BluetoothConnectionExtension *bcx, uint8_t channel, int timeout) {
  bcx->remote.rc_channel = channel;

  if ((bcx->socket = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) != -1) {
    if (bind(bcx->socket, (struct sockaddr *)&bcx->local, sizeof(bcx->local)) != -1) {
      if (setBlockingIo(bcx->socket, 0)) {
        if (connectSocket(bcx->socket, (struct sockaddr *)&bcx->remote, sizeof(bcx->remote), timeout) != -1) {
          return 1;
        } else if ((errno != EHOSTDOWN) && (errno != EHOSTUNREACH)) {
          logSystemError("RFCOMM connect");
        } else {
          logMessage(LOG_DEBUG, "Bluetooth connect error: %s", strerror(errno));
        }
      }
    } else {
      logSystemError("RFCOMM bind");
    }

    close(bcx->socket);
    bcx->socket = INVALID_SOCKET_DESCRIPTOR;
  } else {
    logSystemError("RFCOMM socket");
  }

  return 0;
}

static int
bthFindChannel (uint8_t *channel, sdp_record_t *record) {
  int foundChannel = 0;
  int stopSearching = 0;
  sdp_list_t *protocolsList;

  if (!(sdp_get_access_protos(record, &protocolsList))) {
    sdp_list_t *protocolsElement = protocolsList;

    while (protocolsElement) {
      sdp_list_t *protocolList = (sdp_list_t *)protocolsElement->data;
      sdp_list_t *protocolElement = protocolList;

      while (protocolElement) {
        sdp_data_t *dataList = (sdp_data_t *)protocolElement->data;
        sdp_data_t *dataElement = dataList;
        int uuidProtocol = 0;

        while (dataElement) {
          if (SDP_IS_UUID(dataElement->dtd)) {
            uuidProtocol = sdp_uuid_to_proto(&dataElement->val.uuid);
          } else if (dataElement->dtd == SDP_UINT8) {
            if (uuidProtocol == RFCOMM_UUID) {
              *channel = dataElement->val.uint8;
              foundChannel = 1;
              stopSearching = 1;
            }
          }

          if (stopSearching) break;
          dataElement = dataElement->next;
        }

        if (stopSearching) break;
        protocolElement = protocolElement->next;
      }

      sdp_list_free(protocolList, NULL);
      if (stopSearching) break;
      protocolsElement = protocolsElement->next;
    }

    sdp_list_free(protocolsList, NULL);
  } else {
    logSystemError("sdp_get_access_protos");
  }

  return foundChannel;
}

#ifdef ASYNCHRONOUS_CHANNEL_DISCOVERY
static SocketDescriptor
bthNewL2capConnection (const bdaddr_t *address) {
  SocketDescriptor socketDescriptor = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

  if (socketDescriptor != -1) {
    if (setBlockingIo(socketDescriptor, 0)) {
      struct sockaddr_l2 socketAddress = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = *address,
        .l2_psm = htobs(SDP_PSM)
      };

      if (connectSocket(socketDescriptor, (struct sockaddr *)&socketAddress, sizeof(socketAddress), 5000) != -1) {
        return socketDescriptor;
      } else {
        logSystemError("L2CAP connect");
      }
    }

    close(socketDescriptor);
  } else {
    logSystemError("L2CAP socket");
  }

  return INVALID_SOCKET_DESCRIPTOR;
}

typedef struct {
  sdp_session_t *session;
  int *found;
  uint8_t *channel;
} BluetoothDiscoverChannelData;

static void
bthDiscoverChannelCallback (
  uint8_t type, uint16_t status,
  uint8_t *response, size_t size,
  void *data
) {
  BluetoothDiscoverChannelData *dcd = data;

  switch (status) {
    case 0:
      switch (type) {
        case SDP_SVC_SEARCH_ATTR_RSP: {
          uint8_t *nextByte = response;
          int bytesLeft = size;

          uint8_t dtd = 0;
          int dataLeft = 0;
          int headerLength = sdp_extract_seqtype(nextByte, bytesLeft, &dtd, &dataLeft);

          if (headerLength > 0) {
            nextByte += headerLength;
            bytesLeft -= headerLength;

            while (dataLeft > 0) {
              int stop = 0;
              int recordLength = 0;
              sdp_record_t *record = sdp_extract_pdu(nextByte, bytesLeft, &recordLength);

              if (record) {
                if (bthFindChannel(dcd->channel, record)) {
                  *dcd->found = 1;
                  stop = 1;
                }

                nextByte += recordLength;
                bytesLeft -= recordLength;
                dataLeft -= recordLength;

                sdp_record_free(record);
              } else {
                logSystemError("sdp_extract_pdu");
                stop = 1;
              }

              if (stop) break;
            }
          }

          break;
        }

        default:
          logMessage(LOG_ERR, "unhandled discover channel response type: %u", type);
          break;
      }
      break;

    case 0XFFFF:
      errno = sdp_get_error(dcd->session);
      if (errno < 0) errno = EINVAL;
      logSystemError("discover channel response");
      break;

    default:
      logMessage(LOG_ERR, "unhandled discover channel response status: %u", status);
      break;
  }
}
#endif /* ASYNCHRONOUS_CHANNEL_DISCOVERY */

int
bthDiscoverChannel (
  uint8_t *channel, BluetoothConnectionExtension *bcx,
  const void *uuidBytes, size_t uuidLength
) {
  int foundChannel = 0;

  uuid_t uuid;
  sdp_list_t *searchList;

  sdp_uuid128_create(&uuid, uuidBytes);
  searchList = sdp_list_append(NULL, &uuid);

  if (searchList) {
    uint32_t attributesRange = 0X0000FFFF;
    sdp_list_t *attributesList = sdp_list_append(NULL, &attributesRange);

    if (attributesList) {
#ifdef ASYNCHRONOUS_CHANNEL_DISCOVERY
      SocketDescriptor l2capSocket = bthNewL2capConnection(&bcx->remote.rc_bdaddr);

      if (l2capSocket != INVALID_SOCKET_DESCRIPTOR) {
        sdp_session_t *session = sdp_create(l2capSocket, 0);

        if (session) {
          BluetoothDiscoverChannelData dcd = {
            .session = session,
            .found = &foundChannel,
            .channel = channel
          };

          if (sdp_set_notify(session, bthDiscoverChannelCallback, &dcd) != -1) {
            int queryStatus = sdp_service_search_attr_async(session, searchList,
                                                            SDP_ATTR_REQ_RANGE, attributesList);

            if (!queryStatus) {
              while (awaitSocketInput(l2capSocket, 5000)) {
                if (sdp_process(session) == -1) {
                  break;
                }
              }
            } else {
              logSystemError("sdp_service_search_attr_async");
            }
          } else {
            logSystemError("sdp_set_notify");
          }

          sdp_close(session);
        } else {
          logSystemError("sdp_create");
        }

        close(l2capSocket);
      }
#else /* ASYNCHRONOUS_CHANNEL_DISCOVERY */
      sdp_session_t *session = sdp_connect(BDADDR_ANY, &bcx->remote.rc_bdaddr, SDP_RETRY_IF_BUSY);

      if (session) {
        sdp_list_t *recordList = NULL;
        int queryStatus = sdp_service_search_attr_req(session, searchList,
                                                      SDP_ATTR_REQ_RANGE, attributesList,
                                                      &recordList);

        if (!queryStatus) {
          int stopSearching = 0;
          sdp_list_t *recordElement = recordList;

          while (recordElement) {
            sdp_record_t *record = (sdp_record_t *)recordElement->data;

            if (record) {
              if (bthFindChannel(channel, record)) {
                foundChannel = 1;
                stopSearching = 1;
              }

              sdp_record_free(record);
            } else {
              logMallocError();
              stopSearching = 1;
            }

            if (stopSearching) break;
            recordElement = recordElement->next;
          }

          sdp_list_free(recordList, NULL);
        } else {
          logSystemError("sdp_service_search_attr_req");
        }

        sdp_close(session);
      } else {
        logSystemError("sdp_connect");
      }
#endif /* ASYNCHRONOUS_CHANNEL_DISCOVERY */

      sdp_list_free(attributesList, NULL);
    } else {
      logMallocError();
    }

    sdp_list_free(searchList, NULL);
  } else {
    logMallocError();
  }

  return foundChannel;
}

int
bthAwaitInput (BluetoothConnection *connection, int milliseconds) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return awaitSocketInput(bcx->socket, milliseconds);
}

ssize_t
bthReadData (
  BluetoothConnection *connection, void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return readSocket(bcx->socket, buffer, size, initialTimeout, subsequentTimeout);
}

ssize_t
bthWriteData (BluetoothConnection *connection, const void *buffer, size_t size) {
  BluetoothConnectionExtension *bcx = connection->extension;

  return writeSocket(bcx->socket, buffer, size);
}

char *
bthObtainDeviceName (uint64_t bda, int timeout) {
  char *name = NULL;
  int device = hci_get_route(NULL);

  if (device >= 0) {
    SocketDescriptor socketDescriptor = hci_open_dev(device);

    if (socketDescriptor >= 0) {
      int obtained = 0;
      bdaddr_t address;
      char buffer[HCI_MAX_NAME_LENGTH];

      bthMakeAddress(&address, bda);
      memset(buffer, 0, sizeof(buffer));

#ifdef ASYNCHRONOUS_NAME_QUERY
      if (setBlockingIo(socketDescriptor, 0)) {
        struct hci_filter oldFilter;
        socklen_t oldLength = sizeof(oldFilter);

        if (getsockopt(socketDescriptor, SOL_HCI, HCI_FILTER, &oldFilter, &oldLength) != -1) {
          struct hci_filter newFilter;

          hci_filter_clear(&newFilter);
          hci_filter_set_ptype(HCI_EVENT_PKT, &newFilter);
          hci_filter_set_event(EVT_CMD_STATUS, &newFilter);
          hci_filter_set_event(EVT_CMD_COMPLETE, &newFilter);
          hci_filter_set_event(EVT_REMOTE_NAME_REQ_COMPLETE, &newFilter);

          if (setsockopt(socketDescriptor, SOL_HCI, HCI_FILTER, &newFilter, sizeof(newFilter)) != -1) {
            remote_name_req_cp parameters;

            memset(&parameters, 0, sizeof(parameters));
            bacpy(&parameters.bdaddr, &address);
            parameters.pscan_rep_mode = 0X02;
            parameters.clock_offset = 0;

            if (hci_send_cmd(socketDescriptor, OGF_LINK_CTL, OCF_REMOTE_NAME_REQ, sizeof(parameters), &parameters) != -1) {
              while (awaitSocketInput(socketDescriptor, timeout)) {
                typedef union {
                  unsigned char event[HCI_MAX_EVENT_SIZE];

                  struct {
                    unsigned char type;

                    union {
                      struct {
                        hci_event_hdr header;

                        union {
                          evt_remote_name_req_complete rn;
                        } data;
                      } PACKED event;
                    } data;
                  } PACKED fields;
                } HciPacket;

                HciPacket packet;
                int result = read(socketDescriptor, &packet, sizeof(packet));

                if (result == -1) {
                  if (errno == EAGAIN) continue;
                  if (errno == EINTR) continue;

                  logSystemError("read");
                  break;
                }

                switch (packet.fields.type) {
                  case HCI_EVENT_PKT: {
                    hci_event_hdr *header = &packet.fields.data.event.header;
                    void *data = &packet.fields.data.event.data;

                    switch (header->evt) {
                      case EVT_REMOTE_NAME_REQ_COMPLETE: {
                        evt_remote_name_req_complete *rn = data;

                        if (!rn->status) {
                          if (bacmp(&rn->bdaddr, &address) == 0) {
                            size_t length = header->plen;

                            length -= rn->name - (unsigned char *)rn;
                            length = MIN(length, sizeof(rn->name));
                            length = MIN(length, sizeof(buffer)-1);

                            memcpy(buffer, rn->name, length);
                            buffer[length] = 0;
                            obtained = 1;
                            break;
                          }
                        }

                        break;
                      }

                      default:
                        break;
                    }

                    break;
                  }

                  default:
                    break;
                }

                if (obtained) break;
              }
            } else {
              logSystemError("hci_send_cmd");
            }

            if (setsockopt(socketDescriptor, SOL_HCI, HCI_FILTER, &oldFilter, oldLength) == -1) {
              logSystemError("setsockopt[SOL_HCI,HCI_FILTER]");
            }
          } else {
            logSystemError("setsockopt[SOL_HCI,HCI_FILTER]");
          }
        } else {
          logSystemError("getsockopt[SOL_HCI,HCI_FILTER]");
        }
      }
#else /* ASYNCHRONOUS_NAME_QUERY */
      {
        int result = hci_read_remote_name(socketDescriptor, &address, sizeof(buffer), buffer, timeout);

        if (result >= 0) {
          obtained = 1;
        } else {
          logSystemError("hci_read_remote_name");
        }
      }
#endif /* ASYNCHRONOUS_NAME_QUERY */

      if (obtained) {
        if (!(name = strdup(buffer))) {
          logMallocError();
        }
      }

      close(socketDescriptor);
    } else {
      logSystemError("hci_open_dev");
    }
  } else {
    logSystemError("hci_get_route");
  }

  return name;
}
