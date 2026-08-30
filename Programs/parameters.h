/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2026 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_PARAMETERS
#define BRLTTY_INCLUDED_PARAMETERS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PROGRAM_TERMINATION_REQUEST_COUNT_THRESHOLD 3
#define PROGRAM_TERMINATION_REQUEST_RESET_SECONDS 5

#define DEFAULT_ACTIVITY_START_TIMEOUT 1000
#define DEFAULT_ACTIVITY_STOP_TIMEOUT 1000

#define BRAILLE_DRIVER_START_RETRY_INTERVAL 5000
#define BRAILLE_DRIVER_INPUT_POLL_INTERVAL 40

#define BRAILLE_MESSAGE_ACKNOWLEDGEMENT_TIMEOUT 1000
#define BRAILLE_MESSAGE_UNACKNOWLEDGEED_LIMIT 5

#define SPEECH_DRIVER_START_RETRY_INTERVAL 5000
#define SPEECH_DRIVER_START_AUTOSPEAK_DELAY 4000

#define SPEECH_DRIVER_THREAD_START_TIMEOUT 15000
#define SPEECH_DRIVER_THREAD_STOP_TIMEOUT 5000

#define SPEECH_RESPONSE_WAIT_TIMEOUT 5000

#define SCREEN_DRIVER_START_RETRY_INTERVAL 5000
#define SCREEN_FREEZE_REMINDER_INTERVAL 30000
#define SCREEN_UPDATE_POLL_INTERVAL 40
#define SCREEN_UPDATE_SCHEDULE_DELAY 5

#define KEYBOARD_MONITOR_START_RETRY_INTERVAL 5000

#define PID_FILE_CREATE_RETRY_INTERVAL 5000

#define PTY_MASTER_READ_SIZE 0X1000
#define PTY_OUTER_TERMINAL_QUEUE_LIMIT 0X100000
#define PTY_OUTER_TERMINAL_WRITE_CHUNK_SIZE 0X1000
#define PTY_OUTER_TERMINAL_DRAIN_TIMEOUT 2000

#define UPDATE_SCHEDULE_DELAY 15

#define ROUTING_PROCESS_NICENESS 10
#define ROUTING_POLL_INTERVAL 1
#define ROUTING_MAXIMUM_TIMEOUT 2000

#define TUNE_DEVICE_CLOSE_DELAY 2000
#define TUNE_TOGGLE_REPEAT_DELAY 100

#define MESSAGE_HOLD_TIMEOUT 4000

#define LEARN_MODE_TIMEOUT 10000

#define INPUT_STICKY_MODIFIERS_TIMEOUT 5000

#define MOUNT_TABLE_UPDATE_RETRY_INTERVAL 5000

#define GPM_CONNECTION_RESET_DELAY 5000

#define GIO_USB_INPUT_MONITOR_DISABLE 0

#define SERIAL_DEVICE_RESTART_DELAY 500

#define USB_INPUT_AWAIT_RETRY_INTERVAL_MINIMUM 10
#define USB_INPUT_READ_INITIAL_TIMEOUT_DEFAULT 20
#define USB_INPUT_INTERRUPT_DELAY_MAXIMUM 16
#define USB_INPUT_INTERRUPT_REQUESTS_MAXIMUM 8

#define BLUETOOTH_DEVICE_NAME_OBTAIN_TIMEOUT 5000
#define BLUETOOTH_CHANNEL_BUSY_RETRY_TIMEOUT 2000
#define BLUETOOTH_CHANNEL_BUSY_RETRY_INTERVAL 100
#define BLUETOOTH_CHANNEL_CONNECT_TIMEOUT 15000

/* How often bthStartDarwinRunLoopPump() (bluetooth.c) drains this process's
 * CFRunLoop for the lifetime of a Bluetooth connection, so IOBluetooth can
 * deliver incoming RFCOMM data (see bluetooth_darwin.c's
 * BluetoothConnectionExtensionStruct.runLoopPumpAlarm). Confirmed live at
 * 20ms: pins a full CPU core at 100%, including idle stretches with no
 * Bluetooth activity, for as long as the connection stays open. 100ms is
 * still responsive for interactive key presses. */
#define DARWIN_BLUETOOTH_RUN_LOOP_PUMP_INTERVAL 100

/* How long each async completion wait in bluetooth_darwin.c
 * (bthEnsureConnectionOpen(), bthPerformServiceQuery(), bthOpenChannel())
 * waits for one attempt before giving up and letting the caller retry.
 * Much shorter than BLUETOOTH_CHANNEL_CONNECT_TIMEOUT: a real failure at
 * these steps is reported by macOS's Bluetooth stack within a couple of
 * seconds (confirmed live), but the completion callback for it is not
 * reliably delivered to this process (see bthPerformServiceQuery()'s
 * comment) - so a longer wait here just burns time waiting for a callback
 * that will not arrive. Confirmed live: without this, a single retry cycle
 * hitting all three steps took close to a minute. */
#define DARWIN_BLUETOOTH_ASYNC_STEP_TIMEOUT 3000

/* How often AsynchronousResult's wait: (system_darwin.c) polls its
 * condition while looping asyncAwaitCondition() in small slices, rather
 * than one call for the full timeout. A single call would leave the
 * CFRunLoop drained only as often as BRLTTY's core event loop happens to
 * have some other alarm due (see wait:'s comment) - confirmed live, a name
 * lookup with no Bluetooth connection yet open (so no pump alarm running)
 * had no other alarm to piggyback on and only drained twice in 5 seconds. */
#define DARWIN_WAIT_POLL_INTERVAL 100

/* darwinDrainRunLoop() (system_darwin.c) only continues looping while
 * CFRunLoopRunInMode() itself reports it just handled a source, so this is
 * not expected to trip - but it isn't this code's place to assume nothing
 * on a run loop it doesn't control the contents of could ever report
 * handled sources back to back indefinitely. If it does trip, that's worth
 * knowing rather than silently under-draining. */
#define DARWIN_DRAIN_RUN_LOOP_ITERATION_LIMIT 1000

#define LINUX_INPUT_DEVICE_OPEN_DELAY 1000
#define LINUX_USB_INPUT_PIPE_DISABLE 0
#define LINUX_USB_INPUT_USE_SIGNAL_MONITOR 0
#define LINUX_USB_INPUT_TREAT_INTERRUPT_AS_BULK 0
#define LINUX_BLUETOOTH_NAME_OBTAIN_ASYNCHRONOUS 1
#define LINUX_BLUETOOTH_CHANNEL_DISCOVER_ASYNCHRONOUS 1
#define LINUX_BLUETOOTH_CHANNEL_CONNECT_ASYNCHRONOUS 1

#define WINDOWS_FILE_LOCK_RETRY_INTERVAL 1000

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PARAMETERS */
