/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* MultiBraille/tables.h - keybindings for the MultiBraille
 * Wolfgang Astleitner, March 2000
 * tables.h,v 1.1
 */

/*
  Calculation mask of key-values for top-keys: 1-2-4--8-16-32  (Key 3-2-1--4-5-6)

 hex-values returned when pressing either a front-key or top-key combination
 ===========================================================================
    'S'   'T'    S-bits  'T' meaning                           used brltty-cmd
   ====  ====   ======= ==== ================================  ===============
   Movement keys:
 * 0x07         321          top of screen                     CMD_TOP
 * 0x38             456      bottom of screen                  CMD_BOT
 ? 0x06          21          up several lines                  CMD_NXDIFLN
 ? 0x18             45       down several lines                CMD_PRDIFLN
 * 0x04  0x0e     1     (B)  up one line                       CMD_LNUP
 * 0x08  0x13       4   (D)  down one line                     CMD_LNDN
 ? 0x21  0x10   3     6 (CC) cursor position                   CR_ROUTE
 * 0x03         32           beginning of line                 CMD_LNBEG
 * 0x30              56      end of line                       CMD_LNEND
 * 0x05         3 1          left one character                CMD_CHRLT
 * 0x28             4 6      right one character               CMD_CHRRT
 * 0x2a          2  4 6      left one half window              CMD_HWINLT
 * 0x15         3 1  5       right one half window             CMD_HWINRT
 * 0x01  0x0d   3       (A)  left one full window              CMD_FWINLT
 * 0x20  0x14         6 (E)  right one full window             CMD_FWINRT

  Other functions:
 * 0x34           1  56      speak current line                CMD_SAY_LINE
 * 0x24           1   6      mute speech                       CMD_MUTE
 ? 0x1e          21 45       route cursor to start of window   CMD_HOME
 * 0x23         32    6      cut start                         CR_CUTBEGIN
 * 0x31         3    56      cut end                           CR_CUTRECT+brlcols-1
 * 0x0f         321 4        paste                             CMD_PASTE
 * 0x10              5       cursor visibility on/off          CMD_CSRVIS
 * 0x0c  0x10     1 4   (C)  cursor tracking on/off            CMD_CSRTRK
 * 0x02          2           cursor blink on/off               CMD_CSRBLINK
 * 0x2c           1 4 6      capital letter blink on/off       CMD_CAPBLINK
 ? 0x12          2   5       block/underline cursor            CMD_ATTRVIS
 * 0x13         32   5       six/eight dot braille text        CMD_SIXDOTS
 * 0x3a          2  456      sliding window on/off             CMD_SLIDEWIN
 * 0x1a          2  45       skip identical lines on/off       CMD_SKPIDLNS
 * 0x0b         32  4        audio signals on/off              CMD_TUNES
 * 0x0d         3 1 4        attribute display on/off          CMD_DISPMD
 * 0x0e          21 4        freeze mode on/off                CMD_FREEZE
 * 0x16          21  5       help display on/off               CMD_HELP
 * 0x09         3   4        status mode on/off                CMD_INFO

  Preferences control:
 * 0x3f         321 456      save preferences        CMD_PREFSAVE
 * 0x2d         3 1 4 6      enter preferences menu          CMD_PREFMENU
 * 0x17         321  5       restore preferences          CMD_PREFLOAD

  Explanation:
	A '?' before a line means that I was not sure if the used command is really OK.
	A '*' means that the used command should be fine
 */



/* 
 Command translation table for 'T' events (front/thumb keys, block with keys '0-9', '*', '#'): 
 key numbers for front keys are: (keys in brackets are not available for all braille lines!
   13 - 14 - (15) - 16 - (17) - (18) - 19 - 20 - (21) - (22)
 key numbers for block keys ( '0' - '9', '*', '#'; MB185CR only):
   1 - 2 - 3 - 4 - 5 - 6 - 7 - 8 - 9 - 10 - 11 - 12
	 
 So if you have for example a MB185CR and you want to react front key with number 15
 for example to jump to the beginning of the line, you have to do the following:
 * Search in table cmd_T_trans the element with index 0x0f (== decimal 15)
   (Attention: counting starts with 0)
 * instead of the default entry '0x00' enter CMD_LNBEG (all commands are explained above)
 * rebuild brltty and the next time you are pressing front key 15 the cursor will
   jump to the beginning of the line
 
*/
static int cmd_T_trans[23] = {
/* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
/* 0x08 */ 0x00, 0x00, 0x00, 0x00, 0x00, CMD_FWINLT, CMD_LNUP, 0x00, 
/* 0x10 */ CMD_CSRTRK, 0x00, 0x00, CMD_LNDN, CMD_FWINRT, 0x00, 0x00
};


/* Command translation table for 'S' events (braille dot keys) */
/* 63 customizable key-bindings! (index 0 can't be customized ..) */
/* combinations returning 0x00 are ignored */
static int cmd_S_trans[0x40] = {
/* 0x00 */  0x00,         CMD_FWINLT,   CMD_CSRBLINK, CMD_LNBEG,   
/* 0x04 */  CMD_LNUP,     CMD_CHRLT,    CMD_NXDIFLN,  CMD_TOP,
/* 0x08 */  CMD_LNDN,     CMD_INFO,     0x00,         CMD_TUNES,      
/* 0x0c */  CMD_CSRTRK,   CMD_DISPMD,   CMD_FREEZE,   CMD_PASTE, 
/* 0x10 */  CMD_CSRVIS,   0x00,         CMD_ATTRVIS,  CMD_SIXDOTS,        
/* 0x14 */  0x00,         CMD_HWINRT,   CMD_HELP,     CMD_PREFLOAD, 
/* 0x18 */  CMD_PRDIFLN,  0x00,         CMD_SKPIDLNS, 0x00,        
/* 0x1c */  0x00,         0x00,         CMD_HOME,     0x00, 
/* 0x20 */  CMD_FWINRT,   CR_ROUTE,     0x00,         CR_CUTBEGIN, 
/* 0x24 */  CMD_MUTE,     0x00,         0x00,         0x00, 
/* 0x28 */  CMD_CHRRT,    0x00,         CMD_HWINLT,   0x00,        
/* 0x2c */  CMD_CAPBLINK, CMD_PREFMENU, 0x00,         0x00, 
/* 0x30 */  CMD_LNEND,    CR_CUTRECT,   0x00,         0x00,        
/* 0x34 */  CMD_SAY_LINE, 0x00,         0x00,         0x00, 
/* 0x38 */  CMD_BOT,      0x00,         CMD_SLIDEWIN, 0x00,        
/* 0x3c */  0x00,         0x00,         0x00,         CMD_PREFSAVE, 
};


/* Command translation table for 'R' events (cursor routing keys)*/
/* only keys 3, 4, 5 (startindex: 0) can be customized!! others are
   ignored (hard-coded)! */
static int cmd_R_trans[MB_CR_EXTRAKEYS] = {
/* 0x00 */ 0x00, 0x00, 0x00, CMD_PREFMENU, CMD_PREFLOAD, CMD_HELP
};
