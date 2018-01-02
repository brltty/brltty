/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>

#include "log.h"
#include "strfmt.h"
#include "ktb.h"
#include "ktb_internal.h"
#include "ktb_inspect.h"

static
STR_BEGIN_FORMATTER(formatKeyCombination, KeyTable *table, const KeyCombination *combination)
  char delimiter = 0;

  for (unsigned char index=0; index<combination->modifierCount; index+=1) {
    if (delimiter) {
      STR_PRINTF("%c", delimiter);
    } else {
      delimiter = '+';
    }

    STR_FORMAT(formatKeyName, table,
               &combination->modifierKeys[combination->modifierPositions[index]]);
  }

  if (combination->flags & KCF_IMMEDIATE_KEY) {
    if (delimiter) STR_PRINTF("%c", delimiter);
    STR_PRINTF("!");

    STR_FORMAT(formatKeyName, table, &combination->immediateKey);
  }
STR_END_FORMATTER

typedef struct {
  KeyTable *table;
  const KeyContext *ctx;
  const char *path;
} KeyTableAuditorParameters;

#define KEY_TABLE_AUDITOR(name) int name (const KeyTableAuditorParameters *kta)
typedef KEY_TABLE_AUDITOR(KeyTableAuditor);

static void
reportKeyTableAudit (const char *audit) {
  logMessage(LOG_WARNING, "%s", audit);
}

static
STR_BEGIN_FORMATTER(
  formatKeyTableAuditPrefix,
  const KeyTableAuditorParameters *kta,
  const char *problem
)
  if (kta->path) STR_PRINTF("%s: ", kta->path);
  STR_PRINTF("%s", problem);
  if (kta->ctx) STR_PRINTF(": %" PRIws, kta->ctx->name);
STR_END_FORMATTER

static KEY_TABLE_AUDITOR(reportKeyContextProblems) {
  int ok = 1;

  if (kta->ctx->name && !kta->ctx->isSpecial) {
    const char *problem = NULL;

    if (!kta->ctx->isDefined) {
      problem = "undefined context";
    } else if (!kta->ctx->isReferenced) {
      problem = "unreferenced context";
    } else if (!(kta->ctx->keyBindings.count ||
                 kta->ctx->mappedKeys.count ||
                 kta->ctx->mappedKeys.superimpose ||
                 kta->ctx->hotkeys.count)) {
      problem = "empty context";
    }

    if (problem) {
      ok = 0;

      char audit[0X100];
      STR_BEGIN(audit, sizeof(audit));

      STR_FORMAT(formatKeyTableAuditPrefix, kta, problem);

      STR_END;
      reportKeyTableAudit(audit);
    }
  }

  return ok;
}

static KEY_TABLE_AUDITOR(reportDuplicateKeyBindings) {
  int ok = 1;
  const KeyBinding *const *binding = kta->ctx->keyBindings.sorted;
  const KeyBinding *const *end = binding + kta->ctx->keyBindings.count;
  int reported = 0;

  while (++binding < end) {
    const KeyBinding *current = *binding;
    const KeyBinding *previous = *(binding - 1);

    if (compareKeyBindings(current, previous) != 0) {
      reported = 0;
    } else if (!reported) {
      reported = 1;
      ok = 0;

      char audit[0X100];
      STR_BEGIN(audit, sizeof(audit));

      STR_FORMAT(formatKeyTableAuditPrefix, kta, "duplicate key binding");
      STR_PRINTF(": ");
      STR_FORMAT(formatKeyCombination, kta->table, &current->keyCombination);

      STR_END;
      reportKeyTableAudit(audit);
    }
  }

  return ok;
}

static void
reportKeyProblem (const KeyTableAuditorParameters *kta, const KeyValue *key, const char *problem) {
  char audit[0X100];
  STR_BEGIN(audit, sizeof(audit));

  STR_FORMAT(formatKeyTableAuditPrefix, kta, problem);
  STR_PRINTF(": ");
  STR_FORMAT(formatKeyName, kta->table, key);

  STR_END;
  reportKeyTableAudit(audit);
}

static KEY_TABLE_AUDITOR(reportDuplicateHotkeys) {
  int ok = 1;
  const HotkeyEntry *const *hotkey = kta->ctx->hotkeys.sorted;
  const HotkeyEntry *const *end = hotkey + kta->ctx->hotkeys.count;
  int reported = 0;

  while (++hotkey < end) {
    const HotkeyEntry *current = *hotkey;
    const HotkeyEntry *previous = *(hotkey - 1);

    if (compareKeyValues(&current->keyValue, &previous->keyValue) != 0) {
      reported = 0;
    } else if (!reported) {
      reported = 1;
      ok = 0;

      reportKeyProblem(kta, &current->keyValue, "duplicate hotkey");
    }
  }

  return ok;
}

static KEY_TABLE_AUDITOR(reportDuplicateMappedKeys) {
  int ok = 1;
  const MappedKeyEntry *const *map = kta->ctx->mappedKeys.sorted;
  const MappedKeyEntry *const *end = map + kta->ctx->mappedKeys.count;
  int reported = 0;

  while (++map < end) {
    const MappedKeyEntry *current = *map;
    const MappedKeyEntry *previous = *(map - 1);

    if (compareKeyValues(&current->keyValue, &previous->keyValue) != 0) {
      reported = 0;
    } else if (!reported) {
      reported = 1;
      ok = 0;

      reportKeyProblem(kta, &current->keyValue, "duplicate mapped key");
    }
  }

  return ok;
}

int
auditKeyTable (KeyTable *table, const char *path) {
  int ok = 1;

  for (unsigned int context=0; context<table->keyContexts.count; context+=1) {
    const KeyContext *ctx = getKeyContext(table, context);

    if (ctx) {
      static KeyTableAuditor *const auditors[] = {
        reportKeyContextProblems,
        reportDuplicateKeyBindings,
        reportDuplicateHotkeys,
        reportDuplicateMappedKeys,
        NULL
      };

      const KeyTableAuditorParameters kta = {
        .table = table,
        .ctx = ctx,
        .path = path
      };

      for (
        KeyTableAuditor *const *auditor=auditors;
        *auditor!=NULL; auditor+=1
      ) {
        if (!(*auditor)(&kta)) ok = 0;
      }
    }
  }

  return ok;
}
