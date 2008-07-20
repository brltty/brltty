/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#include <stdio.h>
#include <string.h>
 
#include "misc.h"
#include "datafile.h"
#include "ctb.h"
#include "ctb_internal.h"
#include "brl.h"

static ContractionTableHeader *tableHeader;
static ContractionTableOffset tableSize;
static ContractionTableOffset tableUsed;

static ContractionTableCharacter *characterTable = NULL;
static int characterTableSize = 0;
static int characterEntryCount = 0;

static const wchar_t *const characterClassNames[] = {
  WS_C("space"),
  WS_C("letter"),
  WS_C("digit"),
  WS_C("punctuation"),
  WS_C("uppercase"),
  WS_C("lowercase"),
  NULL
};
struct CharacterClass {
  struct CharacterClass *next;
  ContractionTableCharacterAttributes attribute;
  BYTE length;
  wchar_t name[1];
};
static struct CharacterClass *characterClasses;
static ContractionTableCharacterAttributes characterClassAttribute;

static const wchar_t *const opcodeNames[CTO_None] = {
  WS_C("include"),

  WS_C("capsign"),
  WS_C("begcaps"),
  WS_C("endcaps"),

  WS_C("letsign"),
  WS_C("numsign"),

  WS_C("literal"),
  WS_C("always"),
  WS_C("repeatable"),

  WS_C("largesign"),
  WS_C("lastlargesign"),
  WS_C("word"),
  WS_C("joinword"),
  WS_C("lowword"),
  WS_C("contraction"),

  WS_C("sufword"),
  WS_C("prfword"),
  WS_C("begword"),
  WS_C("begmidword"),
  WS_C("midword"),
  WS_C("midendword"),
  WS_C("endword"),

  WS_C("prepunc"),
  WS_C("postpunc"),

  WS_C("begnum"),
  WS_C("midnum"),
  WS_C("endnum"),

  WS_C("class"),
  WS_C("after"),
  WS_C("before")
};
static unsigned char opcodeLengths[CTO_None] = {0};

static int
allocateBytes (DataFile *file, ContractionTableOffset *offset, int count, int alignment) {
  int size = (tableUsed = (tableUsed + (alignment - 1)) / alignment * alignment) + count;
  if (size > tableSize) {
    void *table = realloc(tableHeader, size|=0XFFF);
    if (!table) {
      reportDataError(file, "Not enough memory for contraction table.");
      return 0;
    }
    memset(((BYTE *)table)+tableSize, 0, size-tableSize);
    tableHeader = (ContractionTableHeader *) table;
    tableSize = size;
  }
  *offset = tableUsed;
  tableUsed += count;
  return 1;
}

static int
saveBytes (DataFile *file, ContractionTableOffset *offset, const void *bytes, int count, int alignment) {
  if (allocateBytes(file, offset, count, alignment)) {
    BYTE *address = getContractionTableItem(tableHeader, *offset);
    memcpy(address, bytes, count);
    return 1;
  }
  return 0;
}

static int
saveSequence (DataFile *file, ContractionTableOffset *offset, const ByteOperand *sequence) {
  if (allocateBytes(file, offset, sequence->length+1, __alignof__(BYTE))) {
    BYTE *address = getContractionTableItem(tableHeader, *offset);
    memcpy(address+1, sequence->bytes, (*address = sequence->length));
    return 1;
  }
  return 0;
}

static ContractionTableCharacter *
getCharacterEntry (wchar_t character) {
  int first = 0;
  int last = characterEntryCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    ContractionTableCharacter *entry = &characterTable[current];

    if (entry->value < character) {
      first = current + 1;
    } else if (entry->value > character) {
      last = current - 1;
    } else {
      return entry;
    }
  }

  if (characterEntryCount == characterTableSize) {
    int newSize = characterTableSize;
    newSize = newSize? newSize<<1: 0X80;

    {
      ContractionTableCharacter *newTable = realloc(characterTable, (newSize * sizeof(*newTable)));
      if (!newTable) return NULL;

      characterTable = newTable;
      characterTableSize = newSize;
    }
  }

  memmove(&characterTable[first+1],
          &characterTable[first],
          (characterEntryCount - first) * sizeof(*characterTable));
  ++characterEntryCount;

  {
    ContractionTableCharacter *entry = &characterTable[first];
    memset(entry, 0, sizeof(*entry));
    entry->value = character;
    return entry;
  }
}

static int
saveCharacterTable (void) {
  ContractionTableOffset offset;
  if (!characterEntryCount) return 1;
  if (!saveBytes(NULL, &offset, characterTable,
                 (tableHeader->characterCount = characterEntryCount) * sizeof(characterTable[0]),
                 __alignof__(characterTable[0])))
    return 0;

  tableHeader->characters = offset;
  return 1;
}

static ContractionTableRule *
addRule (
  DataFile *file,
  ContractionTableOpcode opcode,
  CharacterOperand *find,
  ByteOperand *replace,
  ContractionTableCharacterAttributes after,
  ContractionTableCharacterAttributes before
) {
  ContractionTableOffset ruleOffset;
  int ruleSize = sizeof(ContractionTableRule) - sizeof(find->characters[0]);
  if (find) ruleSize += find->length * sizeof(find->characters[0]);
  if (replace) ruleSize += replace->length;

  if (allocateBytes(file, &ruleOffset, ruleSize, __alignof__(ContractionTableRule))) {
    ContractionTableRule *newRule = getContractionTableItem(tableHeader, ruleOffset);

    newRule->opcode = opcode;
    newRule->after = after;
    newRule->before = before;

    if (find)
      wmemcpy(&newRule->findrep[0], &find->characters[0],
              (newRule->findlen = find->length));
    else
      newRule->findlen = 0;

    if (replace)
      memcpy(&newRule->findrep[newRule->findlen], &replace->bytes[0],
             (newRule->replen = replace->length));
    else
      newRule->replen = 0;

    /*link new rule into table.*/
    {
      ContractionTableOffset *offsetAddress;

      if (newRule->findlen == 1) {
        ContractionTableCharacter *character = getCharacterEntry(newRule->findrep[0]);
        if (!character) return NULL;
        if (newRule->opcode == CTO_Always) character->always = ruleOffset;
        offsetAddress = &character->rules;
      } else {
        offsetAddress = &tableHeader->rules[CTH(newRule->findrep)];
      }

      while (*offsetAddress) {
        ContractionTableRule *currentRule = getContractionTableItem(tableHeader, *offsetAddress);
        if (newRule->findlen > currentRule->findlen) break;
        if (newRule->findlen == currentRule->findlen) {
          if ((currentRule->opcode == CTO_Always) && (newRule->opcode != CTO_Always)) break;
        }
        offsetAddress = &currentRule->next;
      }

      newRule->next = *offsetAddress;
      *offsetAddress = ruleOffset;
    }

    return newRule;
  }
  return NULL;
}

static const struct CharacterClass *
findCharacterClass (const DataOperand *operand) {
  const struct CharacterClass *class = characterClasses;

  while (class) {
    if (operand->length == class->length)
      if (wmemcmp(operand->address, class->name, operand->length) == 0)
        return class;

    class = class->next;
  }

  return NULL;
}

static struct CharacterClass *
addCharacterClass (DataFile *file, const wchar_t *name, int length) {
  struct CharacterClass *class;

  if (characterClassAttribute) {
    if ((class = malloc(sizeof(*class) + ((length - 1) * sizeof(class->name[0]))))) {
      memset(class, 0, sizeof(*class));
      wmemcpy(class->name, name, (class->length = length));

      class->attribute = characterClassAttribute;
      characterClassAttribute <<= 1;

      class->next = characterClasses;
      characterClasses = class;
      return class;
    }
  }

  reportDataError(file, "character class table overflow: %.*" PRIws, length, name);
  return NULL;
}

static void
deallocateCharacterClasses (void) {
  while (characterClasses) {
    struct CharacterClass *class = characterClasses;
    characterClasses = characterClasses->next;
    free(class);
  }
}

static int
allocateCharacterClasses (void) {
  const wchar_t *const *name = characterClassNames;
  characterClasses = NULL;
  characterClassAttribute = 1;
  while (*name) {
    if (!addCharacterClass(NULL, *name, wcslen(*name))) {
      deallocateCharacterClasses();
      return 0;
    }
    ++name;
  }
  return 1;
}

static ContractionTableOpcode
getOpcode (DataFile *file, const DataOperand *operand) {
  ContractionTableOpcode opcode;

  for (opcode=0; opcode<CTO_None; opcode+=1)
    if (operand->length == opcodeLengths[opcode])
      if (wmemcmp(operand->address, opcodeNames[opcode], operand->length) == 0)
        return opcode;

  reportDataError(file, "opcode not defined: %.*" PRIws, operand->length, operand->address);
  return CTO_None;
}

static int
parseDots (DataFile *file, ByteOperand *cells, const DataOperand *operand) {
  BYTE cell = 0;		/*assembly place for dots */
  int count = 0;		/*loop counters */
  int index;		/*loop counters */
  int start = 0;

  for (index=0; index<operand->length; index+=1) {
    int started = index != start;
    wchar_t character = operand->address[index];

    switch (character) {
      {
        int dot;

      case WC_C('1'):
        dot = BRL_DOT1;
        goto haveDot;

      case WC_C('2'):
        dot = BRL_DOT2;
        goto haveDot;

      case WC_C('3'):
        dot = BRL_DOT3;
        goto haveDot;

      case WC_C('4'):
        dot = BRL_DOT4;
        goto haveDot;

      case WC_C('5'):
        dot = BRL_DOT5;
        goto haveDot;

      case WC_C('6'):
        dot = BRL_DOT6;
        goto haveDot;

      case WC_C('7'):
        dot = BRL_DOT7;
        goto haveDot;

      case WC_C('8'):
        dot = BRL_DOT8;
      haveDot:

        if (started && !cell) goto invalid;

        if (cell & dot) {
          reportDataError(file, "dot specified more than once: %.1" PRIws, &character);
          return 0;
        }

        cell |= dot;
        break;
      }

      case WC_C('0'):			/*blank */
        if (started) goto invalid;
        break;

      case WC_C('-'):			/*got all dots for this cell */
        if (!started) {
          reportDataError(file, "missing cell specification: %.*" PRIws,
                          operand->length-index, &operand->address[index]);
          return 0;
        }

        cells->bytes[count++] = cell;
        cell = 0;
        start = index + 1;
        break;

      default:
      invalid:
        reportDataError(file, "invalid dot number: %.1" PRIws, &character);
        return 0;
    }
  }

  if (index == start) {
    reportDataError(file, "missing cell specification");
    return 0;
  }

  cells->bytes[count++] = cell;		/*last cell */
  cells->length = count;
  return 1;
}				/*end of function parseDots */

static int
getFindText (DataFile *file, CharacterOperand *find, DataOperand *operand) {
  return getCharacterOperand(file, find, operand, "find text");
}

static int
getReplacePattern (DataFile *file, ByteOperand *replace, DataOperand *operand) {
  if (getDataOperand(file, operand, "replacement pattern")) {
    if ((operand->length == 1) && (*operand->address == WC_C('='))) {
      replace->length = 0;
      return 1;
    }

    if (parseDots(file, replace, operand)) return 1;
  }

  return 0;
}

static int
getCharacterClass (DataFile *file, const struct CharacterClass **class, DataOperand *operand) {
  if (getDataOperand(file, operand, "character class name")) {
    if ((*class = findCharacterClass(operand))) return 1;
    reportDataError(file, "character class not defined: %.*" PRIws, operand->length, operand->address);
  }
  return 0;
}

static int
parseContractionLine (DataFile *file, DataOperand *operand, void *data) {
  int ok = 0;
  ContractionTableOpcode opcode;
  ContractionTableCharacterAttributes after = 0;
  ContractionTableCharacterAttributes before = 0;

doOpcode:
  switch ((opcode = getOpcode(file, operand))) {
    case CTO_None:
      break;

    case CTO_IncludeFile: {
      CharacterOperand path;
      if (getCharacterOperand(file, &path, operand, "include file path"))
        if (!includeDataFile(file, &path))
          goto failure;
      break;
    }

    case CTO_Always:
    case CTO_LargeSign:
    case CTO_LastLargeSign:
    case CTO_WholeWord:
    case CTO_JoinedWord:
    case CTO_LowWord:
    case CTO_SuffixableWord:
    case CTO_PrefixableWord:
    case CTO_BegWord:
    case CTO_BegMidWord:
    case CTO_MidWord:
    case CTO_MidEndWord:
    case CTO_EndWord:
    case CTO_PrePunc:
    case CTO_PostPunc:
    case CTO_BegNum:
    case CTO_MidNum:
    case CTO_EndNum:
    case CTO_Repeatable: {
      CharacterOperand find;
      ByteOperand replace;
      if (getFindText(file, &find, operand))
        if (getReplacePattern(file, &replace, operand))
          if (!addRule(file, opcode, &find, &replace, after, before))
            goto failure;
      break;
    }

    case CTO_Contraction:
    case CTO_Literal: {
      CharacterOperand find;
      if (getFindText(file, &find, operand))
        if (!addRule(file, opcode, &find, NULL, after, before))
          goto failure;
      break;
    }

    case CTO_CapitalSign: {
      ByteOperand cells;
      if (getDataOperand(file, operand, "capital sign"))
        if (parseDots(file, &cells, operand)) {
          ContractionTableOffset offset;
          if (!saveSequence(file, &offset, &cells)) goto failure;
          tableHeader->capitalSign = offset;
        }
      break;
    }

    case CTO_BeginCapitalSign: {
      ByteOperand cells;
      if (getDataOperand(file, operand, "begin capital sign"))
        if (parseDots(file, &cells, operand)) {
          ContractionTableOffset offset;
          if (!saveSequence(file, &offset, &cells)) goto failure;
          tableHeader->beginCapitalSign = offset;
        }
      break;
    }

    case CTO_EndCapitalSign: {
      ByteOperand cells;
      if (getDataOperand(file, operand, "end capital sign"))
        if (parseDots(file, &cells, operand)) {
          ContractionTableOffset offset;
          if (!saveSequence(file, &offset, &cells)) goto failure;
          tableHeader->endCapitalSign = offset;
        }
      break;
    }

    case CTO_EnglishLetterSign: {
      ByteOperand cells;
      if (getDataOperand(file, operand, "letter sign"))
        if (parseDots(file, &cells, operand)) {
          ContractionTableOffset offset;
          if (!saveSequence(file, &offset, &cells)) goto failure;
          tableHeader->englishLetterSign = offset;
        }
      break;
    }

    case CTO_NumberSign: {
      ByteOperand cells;
      if (getDataOperand(file, operand, "number sign"))
        if (parseDots(file, &cells, operand)) {
          ContractionTableOffset offset;
          if (!saveSequence(file, &offset, &cells)) goto failure;
          tableHeader->numberSign = offset;
        }
      break;
    }

    case CTO_Class: {
      const struct CharacterClass *class;
      CharacterOperand characters;

      if (getDataOperand(file, operand, "character class name")) {
        if ((class = findCharacterClass(operand))) {
          reportDataError(file, "character class already defined: %.*" PRIws,
                          operand->length, operand->address);
        } else if ((class = addCharacterClass(file, operand->address, operand->length))) {
          if (getCharacterOperand(file, &characters, operand, "characters")) {
            int index;
            for (index=0; index<characters.length; ++index) {
              wchar_t character = characters.characters[index];
              ContractionTableCharacter *entry = getCharacterEntry(character);
              if (!entry) goto failure;
              entry->attributes |= class->attribute;
            }
          }
        }
      }
      break;
    }

    {
      ContractionTableCharacterAttributes *attributes;
      const struct CharacterClass *class;

    case CTO_After:
      attributes = &after;
      goto doClass;
    case CTO_Before:
      attributes = &before;
    doClass:

      if (getCharacterClass(file, &class, operand)) {
        *attributes |= class->attribute;
        if (getDataOperand(file, operand, "opcode")) goto doOpcode;
      }
      break;
    }

    default:
      reportDataError(file, "unimplemented opcode: %.*" PRIws, operand->length, operand->address);
      break;
  }				/*end of loop for processing tableStream */
  ok = 1;

failure:
  return ok;
}

ContractionTable *
compileContractionTable (const char *fileName) {
  int ok = 0;
  ContractionTableOffset headerOffset;

  tableHeader = NULL;
  tableSize = 0;
  tableUsed = 0;

  characterTable = NULL;
  characterTableSize = 0;
  characterEntryCount = 0;

  if (!opcodeLengths[0]) {
    ContractionTableOpcode opcode;
    for (opcode=0; opcode<CTO_None; ++opcode)
      opcodeLengths[opcode] = wcslen(opcodeNames[opcode]);
  }

  if (allocateBytes(NULL, &headerOffset, sizeof(*tableHeader), __alignof__(*tableHeader))) {
    if (headerOffset == 0) {
      if (allocateCharacterClasses()) {
        if (processDataFile(fileName, parseContractionLine, NULL)) {
          if (saveCharacterTable()) {
            ok = 1;
          }
        }

        deallocateCharacterClasses();
      }
    } else {
      reportDataError(NULL, "contraction table header not allocated at offset 0.");
    }
  }

  if (characterTable) free(characterTable);

  if (ok) {
    ContractionTable *table;

    if ((table = malloc(sizeof(*table)))) {
      table->header = tableHeader;
      table->characters = NULL;
      table->charactersSize = 0;
      table->characterCount = 0;
      return table;
    }
  }

  if (tableHeader) free(tableHeader);
  return NULL;
}

int
destroyContractionTable (ContractionTable *contractionTable) {
  if (contractionTable->characters) free(contractionTable->characters);
  if (contractionTable->header) free(contractionTable->header);
  free(contractionTable);
  return 1;
}

void
fixContractionTablePath (char **path) {
  fixPath(path, CONTRACTION_TABLE_EXTENSION, CONTRACTION_TABLE_PREFIX);
}
