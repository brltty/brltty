/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#include "misc.h"
#include "driver.h"

static int logOutputPackets = 0;
static int logInputPackets = 0;

static void
logPacket (const char *description, const void *packet, size_t size) {
  LogBytes(LOG_WARNING, description, packet, size);
}

void
logOutputPacket (const void *packet, size_t size) {
  if (logOutputPackets) logPacket("Output Packet", packet, size);
}

void
logInputPacket (const void *packet, size_t size) {
  if (logInputPackets) logPacket("Input Packet", packet, size);
}

void
logInputProblem (const char *problem, const unsigned char *bytes, size_t count) {
  LogBytes(LOG_WARNING, problem, bytes, count);
}

void
logIgnoredByte (unsigned char byte) {
  logInputProblem("Ignored Byte", &byte, 1);
}

void
logDiscardedByte (unsigned char byte) {
  logInputProblem("Discarded Byte", &byte, 1);
}

void
logUnknownPacket (unsigned char byte) {
  logInputProblem("Unknown Packet", &byte, 1);
}

void
logPartialPacket (const void *packet, size_t size) {
  logInputProblem("Partial Packet", packet, size);
}

void
logTruncatedPacket (const void *packet, size_t size) {
  logInputProblem("Truncated Packet", packet, size);
}

void
logShortPacket (const void *packet, size_t size) {
  logInputProblem("Short Packet", packet, size);
}

void
logUnexpectedPacket (const void *packet, size_t size) {
  logInputProblem("Unexpected Packet", packet, size);
}

void
logDiscardedBytes (const unsigned char *bytes, size_t count) {
  logInputProblem("Discarded Bytes", bytes, count);
}
