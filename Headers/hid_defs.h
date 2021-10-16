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

#ifndef BRLTTY_INCLUDED_HID_DEFS
#define BRLTTY_INCLUDED_HID_DEFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  HID_ITM_UsagePage         = 0X04,
  HID_ITM_Usage             = 0X08,
  HID_ITM_LogicalMinimum    = 0X14,
  HID_ITM_UsageMinimum      = 0X18,
  HID_ITM_LogicalMaximum    = 0X24,
  HID_ITM_UsageMaximum      = 0X28,
  HID_ITM_PhysicalMinimum   = 0X34,
  HID_ITM_DesignatorIndex   = 0X38,
  HID_ITM_PhysicalMaximum   = 0X44,
  HID_ITM_DesignatorMinimum = 0X48,
  HID_ITM_UnitExponent      = 0X54,
  HID_ITM_DesignatorMaximum = 0X58,
  HID_ITM_Unit              = 0X64,
  HID_ITM_ReportSize        = 0X74,
  HID_ITM_StringIndex       = 0X78,
  HID_ITM_Input             = 0X80,
  HID_ITM_ReportID          = 0X84,
  HID_ITM_StringMinimum     = 0X88,
  HID_ITM_Output            = 0X90,
  HID_ITM_ReportCount       = 0X94,
  HID_ITM_StringMaximum     = 0X98,
  HID_ITM_Collection        = 0XA0,
  HID_ITM_Push              = 0XA4,
  HID_ITM_Delimiter         = 0XA8,
  HID_ITM_Feature           = 0XB0,
  HID_ITM_Pop               = 0XB4,
  HID_ITM_EndCollection     = 0XC0,

  HID_ITEM_TYPE_MASK        = 0XFC
} HidItemType;

static inline int
hidHasSignedValue (HidItemType type) {
  switch (type) {
    case HID_ITM_LogicalMinimum:
    case HID_ITM_LogicalMaximum:
    case HID_ITM_PhysicalMinimum:
    case HID_ITM_PhysicalMaximum:
    case HID_ITM_UnitExponent:
      return 1;

    default:
      return 0;
  }
}

#define HID_ITEM_TYPE(item) ((item) & HID_ITEM_TYPE_MASK)
#define HID_ITEM_LENGTH(item) ((item) & ~HID_ITEM_TYPE_MASK)
#define HID_ITEM_BIT(type) (UINT64_C(1) << ((type) >> 2))

typedef enum {
  HID_COL_Physical    = 0X00,
  HID_COL_Application = 0X01,
  HID_COL_Logical     = 0X02,
} HidCollectionType;

typedef enum {
  HID_UPG_GenericDesktop          = 0X01,
  HID_UPG_Simulation              = 0X02,
  HID_UPG_VirtualReality          = 0X03,
  HID_UPG_Sport                   = 0X04,
  HID_UPG_Game                    = 0X05,
  HID_UPG_GenericDevice           = 0X06,
  HID_UPG_KeyboardKeypad          = 0X07,
  HID_UPG_LEDs                    = 0X08,
  HID_UPG_Button                  = 0X09,
  HID_UPG_Ordinal                 = 0X0A,
  HID_UPG_Telephony               = 0X0B,
  HID_UPG_Consumer                = 0X0C,
  HID_UPG_Digitizer               = 0X0D,
  HID_UPG_PhysicalInterfaceDevice = 0X0F,
  HID_UPG_Unicode                 = 0X10,
  HID_UPG_AlphanumericDisplay     = 0X14,
  HID_UPG_MedicalInstruments      = 0X40,
  HID_UPG_Braille                 = 0X41,
  HID_UPG_BarCodeScanner          = 0X8C,
  HID_UPG_Scale                   = 0X8D,
  HID_UPG_MagneticStripeReader    = 0X8E,
  HID_UPG_Camera                  = 0X90,
  HID_UPG_Arcade                  = 0X91,
} HidUsagePage;

typedef enum {
  HID_USG_GDT_Pointer                   = 0X01,
  HID_USG_GDT_Mouse                     = 0X02,
  HID_USG_GDT_Joystick                  = 0X04,
  HID_USG_GDT_GamePad                   = 0X05,
  HID_USG_GDT_Keyboard                  = 0X06,
  HID_USG_GDT_Keypad                    = 0X07,
  HID_USG_GDT_MultiAxisController       = 0X08,
  HID_USG_GDT_TabletPCSystemControls    = 0X09,
  HID_USG_GDT_X                         = 0X30,
  HID_USG_GDT_Y                         = 0X31,
  HID_USG_GDT_Z                         = 0X32,
  HID_USG_GDT_Rx                        = 0X33,
  HID_USG_GDT_Ry                        = 0X34,
  HID_USG_GDT_Rz                        = 0X35,
  HID_USG_GDT_Slider                    = 0X36,
  HID_USG_GDT_Dial                      = 0X37,
  HID_USG_GDT_Wheel                     = 0X38,
  HID_USG_GDT_HatSwitch                 = 0X39,
  HID_USG_GDT_CountedBuffer             = 0X3A,
  HID_USG_GDT_ByteCount                 = 0X3B,
  HID_USG_GDT_MotionWakeup              = 0X3C,
  HID_USG_GDT_Start                     = 0X3D,
  HID_USG_GDT_Select                    = 0X3E,
  HID_USG_GDT_Vx                        = 0X40,
  HID_USG_GDT_Vy                        = 0X41,
  HID_USG_GDT_Vz                        = 0X42,
  HID_USG_GDT_Vbrx                      = 0X43,
  HID_USG_GDT_Vbry                      = 0X44,
  HID_USG_GDT_Vbrz                      = 0X45,
  HID_USG_GDT_Vno                       = 0X46,
  HID_USG_GDT_FeatureNotification       = 0X47,
  HID_USG_GDT_ResolutionMultiplier      = 0X48,
  HID_USG_GDT_SystemControl             = 0X80,
  HID_USG_GDT_SystemPowerDown           = 0X81,
  HID_USG_GDT_SystemSleep               = 0X82,
  HID_USG_GDT_SystemWakeUp              = 0X83,
  HID_USG_GDT_SystemContextMenu         = 0X84,
  HID_USG_GDT_SystemMainMenu            = 0X85,
  HID_USG_GDT_SystemAppMenu             = 0X86,
  HID_USG_GDT_SystemMenuHelp            = 0X87,
  HID_USG_GDT_SystemMenuExit            = 0X88,
  HID_USG_GDT_SystemMenuSelect          = 0X89,
  HID_USG_GDT_SystemMenuRight           = 0X8A,
  HID_USG_GDT_SystemMenuLeft            = 0X8B,
  HID_USG_GDT_SystemMenuUp              = 0X8C,
  HID_USG_GDT_SystemMenuDown            = 0X8D,
  HID_USG_GDT_SystemColdRestart         = 0X8E,
  HID_USG_GDT_SystemWarmRestart         = 0X8F,
  HID_USG_GDT_DPadUp                    = 0X90,
  HID_USG_GDT_DPadDown                  = 0X91,
  HID_USG_GDT_DPadRight                 = 0X92,
  HID_USG_GDT_DPadLeft                  = 0X93,
  HID_USG_GDT_SystemDock                = 0XA0,
  HID_USG_GDT_SystemUndock              = 0XA1,
  HID_USG_GDT_SystemSetup               = 0XA2,
  HID_USG_GDT_SystemBreak               = 0XA3,
  HID_USG_GDT_SystemDebuggerBreak       = 0XA4,
  HID_USG_GDT_ApplicationBreak          = 0XA5,
  HID_USG_GDT_ApplicationDebuggerBreak  = 0XA6,
  HID_USG_GDT_SystemSpeakerMute         = 0XA7,
  HID_USG_GDT_SystemHibernate           = 0XA8,
  HID_USG_GDT_SystemDisplayInvert       = 0XB0,
  HID_USG_GDT_SystemDisplayInternal     = 0XB1,
  HID_USG_GDT_SystemDisplayExternal     = 0XB2,
  HID_USG_GDT_SystemDisplayBoth         = 0XB3,
  HID_USG_GDT_SystemDisplayDual         = 0XB4,
  HID_USG_GDT_SystemDisplayToggleIntExt = 0XB5,
  HID_USG_GDT_SystemDisplaySwap         = 0XB6,
  HID_USG_GDT_SystemDisplayLCDAutoscale = 0XB7,
} HidGenericDesktopUsage;

typedef enum {
  HID_USG_APP_Pointer = 0X01,
} HidApplicationUsage;

typedef enum {
  HID_USG_BTN_Button1 = 0X01,
  HID_USG_BTN_Button2 = 0X02,
  HID_USG_BTN_Button3 = 0X03,
} HidButtonUsage;

typedef enum {
  HID_USG_FLG_CONSTANT      = 0X001,
  HID_USG_FLG_VARIABLE      = 0X002,
  HID_USG_FLG_RELATIVE      = 0X004,
  HID_USG_FLG_WRAP          = 0X008,
  HID_USG_FLG_NON_LINEAR    = 0X010,
  HID_USG_FLG_NO_PREFERRED  = 0X020,
  HID_USG_FLG_NULL_STATE    = 0X040,
  HID_USG_FLG_VOLATILE      = 0X080,
  HID_USG_FLG_BUFFERED_BYTE = 0X100,

  HID_USG_FLG_DATA           = 0, // !HID_USG_FLG_CONSTANT,
  HID_USG_FLG_ARRAY          = 0, // !HID_USG_FLG_VARIABLE,
  HID_USG_FLG_ABSOLUTE       = 0, // !HID_USG_FLAG_RELATIVE,
} HidUsageFlags;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_HID_DEFS */
