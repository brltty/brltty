/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "prologue.h"

#include "misc.h"
#include "system.h"

#include "sys_prog_none.h"

#include "sys_boot_none.h"

#include "sys_exec_none.h"

#include "sys_shlib_none.h"

#include "sys_beep_msdos.h"

#ifdef ENABLE_PCM_SUPPORT
#include "sys_pcm_none.h"
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
#include "sys_midi_none.h"
#endif /* ENABLE_MIDI_SUPPORT */

#include "sys_ports_always.h"
#include "sys_ports_x86.h"

#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <dpmi.h>
#include <pc.h>
#include <dos.h>
#include <go32.h>
#include <crt0.h>
#include <sys/farptr.h>

int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

/* reduce image */
int _stklen = 8192;

void __crt0_load_environment_file(char *_app_name) { return; }
char **__crt0_glob_function(char *_arg) { return 0; }

static void tsr_exit(void);
/* Start undocumented way to make exception handling disappear (v2.03) */
short __djgpp_ds_alias;
void __djgpp_exception_processor(void) { return; }
void __djgpp_exception_setup(void) { return; }
void __djgpp_exception_toggle(void) { return; }
int __djgpp_set_ctrl_c(int enable) { return 0; }
void __maybe_fix_w2k_ntvdm_bug(void) { }
void abort(void) { tsr_exit(); }
void _exit(int status) { tsr_exit(); }
int raise(int sig) { return 0; }
void *signal(int signum, void*handler) { return NULL; }
/* End undocumented way to make exception handling disappear */

#define TIMER_INT 0x8
#define DOS_INT 0x21
#define IDLE_INT 0x28

#define PIT_FREQ UINT64_C(1193180)

/* For saving interrupt and main contexts */

typedef char FpuState[(7+20)*8];
struct Dta {
  unsigned short seg;
  unsigned short off;
};

static struct State {
  FpuState fpu;
  struct Dta dta;
  unsigned short psp;
} intState, mainState;
static jmp_buf mainCtx, intCtx;

static int backgrounded; /* whether we really TSR */
static unsigned long inDosOffset, criticalOffset;
static volatile int inInt, inIdle; /* prevent reentrancy */
static volatile unsigned long elapsedTicks, toBeElapsedTicks;
static __dpmi_regs idleRegs;

static _go32_dpmi_seginfo origTimerSeginfo, timerSeginfo;
static _go32_dpmi_seginfo origIdleSeginfo,  idleSeginfo;

/* Handle PSP switch for proper file descriptor table */
static unsigned short getPsp(void) {
  __dpmi_regs r;
  r.h.ah = 0x51;
  __dpmi_int(DOS_INT, &r);
  return r.x.bx;
}

static void setPsp(unsigned short psp) {
  __dpmi_regs r;
  r.h.ah = 0x50;
  r.x.bx = psp;
  __dpmi_int(DOS_INT, &r);
}

/* Handle Dta switch since djgpp uses it for FindFirst/FindNext */
static void getDta(struct Dta *dta) {
  __dpmi_regs r;
  r.h.ah = 0x2f;
  __dpmi_int(DOS_INT, &r);
  dta->seg = r.x.es;
  dta->off = r.x.bx;
}

static void setDta(const struct Dta *dta) {
  __dpmi_regs r;
  r.h.ah = 0x1a;
  r.x.ds = dta->seg;
  r.x.dx = dta->off;
  __dpmi_int(DOS_INT, &r);
}

/* Handle FPU state switch */
#define saveFpuState(p) asm volatile("fnsave (%0); fwait"::"r"(p):"memory")
#define restoreFpuState(p) asm volatile("frstor (%0)"::"r"(p))

static void saveState(struct State *state) {
  saveFpuState(&state->fpu);
  getDta(&state->dta);
  state->psp = getPsp();
}
static void restoreState(const struct State *state) {
  restoreFpuState(&state->fpu);
  setDta(&state->dta);
  setPsp(state->psp);
}

static unsigned short countToNextTimerInt(void) {
  unsigned char clo, chi;
  outportb(0x43, 0xd2);
  clo = inportb(0x40);
  chi = inportb(0x40);
  return ((chi<<8) | clo);
}

/* Timer interrupt handler */
static void
timerInt(void) {
  elapsedTicks += toBeElapsedTicks;
  toBeElapsedTicks = countToNextTimerInt();

  if (!inIdle && !inInt) {
    inInt = 1;
    if (!setjmp(intCtx))
      longjmp(mainCtx, 1);
    inInt = 0;
  }
}

/* Idle interrupt handler */
static void
idleInt(_go32_dpmi_registers *r) {
  if (!inIdle && !inInt) {
    inIdle = 1;
    if (!setjmp(intCtx))
      longjmp(mainCtx, 1);
    inIdle = 0;
  }
  r->x.cs = origIdleSeginfo.rm_segment;
  r->x.ip = origIdleSeginfo.rm_offset;
  _go32_dpmi_simulate_fcall_iret(r);
}

/* Try to restore interrupt handler */
static int restore(int vector, _go32_dpmi_seginfo *seginfo, _go32_dpmi_seginfo *orig_seginfo) {
  _go32_dpmi_seginfo cur_seginfo;

  _go32_dpmi_get_protected_mode_interrupt_vector(vector, &cur_seginfo);

  if (cur_seginfo.pm_selector != seginfo->pm_selector ||
      cur_seginfo.pm_offset != seginfo->pm_offset)
    return 1;

  _go32_dpmi_set_protected_mode_interrupt_vector(vector, orig_seginfo);
  return 0;
}

/* TSR exit: trying to free as many resources as possible */
static void
tsr_exit(void) {
  __dpmi_regs regs;
  unsigned long pspAddr = _go32_info_block.linear_address_of_original_psp;

  if (restore(TIMER_INT, &timerSeginfo, &origTimerSeginfo) +
      restore(IDLE_INT,  &idleSeginfo,  &origIdleSeginfo)) {
    /* failed, hang */
    setjmp(mainCtx);
    longjmp(intCtx, 1);
  }

  /* free environment */
  regs.x.es = _farpeekw(_dos_ds,pspAddr+0x2c);
  regs.x.ax = 0x4900;
  __dpmi_int(DOS_INT, &regs);

  /* free PSP */
  regs.x.es = pspAddr/0x10;
  regs.x.ax = 0x4900;
  __dpmi_int(DOS_INT, &regs);

  /* and return */
  longjmp(intCtx, 1);

  /* TODO: free protected mode memory */
}

/* go to background: TSR */
void
background(void) {
  saveState(&mainState);
  if (!setjmp(mainCtx)) {
    __dpmi_regs regs;

    /* set a chained Protected Mode Timer IRQ handler */
    timerSeginfo.pm_selector = _my_cs();
    timerSeginfo.pm_offset = (unsigned long)&timerInt;
    _go32_dpmi_get_protected_mode_interrupt_vector(TIMER_INT, &origTimerSeginfo);
    _go32_dpmi_chain_protected_mode_interrupt_vector(TIMER_INT, &timerSeginfo);

    /* set a real mode DOS Idle handler which calls back our Idle handler */
    idleSeginfo.pm_selector = _my_cs();
    idleSeginfo.pm_offset = (unsigned long)&idleInt;
    memset(&idleRegs, 0, sizeof(idleRegs));
    _go32_dpmi_get_real_mode_interrupt_vector(IDLE_INT, &origIdleSeginfo);
    _go32_dpmi_allocate_real_mode_callback_iret(&idleSeginfo, &idleRegs);
    _go32_dpmi_set_real_mode_interrupt_vector(IDLE_INT, &idleSeginfo);

    /* Get InDos and Critical flags addresses */
    regs.h.ah = 0x34;
    __dpmi_int(DOS_INT, &regs);
    inDosOffset = regs.x.es*16 + regs.x.bx;

    regs.x.ax = 0x5D06;
    __dpmi_int(DOS_INT, &regs);
    criticalOffset = regs.x.ds*16 + regs.x.si;

    /* We are ready */
    atexit(tsr_exit);
    backgrounded = 1;

    regs.x.ax = 0x3100;
    regs.x.dx = (/* PSP */ 256 + _go32_info_block.size_of_transfer_buffer) / 16;
    __dpmi_int(DOS_INT, &regs);

    /* shouldn't be reached */
    LogPrint(LOG_ERR,"Installation failed\n");
    backgrounded = 0;
  }
  saveState(&intState);
  restoreState(&mainState);
}

unsigned long
tsr_usleep(unsigned long usec) {
  unsigned long count;
  int intsWereEnabled;

  if (!backgrounded) {
    usleep(usec);
    return usec;
  }

  saveState(&mainState);
  restoreState(&intState);

  /* clock ticks to wait */
  count = (usec * PIT_FREQ) / UINT64_C(1000000);

  /* we're starting in the middle of a timer period */
  intsWereEnabled = disable();
  toBeElapsedTicks = countToNextTimerInt();
  elapsedTicks = 0;
  if (intsWereEnabled)
    enable();

  while (elapsedTicks < count) {
    /* wait for next interrupt */
    if (!setjmp(mainCtx))
      longjmp(intCtx, 1);
    /* interrupt returned */
  }

  /* wait for Dos to be free */
  setjmp(mainCtx);

  /* critical sections of DOS are never reentrant */
  if (_farpeekb(_dos_ds, criticalOffset)
  /* DOS is busy but not idle */
   || (!inIdle && _farpeekb(_dos_ds, inDosOffset)))
    longjmp(intCtx, 1);

  saveState(&intState);
  restoreState(&mainState);

  return (elapsedTicks * UINT64_C(1000000)) / PIT_FREQ;
}

int
vsnprintf (char *str, size_t size, const char *format, va_list ap) {
  size_t alloc = 1024;
  char *buf;
  int ret;
  if (alloc < size)
    alloc = size;
  buf = alloca(alloc);
  ret = vsprintf(buf, format, ap);
  if (size > ret + 1)
    size = ret + 1;
  memcpy(str, buf, size);
  return ret;
}

int
snprintf (char *str, size_t size, const char *format, ...) {
  va_list argp;
  int ret;
  va_start(argp, format);
  ret = vsnprintf(str, size, format, argp);
  va_end(argp);
  return ret;
}
