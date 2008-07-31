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
#include "ctb.h"
#include "ctb_internal.h"
#include "datafile.h"
#include "dataarea.h"
#include "brldots.h"

typedef struct {
  unsigned char length;
  unsigned char bytes[0XFF];
} ByteOperand;

static DataArea *dataArea;

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
  WS_C("before"),

  WS_C("include")
};
static unsigned char opcodeLengths[CTO_None] = {0};

static inline ContractionTableHeader *
getContractionTableHeader (void) {
  return getDataItem(dataArea, 0);
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
  DataOffset offset;
  if (!characterEntryCount) return 1;
  if (!saveDataItem(dataArea, &offset, characterTable,
                    characterEntryCount * sizeof(characterTable[0]),
                    __alignof__(characterTable[0])))
    return 0;

  {
    ContractionTableHeader *header = getContractionTableHeader();
    header->characters = offset;
    header->characterCount = characterEntryCount;
  }

  return 1;
}

static ContractionTableRule *
addRule (
  DataFile *file,
  ContractionTableOpcode opcode,
  DataString *find,
  ByteOperand *replace,
  ContractionTableCharacterAttributes after,
  ContractionTableCharacterAttributes before
) {
  DataOffset ruleOffset;
  int ruleSize = sizeof(ContractionTableRule) - sizeof(find->characters[0]);
  if (find) ruleSize += find->length * sizeof(find->characters[0]);
  if (replace) ruleSize += replace->length;

  if (allocateDataItem(dataArea, &ruleOffset, ruleSize, __alignof__(ContractionTableRule))) {
    ContractionTableRule *newRule = getDataItem(dataArea, ruleOffset);

    newRule->opcode = opcode;
    newRule->after = after;
    newRule->before = before;

    if (find) {
      wmemcpy(&newRule->findrep[0], &find->characters[0],
              (newRule->findlen = find->length));
    } else {
      newRule->findlen = 0;
    }

    if (replace) {
      memcpy(&newRule->findrep[newRule->findlen], &replace->bytes[0],
             (newRule->replen = replace->length));
    } else {
      newRule->replen = 0;
    }

    /*link new rule into table.*/
    {
      ContractionTableOffset *offsetAddress;

      if (newRule->findlen == 1) {
        ContractionTableCharacter *character = getCharacterEntry(newRule->findrep[0]);
        if (!character) return NULL;
        if (newRule->opcode == CTO_Always) character->always = ruleOffset;
        offsetAddress = &character->rules;
      } else {
        offsetAddress = &getContractionTableHeader()->rules[CTH(newRule->findrep)];
      }

      while (*offsetAddress) {
        ContractionTableRule *currentRule = getDataItem(dataArea, *offsetAddress);

        if (newRule->findlen > currentRule->findlen) break;

        if (newRule->findlen == currentRule->findlen)
          if ((currentRule->opcode == CTO_Always) && (newRule->opcode != CTO_Always))
            break;

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
findCharacterClass (const wchar_t *name, int length) {
  const struct CharacterClass *class = characterClasses;

  while (class) {
    if (length == class->length)
      if (wmemcmp(name, class->name, length) == 0)
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

static int
getCharacterClass (DataFile *file, const struct CharacterClass **class) {
  DataOperand operand;

  if (getDataOperand(file, &operand, "character class name")) {
    if ((*class = findCharacterClass(operand.characters, operand.length))) return 1;
    reportDataError(file, "character class not defined: %.*" PRIws, operand.length, operand.characters);
  }

  return 0;
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
getOpcode (DataFile *file) {
  DataOperand operand;

  if (getDataOperand(file, &operand, "opcode")) {
    ContractionTableOpcode opcode;

    for (opcode=0; opcode<CTO_None; opcode+=1)
      if (operand.length == opcodeLengths[opcode])
        if (wmemcmp(operand.characters, opcodeNames[opcode], operand.length) == 0)
          return opcode;

    reportDataError(file, "opcode not defined: %.*" PRIws, operand.length, operand.characters);
  }

  return CTO_None;
}

static int
saveCellsOperand (DataFile *file, DataOffset *offset, const ByteOperand *sequence) {
  if (allocateDataItem(dataArea, offset, sequence->length+1, __alignof__(BYTE))) {
    BYTE *address = getDataItem(dataArea, *offset);
    memcpy(address+1, sequence->bytes, (*address = sequence->length));
    return 1;
  }
  return 0;
}

static int
parseCellsOperand (DataFile *file, ByteOperand *cells, const wchar_t *characters, int length) {
  BYTE cell = 0;
  int start = 0;
  int index;

  cells->length = 0;

  for (index=0; index<length; index+=1) {
    int started = index != start;
    wchar_t character = characters[index];

    switch (character) {
      {
        int dot;

      case WC_C('1'):
        dot = BRL_DOT1;
        goto doDot;

      case WC_C('2'):
        dot = BRL_DOT2;
        goto doDot;

      case WC_C('3'):
        dot = BRL_DOT3;
        goto doDot;

      case WC_C('4'):
        dot = BRL_DOT4;
        goto doDot;

      case WC_C('5'):
        dot = BRL_DOT5;
        goto doDot;

      case WC_C('6'):
        dot = BRL_DOT6;
        goto doDot;

      case WC_C('7'):
        dot = BRL_DOT7;
        goto doDot;

      case WC_C('8'):
        dot = BRL_DOT8;
        goto doDot;

      doDot:
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
                          length-index, &characters[index]);
          return 0;
        }

        cells->bytes[cells->length++] = cell;

        if (cells->length == ARRAY_COUNT(cells->bytes)) {
          reportDataError(file, "cells operand too long");
          return 0;
        }

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

  cells->bytes[cells->length++] = cell;		/*last cell */
  return 1;
}

static int
getCellsOperand (DataFile *file, ByteOperand *cells, const char *description) {
  DataOperand operand;

  if (getDataOperand(file, &operand, description))
    if (parseCellsOperand(file, cells, operand.characters, operand.length))
      return 1;

  return 0;
}

static int
getReplacePattern (DataFile *file, ByteOperand *replace) {
  DataOperand operand;

  if (getDataOperand(file, &operand, "replacement pattern")) {
    if ((operand.length == 1) && (operand.characters[0] == WC_C('='))) {
      replace->length = 0;
      return 1;
    }

    if (parseCellsOperand(file, replace, operand.characters, operand.length)) return 1;
  }

  return 0;
}

static int
getFindText (DataFile *file, DataString *find) {
  return getDataString(file, find, "find text");
}

static int
processContractionTableLine (DataFile *file, void *data) {
  ContractionTableCharacterAttributes after = 0;
  ContractionTableCharacterAttributes before = 0;

  while (1) {
    ContractionTableOpcode opcode;

    switch ((opcode = getOpcode(file))) {
      case CTO_None:
        break;

      case CTO_IncludeFile:
        if (!processIncludeOperands(file, data)) return 0;
        break;

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
        DataString find;
        ByteOperand replace;
        if (getFindText(file, &find))
          if (getReplacePattern(file, &replace))
            if (!addRule(file, opcode, &find, &replace, after, before))
              return 0;
        break;
      }

      case CTO_Contraction:
      case CTO_Literal: {
        DataString find;
        if (getFindText(file, &find))
          if (!addRule(file, opcode, &find, NULL, after, before))
            return 0;
        break;
      }

      case CTO_CapitalSign: {
        ByteOperand cells;
        if (getCellsOperand(file, &cells, "capital sign")) {
          DataOffset offset;
          if (!saveCellsOperand(file, &offset, &cells)) return 0;
          getContractionTableHeader()->capitalSign = offset;
        }
        break;
      }

      case CTO_BeginCapitalSign: {
        ByteOperand cells;
        if (getCellsOperand(file, &cells, "begin capital sign")) {
          DataOffset offset;
          if (!saveCellsOperand(file, &offset, &cells)) return 0;
          getContractionTableHeader()->beginCapitalSign = offset;
        }
        break;
      }

      case CTO_EndCapitalSign: {
        ByteOperand cells;
        if (getCellsOperand(file, &cells, "end capital sign")) {
          DataOffset offset;
          if (!saveCellsOperand(file, &offset, &cells)) return 0;
          getContractionTableHeader()->endCapitalSign = offset;
        }
        break;
      }

      case CTO_EnglishLetterSign: {
        ByteOperand cells;
        if (getCellsOperand(file, &cells, "letter sign")) {
          DataOffset offset;
          if (!saveCellsOperand(file, &offset, &cells)) return 0;
          getContractionTableHeader()->englishLetterSign = offset;
        }
        break;
      }

      case CTO_NumberSign: {
        ByteOperand cells;
        if (getCellsOperand(file, &cells, "number sign")) {
          DataOffset offset;
          if (!saveCellsOperand(file, &offset, &cells)) return 0;
          getContractionTableHeader()->numberSign = offset;
        }
        break;
      }

      case CTO_Class: {
        DataOperand name;

        if (getDataOperand(file, &name, "character class name")) {
          const struct CharacterClass *class;

          if ((class = findCharacterClass(name.characters, name.length))) {
            reportDataError(file, "character class already defined: %.*" PRIws,
                            name.length, name.characters);
          } else if ((class = addCharacterClass(file, name.characters, name.length))) {
            DataString characters;

            if (getDataString(file, &characters, "characters")) {
              int index;

              for (index=0; index<characters.length; index+=1) {
                wchar_t character = characters.characters[index];
                ContractionTableCharacter *entry = getCharacterEntry(character);
                if (!entry) return 0;
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

        if (getCharacterClass(file, &class)) {
          *attributes |= class->attribute;
          continue;
        }
        break;
      }

      default:
        reportDataError(file, "unimplemented opcode: %" PRIws, opcodeNames[opcode]);
        break;
    }

    return 1;
  }
}

ContractionTable *
compileContractionTable (const char *fileName) {
  ContractionTable *table = NULL;

  characterTable = NULL;
  characterTableSize = 0;
  characterEntryCount = 0;

  if (!opcodeLengths[0]) {
    ContractionTableOpcode opcode;
    for (opcode=0; opcode<CTO_None; ++opcode)
      opcodeLengths[opcode] = wcslen(opcodeNames[opcode]);
  }

  if ((dataArea = newDataArea())) {
    if (allocateDataItem(dataArea, NULL, sizeof(ContractionTableHeader), __alignof__(ContractionTableHeader))) {
      if (allocateCharacterClasses()) {
        if (processDataFile(fileName, processContractionTableLine, NULL)) {
          if (saveCharacterTable()) {
            if ((table = malloc(sizeof(*table)))) {
              table->header.fields = getContractionTableHeader();
              table->size = getDataSize(dataArea);
              resetDataArea(dataArea);

              table->characters = NULL;
              table->charactersSize = 0;
              table->characterCount = 0;
            }
          }
        }

        deallocateCharacterClasses();
      }
    }

    destroyDataArea(dataArea);
  }

  if (characterTable) free(characterTable);
  return table;
}

void
destroyContractionTable (ContractionTable *table) {
  if (table->characters) {
    free(table->characters);
    table->characters = NULL;
  }

  if (table->size) {
    free(table->header.fields);
    free(table);
  }
}

void
fixContractionTablePath (char **path) {
  fixPath(path, CONTRACTION_TABLE_EXTENSION, CONTRACTION_TABLE_PREFIX);
}
