/*
 * brltty - macOS Accessibility screen driver: implementation of the AX bridge.
 *
 * This translation unit owns all Apple AX/AppKit imports so that screen.m
 * stays free of any header that transitively pulls in Quickdraw.
 */

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>  // UCKeyTranslate, TISCopyCurrentKeyboardLayoutInputSource

#include "ax_bridge.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// brltty core: schedule a screen redraw on the main update loop. Declared
// here rather than via scr_main.h so this translation unit stays free of
// brltty headers (which collide with Quickdraw).
extern void mainScreenUpdated(void);

static NSString *
copyStringAttribute(AXUIElementRef element, CFStringRef attribute) {
    if (!element) return nil;
    CFTypeRef value = NULL;
    AXError err = AXUIElementCopyAttributeValue(element, attribute, &value);
    if (err != kAXErrorSuccess || !value) return nil;
    if (CFGetTypeID(value) != CFStringGetTypeID()) {
        CFRelease(value);
        return nil;
    }
    return [(NSString *)value autorelease];
}

static AXUIElementRef
copyElementAttribute(AXUIElementRef element, CFStringRef attribute) {
    if (!element) return NULL;
    CFTypeRef value = NULL;
    AXError err = AXUIElementCopyAttributeValue(element, attribute, &value);
    if (err != kAXErrorSuccess || !value) return NULL;
    return (AXUIElementRef)value;
}

static int
copyIntAttribute(AXUIElementRef element, CFStringRef attribute, int *out) {
    if (!element) return 0;
    CFTypeRef value = NULL;
    AXError err = AXUIElementCopyAttributeValue(element, attribute, &value);
    if (err != kAXErrorSuccess || !value) return 0;
    if (CFGetTypeID(value) != CFNumberGetTypeID()) { CFRelease(value); return 0; }
    int v = 0;
    Boolean ok = CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &v);
    CFRelease(value);
    if (!ok) return 0;
    *out = v;
    return 1;
}

static int
copyRangeAttribute(AXUIElementRef element, CFStringRef attribute, CFRange *out) {
    if (!element) return 0;
    CFTypeRef value = NULL;
    AXError err = AXUIElementCopyAttributeValue(element, attribute, &value);
    if (err != kAXErrorSuccess || !value) return 0;
    AXValueType type = AXValueGetType((AXValueRef)value);
    if (type != kAXValueCFRangeType) { CFRelease(value); return 0; }
    Boolean ok = AXValueGetValue((AXValueRef)value, kAXValueCFRangeType, out);
    CFRelease(value);
    return ok ? 1 : 0;
}

// Resolve cursor row/col from an AXTextArea-like element. Uses the parameterized
// attribute AXLineForIndex to convert the caret offset into a line number,
// then AXRangeForLine to get the line's start offset; the column is the
// difference. Returns 1 on success.
static int
computeCursorPosition(AXUIElementRef element, int *outRow, int *outCol) {
    if (!element) return 0;
    CFRange selected;
    if (!copyRangeAttribute(element, kAXSelectedTextRangeAttribute, &selected)) return 0;

    CFIndex caret = selected.location;
    CFTypeRef value = NULL;

    // Convert caret offset -> line number.
    CFNumberRef idxNum = CFNumberCreate(NULL, kCFNumberCFIndexType, &caret);
    AXError err = AXUIElementCopyParameterizedAttributeValue(
        element, kAXLineForIndexParameterizedAttribute, idxNum, &value);
    CFRelease(idxNum);
    if (err != kAXErrorSuccess || !value) return 0;
    int line = 0;
    Boolean ok = CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &line);
    CFRelease(value);
    if (!ok) return 0;

    // Convert line -> CFRange (start offset, length).
    CFNumberRef lineNum = CFNumberCreate(NULL, kCFNumberIntType, &line);
    err = AXUIElementCopyParameterizedAttributeValue(
        element, kAXRangeForLineParameterizedAttribute, lineNum, &value);
    CFRelease(lineNum);
    if (err != kAXErrorSuccess || !value) {
        *outRow = line;
        *outCol = 0;
        return 1;
    }
    CFRange lineRange = {0, 0};
    AXValueGetValue((AXValueRef)value, kAXValueCFRangeType, &lineRange);
    CFRelease(value);

    *outRow = line;
    *outCol = (int)(caret - lineRange.location);
    if (*outCol < 0) *outCol = 0;
    return 1;
}

// Read kAXSize as area (w*h). Returns 0 if not available.
static double
elementArea(AXUIElementRef el) {
    if (!el) return 0;
    CFTypeRef sizeCF = NULL;
    if (AXUIElementCopyAttributeValue(el, kAXSizeAttribute, &sizeCF) != kAXErrorSuccess
        || !sizeCF) return 0;
    CGSize sz = {0, 0};
    if (AXValueGetType((AXValueRef)sizeCF) == kAXValueCGSizeType) {
        AXValueGetValue((AXValueRef)sizeCF, kAXValueCGSizeType, &sz);
    }
    CFRelease(sizeCF);
    return (double)sz.width * (double)sz.height;
}

// Recursive search for the largest TextArea/TextField in the subtree.
// Used after a tab switch: the first text-like element a naive DFS finds
// can be a small toolbar TextField (search bar). The shell text view
// occupies almost the whole window, so picking the largest gets it right.
// Updates *bestEl and *bestArea in-place. Caller must CFRelease *bestEl
// when done.
static void
findLargestTextLikeDescendant(AXUIElementRef root, int maxDepth,
                              int allowTextField,
                              AXUIElementRef *bestEl, double *bestArea) {
    if (!root || maxDepth < 0) return;

    NSString *role = copyStringAttribute(root, kAXRoleAttribute);
    int isTextArea  = [role isEqualToString:(NSString *)kAXTextAreaRole];
    int isTextField = [role isEqualToString:(NSString *)kAXTextFieldRole];
    if (isTextArea || (allowTextField && isTextField)) {
        double a = elementArea(root);
        if (a > *bestArea) {
            if (*bestEl) CFRelease(*bestEl);
            CFRetain(root);
            *bestEl = root;
            *bestArea = a;
        }
        /* Don't descend further inside text elements — their children are
         * usually layout/glyph nodes, not other widgets. */
        return;
    }

    CFTypeRef childrenCF = NULL;
    if (AXUIElementCopyAttributeValue(root, kAXChildrenAttribute, &childrenCF) != kAXErrorSuccess
        || !childrenCF) {
        return;
    }
    if (CFGetTypeID(childrenCF) != CFArrayGetTypeID()) {
        CFRelease(childrenCF);
        return;
    }

    CFArrayRef children = (CFArrayRef)childrenCF;
    CFIndex n = CFArrayGetCount(children);
    if (n > 64) n = 64;
    for (CFIndex i = 0; i < n; i += 1) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        findLargestTextLikeDescendant(child, maxDepth - 1, allowTextField,
                                      bestEl, bestArea);
    }
    CFRelease(childrenCF);
}

// Convenience: find the most likely "main content" text view inside root.
// Prefers TextArea; falls back to TextField only if no TextArea exists.
static AXUIElementRef
findShellTextArea(AXUIElementRef root, int maxDepth) {
    AXUIElementRef best = NULL;
    double area = 0;
    findLargestTextLikeDescendant(root, maxDepth, /*allowTextField=*/0,
                                  &best, &area);
    if (best) return best;
    findLargestTextLikeDescendant(root, maxDepth, /*allowTextField=*/1,
                                  &best, &area);
    return best;
}

// Climb up from any node toward the nearest text container (TextArea,
// TextField, ScrollArea wrapping a TextArea). Returns a retained reference
// or NULL.
static AXUIElementRef
findTextContainer(AXUIElementRef element) {
    AXUIElementRef cur = element;
    if (cur) CFRetain(cur);

    for (int depth = 0; cur && depth < 6; depth += 1) {
        NSString *role = copyStringAttribute(cur, kAXRoleAttribute);
        if ([role isEqualToString:(NSString *)kAXTextAreaRole]
         || [role isEqualToString:(NSString *)kAXTextFieldRole]) {
            return cur;
        }
        AXUIElementRef parent = copyElementAttribute(cur, kAXParentAttribute);
        CFRelease(cur);
        cur = parent;
    }
    if (cur) CFRelease(cur);
    return NULL;
}

// ---------------------------------------------------------------------------
// AXObserver-backed change detection. macOS apps (Terminal.app in particular)
// publish AX state asynchronously: querying repeatedly returns the same
// snapshot until the source app actively pushes an update through its
// accessibility tree. To react to keystrokes we therefore register an
// AXObserver and let macOS notify us when the value, caret or focus moves.
//
// A dedicated thread runs a CFRunLoop on which the observer is scheduled,
// plus a periodic timer that re-attaches the observer when the frontmost
// application or its focused text container changes.

static _Atomic int observerDirty = 0;
static _Atomic int observerRunning = 0;
static pthread_t observerThread;
static CFRunLoopRef observerRunLoop = NULL;

// Self-pipe used to wake the brltty main thread when our AXObserver thread
// sees an event. brltty registers the read end with asyncMonitorFileInput.
static int wakePipeRead = -1;
static int wakePipeWrite = -1;

static pthread_mutex_t observerLock = PTHREAD_MUTEX_INITIALIZER;
static pid_t observedPid = 0;
static AXObserverRef observedObserver = NULL;
static AXUIElementRef observedAppElement = NULL;
static AXUIElementRef observedFocusedElement = NULL;

// ax_frontmost_bundle_id() reads NSWorkspace.frontmostApplication
// directly on each call (see the function definition further down).
// No cache, no observer-thread synchronisation needed.

static const CFStringRef kAppNotifications[] = {
    CFSTR("AXFocusedUIElementChanged"),
    CFSTR("AXFocusedWindowChanged"),
    CFSTR("AXMainWindowChanged"),
    CFSTR("AXWindowMiniaturized"),
    CFSTR("AXWindowDeminiaturized"),
};

static const CFStringRef kFocusedNotifications[] = {
    CFSTR("AXValueChanged"),
    CFSTR("AXSelectedTextChanged"),
    CFSTR("AXTitleChanged"),
    /* Fires after the text container's layout has settled — Terminal.app
     * sends this *after* AXSelectedTextChanged once the new prompt line
     * is actually visible in AXStringForRange. Without it we read a
     * transient state where the cursor row is empty. */
    CFSTR("AXLayoutChanged"),
    /* Fires when the visible character range itself moves (scrollback
     * advance, page-up/page-down inside Terminal). Without it we'd miss
     * scroll-only updates where the cursor doesn't move. */
    CFSTR("AXVisibleCharacterRangeChanged"),
    /* Some text views post this when content is appended/replaced. */
    CFSTR("AXRowCountChanged"),
};

// Diagnostic trace path. Enabled by setting BRLTTY_AX_DEBUG_LOG in the
// environment to a writable file path (typically /tmp/brltty-ax.log).
// Useful when investigating AX behaviour on a specific app; off by default.
static void
axDebugLog(const char *fmt, ...) {
    static FILE *f = NULL;
    static int initialized = 0;
    if (!initialized) {
        initialized = 1;
        const char *path = getenv("BRLTTY_AX_DEBUG_LOG");
        if (path && *path) {
            f = fopen(path, "a");
            if (f) setlinebuf(f);
        }
    }
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
}

static void
axCallback(AXObserverRef obs, AXUIElementRef element, CFStringRef notification, void *refcon) {
    (void)obs;
    (void)element;
    (void)refcon;
    char name[64] = "?";
    CFStringGetCString(notification, name, sizeof(name), kCFStringEncodingUTF8);
    axDebugLog("notification: %s\n", name);
    atomic_store(&observerDirty, 1);
    // Wake the brltty main thread by writing one byte to the self-pipe.
    // brltty's async file-input monitor will run on its own thread and call
    // mainScreenUpdated() there, which is the only safe way to touch the
    // brltty async alarm queue.
    if (wakePipeWrite >= 0) {
        unsigned char b = 1;
        ssize_t r = write(wakePipeWrite, &b, 1);
        (void)r;
    }
}

static void
detachObservers_locked(void) {
    if (!observedObserver) {
        if (observedAppElement) { CFRelease(observedAppElement); observedAppElement = NULL; }
        if (observedFocusedElement) { CFRelease(observedFocusedElement); observedFocusedElement = NULL; }
        return;
    }

    if (observedAppElement) {
        for (size_t i = 0; i < sizeof(kAppNotifications) / sizeof(kAppNotifications[0]); i += 1) {
            AXObserverRemoveNotification(observedObserver, observedAppElement, kAppNotifications[i]);
        }
        CFRelease(observedAppElement);
        observedAppElement = NULL;
    }
    if (observedFocusedElement) {
        for (size_t i = 0; i < sizeof(kFocusedNotifications) / sizeof(kFocusedNotifications[0]); i += 1) {
            AXObserverRemoveNotification(observedObserver, observedFocusedElement, kFocusedNotifications[i]);
        }
        CFRelease(observedFocusedElement);
        observedFocusedElement = NULL;
    }

    if (observerRunLoop) {
        CFRunLoopRemoveSource(observerRunLoop, AXObserverGetRunLoopSource(observedObserver), kCFRunLoopDefaultMode);
    }
    CFRelease(observedObserver);
    observedObserver = NULL;
    observedPid = 0;
}

static void
attachObservers_locked(pid_t pid, AXUIElementRef appElement, AXUIElementRef focusedElement) {
    if (AXObserverCreate(pid, axCallback, &observedObserver) != kAXErrorSuccess
        || !observedObserver) {
        observedObserver = NULL;
        return;
    }
    for (size_t i = 0; i < sizeof(kAppNotifications) / sizeof(kAppNotifications[0]); i += 1) {
        AXObserverAddNotification(observedObserver, appElement, kAppNotifications[i], NULL);
    }
    if (focusedElement) {
        for (size_t i = 0; i < sizeof(kFocusedNotifications) / sizeof(kFocusedNotifications[0]); i += 1) {
            AXObserverAddNotification(observedObserver, focusedElement, kFocusedNotifications[i], NULL);
        }
    }

    CFRetain(appElement);
    observedAppElement = appElement;
    if (focusedElement) {
        CFRetain(focusedElement);
        observedFocusedElement = focusedElement;
    }
    observedPid = pid;

    CFRunLoopAddSource(observerRunLoop, AXObserverGetRunLoopSource(observedObserver), kCFRunLoopDefaultMode);
}

// Reattach the observer to the current frontmost app / focused element.
// Called from the observer thread on a timer.
static void
reattachIfNeeded(void) {
    @autoreleasepool {
        NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
        if (!app) return;
        pid_t pid = app.processIdentifier;

        AXUIElementRef appEl = AXUIElementCreateApplication(pid);
        AXUIElementRef focused = copyElementAttribute(appEl, kAXFocusedUIElementAttribute);
        AXUIElementRef textContainer = findTextContainer(focused);
        AXUIElementRef toWatch = textContainer ? textContainer : focused;

        pthread_mutex_lock(&observerLock);
        int needsReattach = 0;
        if (pid != observedPid) {
            needsReattach = 1;
        } else if ((toWatch == NULL) != (observedFocusedElement == NULL)) {
            needsReattach = 1;
        } else if (toWatch && observedFocusedElement
                   && !CFEqual(toWatch, observedFocusedElement)) {
            needsReattach = 1;
        }
        if (needsReattach) {
            detachObservers_locked();
            attachObservers_locked(pid, appEl, toWatch);
            NSString *role = toWatch ? copyStringAttribute(toWatch, kAXRoleAttribute) : @"(none)";
            axDebugLog("observer attached pid=%d role=%s\n",
                       pid, role.UTF8String ?: "(none)");
            atomic_store(&observerDirty, 1);
            if (wakePipeWrite >= 0) {
                unsigned char b = 1;
                ssize_t r = write(wakePipeWrite, &b, 1);
                (void)r;
            }
        }
        pthread_mutex_unlock(&observerLock);

        if (textContainer) CFRelease(textContainer);
        if (focused) CFRelease(focused);
        CFRelease(appEl);
    }
}

static void
reattachTimerCallback(CFRunLoopTimerRef timer, void *info) {
    (void)timer;
    (void)info;
    if (!atomic_load(&observerRunning)) {
        CFRunLoopStop(CFRunLoopGetCurrent());
        return;
    }
    reattachIfNeeded();
}

static id appActivatedObserver = nil;

static void *
observerLoop(void *unused) {
    (void)unused;
    observerRunLoop = CFRunLoopGetCurrent();

    CFRunLoopTimerContext ctx = {0};
    CFRunLoopTimerRef timer = CFRunLoopTimerCreate(
        kCFAllocatorDefault,
        CFAbsoluteTimeGetCurrent(),
        0.1, /* backup fire every 100ms — catches intra-app focus changes */
        0, 0, reattachTimerCallback, &ctx);
    CFRunLoopAddTimer(observerRunLoop, timer, kCFRunLoopDefaultMode);

    /* NSWorkspace posts NSWorkspaceDidActivateApplicationNotification on
     * the main thread of the process running NSWorkspace's notification
     * center — that's the workspace notification center, which delivers
     * to whichever thread is observing. Register here so re-attach fires
     * immediately on app switch instead of waiting for the backup timer. */
    @autoreleasepool {
        NSNotificationCenter *nc = [[NSWorkspace sharedWorkspace] notificationCenter];
        appActivatedObserver = [nc addObserverForName:NSWorkspaceDidActivateApplicationNotification
                                               object:nil
                                                queue:nil
                                           usingBlock:^(NSNotification *note) {
            (void)note;
            reattachIfNeeded();
        }];
    }

    // Initial attach so the very first frame is reactive.
    reattachIfNeeded();

    CFRunLoopRun();

    if (appActivatedObserver) {
        NSNotificationCenter *nc = [[NSWorkspace sharedWorkspace] notificationCenter];
        [nc removeObserver:appActivatedObserver];
        appActivatedObserver = nil;
    }

    CFRunLoopRemoveTimer(observerRunLoop, timer, kCFRunLoopDefaultMode);
    CFRelease(timer);

    pthread_mutex_lock(&observerLock);
    detachObservers_locked();
    pthread_mutex_unlock(&observerLock);

    observerRunLoop = NULL;
    return NULL;
}

int
ax_observer_start(void) {
    int expected = 0;
    if (!atomic_compare_exchange_strong(&observerRunning, &expected, 1)) {
        return wakePipeRead;
    }
    int fds[2];
    if (pipe(fds) != 0) {
        atomic_store(&observerRunning, 0);
        return -1;
    }
    fcntl(fds[0], F_SETFL, O_NONBLOCK);
    fcntl(fds[1], F_SETFL, O_NONBLOCK);
    wakePipeRead = fds[0];
    wakePipeWrite = fds[1];

    if (pthread_create(&observerThread, NULL, observerLoop, NULL) != 0) {
        atomic_store(&observerRunning, 0);
        close(wakePipeRead);
        close(wakePipeWrite);
        wakePipeRead = wakePipeWrite = -1;
        return -1;
    }
    return wakePipeRead;
}

void
ax_observer_stop(void) {
    int expected = 1;
    if (!atomic_compare_exchange_strong(&observerRunning, &expected, 0)) return;
    if (observerRunLoop) CFRunLoopWakeUp(observerRunLoop);
    pthread_join(observerThread, NULL);
    if (wakePipeRead >= 0) close(wakePipeRead);
    if (wakePipeWrite >= 0) close(wakePipeWrite);
    wakePipeRead = wakePipeWrite = -1;
}

void
ax_observer_drain(int fd) {
    unsigned char buf[64];
    while (read(fd, buf, sizeof(buf)) > 0) { /* drain */ }
}

int
ax_consume_dirty(void) {
    return atomic_exchange(&observerDirty, 0);
}

// ---------------------------------------------------------------------------
// Key injection. brltty's ScreenKey is documented inside scr_types.h:
//   bits 0..19    Unicode codepoint, or a special-key code starting at 0xF800
//   bits 24..30   modifier flags
// macOS expects either a virtual keycode (kVK_*) or a Unicode string set
// via CGEventKeyboardSetUnicodeString. We use the virtual keycode path for
// non-printables (so they generate the same key-down/up sequence a physical
// keyboard would) and the Unicode path for everything else (avoids having
// to know the user's keyboard layout to find the right keycode).

#define BR_SHIFT     0x40000000U
#define BR_UPPER     0x20000000U
#define BR_CONTROL   0x10000000U
#define BR_ALT_LEFT  0x08000000U
#define BR_ALT_RIGHT 0x04000000U
#define BR_GUI       0x02000000U
#define BR_CAPSLOCK  0x01000000U
#define BR_CHAR_MASK 0x000FFFFFU
#define BR_UNICODE_ROW 0xF800U

// macOS virtual keycodes (HIToolbox/Events.h). Repeated here so we don't
// pull Carbon's umbrella header (which collides with brltty's headers
// similarly to Quickdraw — see screen.m for context).
enum {
    VK_Return        = 0x24, VK_Tab           = 0x30,
    VK_Delete        = 0x33, VK_Escape        = 0x35,
    VK_LeftArrow     = 0x7B, VK_RightArrow    = 0x7C,
    VK_DownArrow     = 0x7D, VK_UpArrow       = 0x7E,
    VK_PageUp        = 0x74, VK_PageDown      = 0x79,
    VK_Home          = 0x73, VK_End           = 0x77,
    VK_Help          = 0x72, // brltty INSERT -> macOS Help/Insert
    VK_ForwardDelete = 0x75,
    VK_F1 = 0x7A, VK_F2 = 0x78, VK_F3 = 0x63, VK_F4 = 0x76,
    VK_F5 = 0x60, VK_F6 = 0x61, VK_F7 = 0x62, VK_F8 = 0x64,
    VK_F9 = 0x65, VK_F10 = 0x6D, VK_F11 = 0x67, VK_F12 = 0x6F,
    VK_F13 = 0x69, VK_F14 = 0x6B, VK_F15 = 0x71, VK_F16 = 0x6A,
    VK_F17 = 0x40, VK_F18 = 0x4F, VK_F19 = 0x50, VK_F20 = 0x5A,
};

// brltty special-key codes that map to a macOS virtual keycode.
static const struct {
    uint16_t brltty;     // codepoint within BR_CHAR_MASK
    CGKeyCode vk;
} kSpecialMap[] = {
    {0xF800,      VK_Return},        // ENTER
    {0xF800 + 1,  VK_Tab},
    {0xF800 + 2,  VK_Delete},        // BACKSPACE
    {0xF800 + 3,  VK_Escape},
    {0xF800 + 4,  VK_LeftArrow},
    {0xF800 + 5,  VK_RightArrow},
    {0xF800 + 6,  VK_UpArrow},
    {0xF800 + 7,  VK_DownArrow},
    {0xF800 + 8,  VK_PageUp},
    {0xF800 + 9,  VK_PageDown},
    {0xF800 + 10, VK_Home},
    {0xF800 + 11, VK_End},
    {0xF800 + 12, VK_Help},          // INSERT
    {0xF800 + 13, VK_ForwardDelete}, // DELETE
    // 0xF800 + 14 = FUNCTION / F1, then F2 .. F20 sequentially.
    {0xF800 + 14, VK_F1},  {0xF800 + 15, VK_F2},  {0xF800 + 16, VK_F3},
    {0xF800 + 17, VK_F4},  {0xF800 + 18, VK_F5},  {0xF800 + 19, VK_F6},
    {0xF800 + 20, VK_F7},  {0xF800 + 21, VK_F8},  {0xF800 + 22, VK_F9},
    {0xF800 + 23, VK_F10}, {0xF800 + 24, VK_F11}, {0xF800 + 25, VK_F12},
    {0xF800 + 26, VK_F13}, {0xF800 + 27, VK_F14}, {0xF800 + 28, VK_F15},
    {0xF800 + 29, VK_F16}, {0xF800 + 30, VK_F17}, {0xF800 + 31, VK_F18},
    {0xF800 + 32, VK_F19}, {0xF800 + 33, VK_F20},
};

static int
specialKeyToVK(uint16_t codepoint, CGKeyCode *outVK) {
    for (size_t i = 0; i < sizeof(kSpecialMap) / sizeof(kSpecialMap[0]); i += 1) {
        if (kSpecialMap[i].brltty == codepoint) {
            *outVK = kSpecialMap[i].vk;
            return 1;
        }
    }
    return 0;
}

static CGEventFlags
modifierFlagsFromBrltty(uint32_t key) {
    CGEventFlags flags = 0;
    if (key & (BR_SHIFT | BR_UPPER)) flags |= kCGEventFlagMaskShift;
    if (key & BR_CONTROL) flags |= kCGEventFlagMaskControl;
    if (key & BR_ALT_LEFT) flags |= kCGEventFlagMaskAlternate;
    if (key & BR_ALT_RIGHT) flags |= kCGEventFlagMaskAlternate;
    if (key & BR_GUI) flags |= kCGEventFlagMaskCommand;
    if (key & BR_CAPSLOCK) flags |= kCGEventFlagMaskAlphaShift;
    return flags;
}

/* Mapping from CGEventFlags bits to the actual macOS virtual keycode of the
 * corresponding modifier key. Used to synthesize explicit modifier
 * down/up events around a key with a non-trivial main keycode (Unicode-
 * injection events carry keycode 0, so the host HID system doesn't see a
 * matching modifier release and treats the modifier as still held for
 * subsequent input). */
typedef struct {
    CGEventFlags flag;
    CGKeyCode vk;
} ModifierMapping;
static const ModifierMapping kModifierMappings[] = {
    { kCGEventFlagMaskCommand,   0x37 }, /* kVK_Command  */
    { kCGEventFlagMaskShift,     0x38 }, /* kVK_Shift    */
    { kCGEventFlagMaskAlternate, 0x3A }, /* kVK_Option   */
    { kCGEventFlagMaskControl,   0x3B }, /* kVK_Control  */
    /* CapsLock (0x39) intentionally omitted — toggling it via synthetic
     * events confuses the global lock state on macOS. brltty's UPPER
     * sticky already injects Shift instead. */
};

static void
postModifierKeys(CGEventSourceRef src, CGEventFlags flags, bool down) {
    for (size_t i = 0; i < sizeof(kModifierMappings)/sizeof(kModifierMappings[0]); i += 1) {
        const ModifierMapping *m = &kModifierMappings[i];
        if (flags & m->flag) {
            CGEventRef ev = CGEventCreateKeyboardEvent(src, m->vk, down);
            if (ev) {
                CGEventPost(kCGHIDEventTap, ev);
                CFRelease(ev);
            }
        }
    }
}

/* Look up the virtual keycode that produces a given Unicode codepoint on
 * the user's current keyboard layout. Needed because macOS matches
 * keyboard shortcuts on the (layout-translated) keycode, not on the
 * Unicode string attached via CGEventKeyboardSetUnicodeString — so a
 * Unicode-injected 'b' with Cmd flag is interpreted as Cmd+A (keycode 0
 * being kVK_ANSI_A by coincidence) regardless of the requested character.
 *
 * Returns 1 on success, writing the keycode into *outKeycode and a
 * shift-required flag into *outNeedsShift. Returns 0 if no mapping exists
 * for this character on the current layout (caller should fall back to
 * Unicode injection).
 *
 * Built lazily once: iterate every keycode 0..127 in both unshifted and
 * shifted state, ask UCKeyTranslate what each produces, and populate a
 * char -> (keycode, shift) reverse table. Covers BMP chars up to U+FFFF.
 */
static int
lookupVKForChar(UniChar ch, CGKeyCode *outKeycode, int *outNeedsShift) {
    static struct { CGKeyCode kc; uint8_t needsShift; uint8_t present; } table[0x10000];
    static int initialized = 0;
    if (!initialized) {
        initialized = 1;
        memset(table, 0, sizeof(table));

        TISInputSourceRef src = TISCopyCurrentKeyboardLayoutInputSource();
        if (!src) return 0;

        CFDataRef layoutData = (CFDataRef)TISGetInputSourceProperty(
            src, kTISPropertyUnicodeKeyLayoutData);
        if (!layoutData) { CFRelease(src); return 0; }

        const UCKeyboardLayout *layout =
            (const UCKeyboardLayout *)CFDataGetBytePtr(layoutData);
        if (!layout) { CFRelease(src); return 0; }

        UInt32 kbdType = LMGetKbdType();
        for (CGKeyCode kc = 0; kc < 128; kc += 1) {
            for (int shifted = 0; shifted <= 1; shifted += 1) {
                UInt32 modifierKeyState = shifted ? ((shiftKey >> 8) & 0xff) : 0;
                UInt32 deadKeyState = 0;
                UniChar buf[8];
                UniCharCount actualLen = 0;
                OSStatus s = UCKeyTranslate(
                    layout, kc, kUCKeyActionDown, modifierKeyState,
                    kbdType, kUCKeyTranslateNoDeadKeysBit,
                    &deadKeyState, 8, &actualLen, buf);
                if (s == noErr && actualLen == 1) {
                    UniChar c = buf[0];
                    if (!table[c].present) {
                        table[c].kc = kc;
                        table[c].needsShift = (uint8_t)shifted;
                        table[c].present = 1;
                    }
                }
            }
        }
        CFRelease(src);
    }

    if (table[ch].present) {
        if (outKeycode) *outKeycode = table[ch].kc;
        if (outNeedsShift) *outNeedsShift = table[ch].needsShift;
        return 1;
    }
    return 0;
}

int
ax_post_key(uint32_t key) {
    uint32_t value = key & BR_CHAR_MASK;
    CGEventFlags flags = modifierFlagsFromBrltty(key);

    /* Cache the event source — CGEventSourceCreate isn't free, and we end
     * up calling ax_post_key on the hot path for every braille keystroke. */
    static CGEventSourceRef src = NULL;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        /* Private source state: our flag-bearing events don't pollute the
         * global session modifier state. Avoids "stuck Cmd/Ctrl" symptoms
         * after a synthetic shortcut where the modifier appeared held to
         * subsequent physical keystrokes. */
        src = CGEventSourceCreate(kCGEventSourceStatePrivate);
    });
    if (!src) return 0;

    CGEventRef down = NULL, up = NULL;

    if (value >= BR_UNICODE_ROW) {
        CGKeyCode vk = 0;
        if (!specialKeyToVK((uint16_t)value, &vk)) {
            /* `src` is cached — do not release. */
            return 0;
        }
        down = CGEventCreateKeyboardEvent(src, vk, true);
        up   = CGEventCreateKeyboardEvent(src, vk, false);
    } else if (flags && value < 0x10000) {
        /* Modifier + printable character: resolve to a real virtual
         * keycode via the user's current keyboard layout. macOS matches
         * shortcuts on the (layout-translated) keycode, not on the
         * Unicode string we'd otherwise attach — so Cmd+(unicode-'b')
         * via keycode 0 is interpreted as Cmd+A (since keycode 0 happens
         * to be kVK_ANSI_A) regardless of the requested character.
         * Falls back to Unicode injection below if no keycode produces
         * this character on the current layout. */
        CGKeyCode kc = 0;
        int needsShift = 0;
        if (lookupVKForChar((UniChar)value, &kc, &needsShift)) {
            if (needsShift) flags |= kCGEventFlagMaskShift;
            down = CGEventCreateKeyboardEvent(src, kc, true);
            up   = CGEventCreateKeyboardEvent(src, kc, false);
        }
    }

    if (!down || !up) {
        if (down) { CFRelease(down); down = NULL; }
        if (up)   { CFRelease(up);   up   = NULL; }
        // Printable / control codepoint, no modifiers (or no layout
        // mapping): Unicode string injection. CGEventKeyboardSetUnicodeString
        // accepts UTF-16, so we surrogate-encode for chars >= U+10000.
        down = CGEventCreateKeyboardEvent(src, 0, true);
        up   = CGEventCreateKeyboardEvent(src, 0, false);
        if (down && up) {
            UniChar buf[2];
            UniCharCount len = 0;
            if (value < 0x10000) {
                buf[0] = (UniChar)value;
                len = 1;
            } else {
                uint32_t v = value - 0x10000;
                buf[0] = (UniChar)(0xD800 | (v >> 10));
                buf[1] = (UniChar)(0xDC00 | (v & 0x3FF));
                len = 2;
            }
            CGEventKeyboardSetUnicodeString(down, len, buf);
            CGEventKeyboardSetUnicodeString(up, len, buf);
        }
    }

    int ok = 0;
    if (down && up) {
        if (flags) {
            CGEventSetFlags(down, flags);
            CGEventSetFlags(up, flags);
        }
        /* When the main event uses Unicode injection (keycode 0), macOS
         * has no separate modifier release to balance the flagged event,
         * so the modifier appears "held" to subsequent physical or
         * synthetic keystrokes — sticky GUI/Cmd notably stays active
         * after a single chord application. Synthesize the missing
         * modifier-key transitions explicitly. We also do it for the
         * virtual-keycode path: harmless there, and uniform code.
         *
         * kCGHIDEventTap posts at the HIDSystem entry point — earlier in
         * the dispatch chain than kCGSessionEventTap, so the event
         * reaches the target app sooner. */
        if (flags) postModifierKeys(src, flags, /*down=*/true);
        CGEventPost(kCGHIDEventTap, down);
        CGEventPost(kCGHIDEventTap, up);
        if (flags) postModifierKeys(src, flags, /*down=*/false);
        ok = 1;
    }

    if (down) CFRelease(down);
    if (up) CFRelease(up);
    /* Do not release `src` — it's cached. */
    return ok;
}

/* Post a synthetic shortcut: virtual keycode + modifier flags, both as
 * key-down + key-up. Used by SWITCHVT and friends — we want the target
 * app's real shortcut handler to fire, not a text-injection. */
static int
postShortcut(CGKeyCode vk, CGEventFlags flags) {
    static CGEventSourceRef src = NULL;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        /* Private source state: our flag-bearing events don't pollute the
         * global session modifier state. Avoids "stuck Cmd/Ctrl" symptoms
         * after a synthetic shortcut where the modifier appeared held to
         * subsequent physical keystrokes. */
        src = CGEventSourceCreate(kCGEventSourceStatePrivate);
    });
    if (!src) return 0;

    CGEventRef down = CGEventCreateKeyboardEvent(src, vk, true);
    CGEventRef up   = CGEventCreateKeyboardEvent(src, vk, false);
    int ok = 0;
    if (down && up) {
        CGEventSetFlags(down, flags);
        CGEventSetFlags(up, flags);
        CGEventPost(kCGHIDEventTap, down);
        CGEventPost(kCGHIDEventTap, up);
        ok = 1;
    }
    if (down) CFRelease(down);
    if (up) CFRelease(up);
    return ok;
}

/* After a tab/window switch the new frontmost window's text area is
 * usually NOT the focused element (focus may sit on the tab bar). brltty
 * then sends synthetic keystrokes that hit a non-text element and macOS
 * beeps. We schedule a delayed AX call that finds the new window's text
 * area and explicitly sets kAXFocusedAttribute=true on it. */
static void
focusFrontmostTextAreaAfterDelay(double delaySeconds) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                                 (int64_t)(delaySeconds * NSEC_PER_SEC)),
                   dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool {
            NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
            if (!app) return;
            pid_t pid = app.processIdentifier;
            AXUIElementRef appEl = AXUIElementCreateApplication(pid);
            AXUIElementRef window = copyElementAttribute(appEl, kAXFocusedWindowAttribute);

            /* Check if focus already landed on a text element naturally —
             * many apps re-focus the content on tab switch. If so, leave
             * it alone: setting AXFocused on a different element could
             * move focus to (e.g.) a search field. */
            AXUIElementRef focused = copyElementAttribute(appEl, kAXFocusedUIElementAttribute);
            int alreadyOnText = 0;
            if (focused) {
                AXUIElementRef container = findTextContainer(focused);
                if (container) {
                    alreadyOnText = 1;
                    CFRelease(container);
                }
                CFRelease(focused);
            }

            if (!alreadyOnText && window) {
                /* Pick the LARGEST TextArea — the shell occupies the bulk
                 * of the window, unlike a small toolbar search field. */
                AXUIElementRef textArea = findShellTextArea(window, 5);
                if (textArea) {
                    AXError err = AXUIElementSetAttributeValue(textArea,
                                                              kAXFocusedAttribute,
                                                              kCFBooleanTrue);
                    axDebugLog("focus-after-switch: set kAXFocused on largest text area, err=%d\n",
                               (int)err);
                    CFRelease(textArea);
                } else {
                    axDebugLog("focus-after-switch: no text area found in window\n");
                }
            } else if (alreadyOnText) {
                axDebugLog("focus-after-switch: already on text, no-op\n");
            }
            if (window) CFRelease(window);
            CFRelease(appEl);
        }
    });
}

int ax_post_shortcut_tab_next(void) {
    /* Ctrl+Tab — universal "next tab" shortcut (Terminal, Safari, Chrome,
     * iTerm, VS Code, ...). Uses kVK_Tab which is layout-independent,
     * unlike kVK_ANSI_*Bracket which points to a physical position on
     * ANSI layouts but a different character on AZERTY / non-US. */
    int ok = postShortcut(/*kVK_Tab*/0x30, kCGEventFlagMaskControl);
    if (ok) focusFrontmostTextAreaAfterDelay(0.12);
    return ok;
}

int ax_post_shortcut_tab_prev(void) {
    /* Ctrl+Shift+Tab — universal "previous tab". */
    int ok = postShortcut(/*kVK_Tab*/0x30,
                          kCGEventFlagMaskControl | kCGEventFlagMaskShift);
    if (ok) focusFrontmostTextAreaAfterDelay(0.12);
    return ok;
}

int ax_post_shortcut_tab_index(int n) {
    if (n < 1 || n > 9) return 0;
    /* Cmd+1 .. Cmd+9 — Terminal.app, Safari, Chrome (and many) use these
     * to jump to a specific tab by index. */
    static const CGKeyCode digitKeys[9] = {
        /*1*/0x12, /*2*/0x13, /*3*/0x14, /*4*/0x15, /*5*/0x17,
        /*6*/0x16, /*7*/0x1A, /*8*/0x1C, /*9*/0x19,
    };
    int ok = postShortcut(digitKeys[n - 1], kCGEventFlagMaskCommand);
    if (ok) focusFrontmostTextAreaAfterDelay(0.12);
    return ok;
}

int
ax_request_trust(void) {
    NSDictionary *opts = @{
        (__bridge NSString *)kAXTrustedCheckOptionPrompt: @YES
    };
    return AXIsProcessTrustedWithOptions((CFDictionaryRef)opts) ? 1 : 0;
}

size_t
ax_frontmost_bundle_id(char *out_buf, size_t out_len) {
    if (!out_buf || out_len == 0) return 0;
    out_buf[0] = '\0';

    // Query NSWorkspace directly each call. The earlier cached
    // implementation relied on NSWorkspaceDidActivateApplicationNotification
    // to invalidate the cache, but that notification needs an active
    // notification-aware run loop in this process — brltty is a C
    // daemon without NSApp, so the callback fired late or not at
    // all and describe() kept seeing the previous frontmost (visible
    // as "stale terminal content sticking around after Cmd+Tab").
    // NSWorkspace's frontmostApplication is a cheap XPC probe; calling
    // it once per brltty refresh cycle is well within budget.
    size_t written = 0;
    @autoreleasepool {
        NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
        const char *bid = app.bundleIdentifier.UTF8String;
        if (bid) {
            written = strnlen(bid, out_len - 1);
            memcpy(out_buf, bid, written);
            out_buf[written] = '\0';
        }
    }
    return written;
}

size_t
ax_fingerprint(char *out_buf, size_t out_len) {
    if (!out_buf || out_len == 0) return 0;
    size_t written = 0;

    @autoreleasepool {
        NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
        if (!app) {
            written = (size_t)snprintf(out_buf, out_len, "(no app)");
            return written;
        }
        pid_t pid = app.processIdentifier;
        AXUIElementRef appEl = AXUIElementCreateApplication(pid);
        AXUIElementRef window = copyElementAttribute(appEl, kAXFocusedWindowAttribute);
        AXUIElementRef focused = copyElementAttribute(appEl, kAXFocusedUIElementAttribute);

        NSString *winTitle = copyStringAttribute(window, kAXTitleAttribute) ?: @"";
        NSString *focusedTitle = copyStringAttribute(focused, kAXTitleAttribute) ?: @"";
        NSString *focusedValue = copyStringAttribute(focused, kAXValueAttribute) ?: @"";

        // The selected range fingerprint catches caret moves within the same
        // text body; without it, brltty would not refresh when only the
        // cursor position changes (e.g. typing inside a Terminal tab).
        // The visible range catches scrolling. We also sample the start and
        // end of the visible text content, because some apps (Terminal.app)
        // expose a fixed-size visible buffer whose length is constant even
        // as the displayed characters change.
        NSString *rangeStr = @"";
        NSString *visibleStr = @"";
        NSString *contentHash = @"";
        if (focused) {
            CFRange r;
            if (copyRangeAttribute(focused, kAXSelectedTextRangeAttribute, &r)) {
                rangeStr = [NSString stringWithFormat:@"%ld,%ld", (long)r.location, (long)r.length];
            }
            AXUIElementRef textContainer = findTextContainer(focused);
            if (textContainer) {
                CFRange v;
                if (copyRangeAttribute(textContainer, kAXVisibleCharacterRangeAttribute, &v)
                    && v.length > 0) {
                    visibleStr = [NSString stringWithFormat:@"%ld,%ld", (long)v.location, (long)v.length];

                    AXValueRef rangeRef = AXValueCreate(kAXValueCFRangeType, &v);
                    CFTypeRef sliceCF = NULL;
                    if (AXUIElementCopyParameterizedAttributeValue(
                          textContainer, kAXStringForRangeParameterizedAttribute,
                          rangeRef, &sliceCF) == kAXErrorSuccess && sliceCF
                          && CFGetTypeID(sliceCF) == CFStringGetTypeID()) {
                        NSString *visible = [(NSString *)sliceCF autorelease];
                        // Sample 32 chars from the start and 32 from the end
                        // to keep the fingerprint cheap while still reacting
                        // to typing and scrollback updates.
                        NSUInteger len = visible.length;
                        NSUInteger headLen = len > 32 ? 32 : len;
                        NSString *head = [visible substringToIndex:headLen];
                        NSString *tail = len > 32
                            ? [visible substringFromIndex:len - 32]
                            : @"";
                        contentHash = [NSString stringWithFormat:@"%lu:%@:%@",
                                          (unsigned long)len, head, tail];
                    } else if (sliceCF) {
                        CFRelease(sliceCF);
                    }
                }
                CFRelease(textContainer);
            }
        }

        NSString *fp = [NSString stringWithFormat:@"%d|%@|%@|%lu|%@|%@|%@",
                          pid, winTitle, focusedTitle,
                          (unsigned long)focusedValue.length,
                          rangeStr, visibleStr, contentHash];
        const char *cstr = [fp UTF8String];
        if (cstr) {
            written = strlcpy(out_buf, cstr, out_len);
            if (written >= out_len) written = out_len - 1;
        }
        if (window) CFRelease(window);
        if (focused) CFRelease(focused);
        CFRelease(appEl);
    }
    return written;
}

size_t
ax_snapshot_lines(char *out_buf, size_t out_len,
                  int *out_row, int *out_col) {
    if (out_row) *out_row = 0;
    if (out_col) *out_col = 0;
    if (!out_buf || out_len == 0) return 0;
    out_buf[0] = '\0';

    NSMutableString *result = [NSMutableString stringWithCapacity:512];

    @autoreleasepool {
        NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
        if (!app) {
            [result appendString:@"(no frontmost application)"];
        } else {
            pid_t pid = app.processIdentifier;
            AXUIElementRef appEl = AXUIElementCreateApplication(pid);

            NSString *appName = app.localizedName ?: @"(unknown)";
            AXUIElementRef window = copyElementAttribute(appEl, kAXFocusedWindowAttribute);
            AXUIElementRef focused = copyElementAttribute(appEl, kAXFocusedUIElementAttribute);

            // If the focused element (or a nearby ancestor) is a text
            // container, render its content directly — no app/window header
            // — so the cursor coordinates apply to what brltty displays.
            AXUIElementRef textContainer = findTextContainer(focused);
            if (textContainer) {
                // Terminal.app and other text-area apps expose the full
                // scrollback through AXValue (potentially megabytes of text).
                // What we actually want is the currently-visible window, which
                // Terminal advertises via AXVisibleCharacterRange. We fetch
                // only that slice via the parameterized AXStringForRange.
                NSString *value = nil;
                CFRange visibleRange = {0, 0};
                int haveVisible = copyRangeAttribute(
                    textContainer, kAXVisibleCharacterRangeAttribute, &visibleRange);

                if (haveVisible && visibleRange.length > 0) {
                    AXValueRef rangeRef = AXValueCreate(kAXValueCFRangeType, &visibleRange);
                    CFTypeRef sliceCF = NULL;
                    AXError err = AXUIElementCopyParameterizedAttributeValue(
                        textContainer, kAXStringForRangeParameterizedAttribute,
                        rangeRef, &sliceCF);
                    CFRelease(rangeRef);
                    if (err == kAXErrorSuccess && sliceCF
                        && CFGetTypeID(sliceCF) == CFStringGetTypeID()) {
                        value = [(NSString *)sliceCF autorelease];
                    } else if (sliceCF) {
                        CFRelease(sliceCF);
                    }
                }

                // Fallback: full AXValue (single-line text fields, or apps
                // that don't expose a visible character range).
                if (!value) {
                    value = copyStringAttribute(textContainer, kAXValueAttribute);
                    haveVisible = 0;
                }
                if (!value) value = @"";
                [result appendString:value];

                if (out_row && out_col) {
                    int row = 0, col = 0;
                    if (computeCursorPosition(textContainer, &row, &col)) {
                        // When we rendered only the visible slice, the
                        // cursor line returned by AXLineForIndex is relative
                        // to the full text. Translate it to be relative to
                        // the slice we sent to brltty.
                        if (haveVisible) {
                            int firstLine = 0;
                            CFIndex visStart = visibleRange.location;
                            CFNumberRef idxNum = CFNumberCreate(NULL, kCFNumberCFIndexType, &visStart);
                            CFTypeRef firstLineCF = NULL;
                            if (AXUIElementCopyParameterizedAttributeValue(
                                  textContainer, kAXLineForIndexParameterizedAttribute,
                                  idxNum, &firstLineCF) == kAXErrorSuccess && firstLineCF) {
                                CFNumberGetValue((CFNumberRef)firstLineCF,
                                                 kCFNumberIntType, &firstLine);
                                CFRelease(firstLineCF);
                            }
                            CFRelease(idxNum);
                            row -= firstLine;
                            if (row < 0) row = 0;
                        }
                        *out_row = row;
                        *out_col = col;
                    }
                }
                CFRelease(textContainer);
            } else {
                // Non-text focus: surface the app/window header + focused label.
                NSString *winTitle = copyStringAttribute(window, kAXTitleAttribute);
                if (winTitle.length) {
                    [result appendFormat:@"%@ | %@", appName, winTitle];
                } else {
                    [result appendString:appName];
                }
                if (focused) {
                    NSString *role = copyStringAttribute(focused, kAXRoleAttribute);
                    NSString *label = copyStringAttribute(focused, kAXTitleAttribute);
                    if (!label) label = copyStringAttribute(focused, kAXDescriptionAttribute);
                    if (role || label) {
                        [result appendFormat:@"\n[%@] %@", role ?: @"?", label ?: @""];
                    }
                    NSString *value = copyStringAttribute(focused, kAXValueAttribute);
                    if (value.length) {
                        [result appendFormat:@"\n%@", value];
                    }
                }
            }

            if (focused) CFRelease(focused);
            if (window) CFRelease(window);
            CFRelease(appEl);
        }

        const char *cstr = [result UTF8String];
        if (cstr) {
            size_t n = strlcpy(out_buf, cstr, out_len);
            if (n >= out_len) n = out_len - 1;
            // Sample the last 80 chars so we can see typed text appear.
            const char *tail = n > 80 ? out_buf + n - 80 : out_buf;
            axDebugLog("snapshot len=%zu row=%d col=%d tail='%s'\n",
                       n, out_row ? *out_row : -1, out_col ? *out_col : -1, tail);
            return n;
        }
    }
    return 0;
}
