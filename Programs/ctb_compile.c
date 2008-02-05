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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
 
#include "misc.h"
#include "charset.h"
#include "ctb.h"
#include "ctb_internal.h"
#include "brl.h"

typedef struct {
  BYTE length;
  wchar_t characters[0XFF];
} CharacterString;

typedef struct {
  BYTE length;
  BYTE bytes[0XFF];
} ByteString;

static int errorCount;

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
  WS_C("repeated"),

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

typedef struct {
  const char *fileName;
  int lineNumber;		/*line number in table */
} FileData;

static void compileError (FileData *data, char *format, ...) PRINTF(2, 3);
static void
compileError (FileData *data, char *format, ...) {
  char message[0X100];

  {
    const char *file = NULL;
    const int *line = NULL;
    va_list args;

    if (data) {
      file = data->fileName;
      if (data->lineNumber) line = &data->lineNumber;
    }

    va_start(args, format);
    formatInputError(message, sizeof(message),
                     file, line,
                     format, args);
    va_end(args);
  }

  LogPrint(LOG_WARNING, "%s", message);
  errorCount++;
}

static int
allocateBytes (FileData *data, ContractionTableOffset *offset, int count, int alignment) {
  int size = (tableUsed = (tableUsed + (alignment - 1)) / alignment * alignment) + count;
  if (size > tableSize) {
    void *table = realloc(tableHeader, size|=0XFFF);
    if (!table) {
      compileError(data, "Not enough memory for contraction table.");
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
saveBytes (FileData *data, ContractionTableOffset *offset, const void *bytes, int count, int alignment) {
  if (allocateBytes(data, offset, count, alignment)) {
    BYTE *address = CTA(tableHeader, *offset);
    memcpy(address, bytes, count);
    return 1;
  }
  return 0;
}

static int
saveSequence (FileData *data, ContractionTableOffset *offset, const ByteString *sequence) {
  if (allocateBytes(data, offset, sequence->length+1, __alignof__(BYTE))) {
    BYTE *address = CTA(tableHeader, *offset);
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
  FileData *data,
  ContractionTableOpcode opcode,
  CharacterString *find,
  ByteString *replace,
  ContractionTableCharacterAttributes after,
  ContractionTableCharacterAttributes before
) {
  ContractionTableOffset ruleOffset;
  int ruleSize = sizeof(ContractionTableRule) - sizeof(find->characters[0]);
  if (find) ruleSize += find->length * sizeof(find->characters[0]);
  if (replace) ruleSize += replace->length;

  if (allocateBytes(data, &ruleOffset, ruleSize, __alignof__(ContractionTableRule))) {
    ContractionTableRule *newRule = CTR(tableHeader, ruleOffset);

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
        ContractionTableRule *currentRule = CTR(tableHeader, *offsetAddress);
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
findCharacterClass (const wchar_t *name, int length) {
  const struct CharacterClass *class = characterClasses;
  while (class) {
    if ((length == class->length) &&
        (wmemcmp(name, class->name, length) == 0))
      return class;
    class = class->next;
  }
  return NULL;
}

static struct CharacterClass *
addCharacterClass (FileData *data, const wchar_t *name, int length) {
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
  compileError(data, "character class table overflow: %.*" PRIws, length, name);
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
getOpcode (FileData *data, const wchar_t *token, int length) {
  ContractionTableOpcode opcode;
  for (opcode=0; opcode<CTO_None; ++opcode)
    if (length == opcodeLengths[opcode])
      if (wmemcmp(token, opcodeNames[opcode], length) == 0)
        return opcode;
  compileError(data, "opcode not defined: %.*" PRIws, length, token);
  return CTO_None;
}

static int
getToken (FileData *data, const wchar_t **token, int *length, const char *description) { /*find the next string of contiguous nonblank characters */
  const wchar_t *start = *token + *length;
  int count = 0;

  while (iswspace(*start)) start++;
  while (start[count] && !iswspace(start[count])) count++;
  *token = start;
  if (!(*length = count)) {
    if (description)
      compileError(data, "%s not specified.", description);
    return 0;
  }
  return 1;
}				/*token obtained */

static int
hexadecimalDigit (FileData *data, int *value, wchar_t digit) {
  if (digit >= WC_C('0') && digit <= WC_C('9'))
    *value = digit - WC_C('0');
  else if (digit >= WC_C('a') && digit <= WC_C('f'))
    *value = digit - WC_C('a') + 10;
  else if (digit >= WC_C('A') && digit <= WC_C('F'))
    *value = digit - WC_C('A') + 10;
  else
    return 0;
  return 1;
}

static int
octalDigit (FileData *data, int *value, wchar_t digit) {
  if (digit < WC_C('0') || digit > WC_C('7')) return 0;
  *value = digit - WC_C('0');
  return 1;
}

static int
parseCharacters (FileData *data, CharacterString *result, const wchar_t *token, const int length) {	/*interpret find string */
  int count = 0;		/*loop counters */
  int index;		/*loop counters */
  for (index = 0; index < length; index++) {
    wchar_t character = token[index];
    if (character == WC_C('\\')) { /* escape sequence */
      int ok = 0;
      int start = index;
      if (++index < length) {
        switch (character = token[index]) {
          case WC_C('\\'):
            ok = 1;
            break;
          case WC_C('f'):
            character = WC_C('\f');
            ok = 1;
            break;
          case WC_C('n'):
            character = WC_C('\n');
            ok = 1;
            break;
          case WC_C('o'):
            if (length - index > 3) {
              int high, middle, low;
              if (octalDigit(data, &high, token[++index]))
                if (high < 04)
                  if (octalDigit(data, &middle, token[++index]))
                    if (octalDigit(data, &low, token[++index])) {
                      character = (high << 6) | (middle << 3) | low;
                      ok = 1;
                    }
            }
            break;
          case WC_C('r'):
            character = WC_C('\r');
            ok = 1;
            break;
          case WC_C('s'):
            character = WC_C(' ');
            ok = 1;
            break;
          case WC_C('t'):
            character = WC_C('\t');
            ok = 1;
            break;
          case WC_C('v'):
            character = WC_C('\v');
            ok = 1;
            break;
          case WC_C('x'):
            if (length - index > 2) {
              int high, low;
              if (hexadecimalDigit(data, &high, token[++index]))
                if (hexadecimalDigit(data, &low, token[++index])) {
                  character = (high << 4) | low;
                  ok = 1;
                }
            }
            break;
        }
      }
      if (!ok) {
        index++;
        compileError(data, "invalid escape sequence: %.*" PRIws,
                     index-start, &token[start]);
        return 0;
      }
    }
    if (!character) character = WC_C(' ');
    result->characters[count++] = character;
  }
  result->length = count;
  return 1;
}				/*find string interpreted */

static int
parseDots (FileData *data, ByteString *cells, const wchar_t *token, const int length) {	/*get dot patterns */
  BYTE cell = 0;		/*assembly place for dots */
  int count = 0;		/*loop counters */
  int index;		/*loop counters */
  int start = 0;

  for (index = 0; index < length; index++) {
    int started = index != start;
    wchar_t character = token[index];
    switch (character) { /*or dots to make up Braille cell */
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
          compileError(data, "dot specified more than once: %.1" PRIws, &character);
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
          compileError(data, "missing cell specification: %.*" PRIws,
                       length-index, &token[index]);
          return 0;
        }
        cells->bytes[count++] = cell;
        cell = 0;
        start = index + 1;
        break;
      default:
      invalid:
        compileError(data, "invalid dot number: %.1" PRIws, &character);
        return 0;
    }
  }
  if (index == start) {
    compileError(data, "missing cell specification.");
    return 0;
  }
  cells->bytes[count++] = cell;		/*last cell */
  cells->length = count;
  return 1;
}				/*end of function parseDots */

static int
getCharacters (FileData *data, CharacterString *characters, const wchar_t **token, int *length) {
  if (getToken(data, token, length, "characters"))
    if (parseCharacters(data, characters, *token, *length))
      return 1;
  return 0;
}

static int
getFindText (FileData *data, CharacterString *find, const wchar_t **token, int *length) {
  if (getToken(data, token, length, "find text"))
    if (parseCharacters(data, find, *token, *length))
      return 1;
  return 0;
}

static int
getReplacePattern (FileData *data, ByteString *replace, const wchar_t **token, int *length) {
  if (getToken(data, token, length, "replacement pattern")) {
    if (*length == 1 && **token == WC_C('=')) {
      replace->length = 0;
      return 1;
    }
    if (parseDots(data, replace, *token, *length))
      return 1;
  }
  return 0;
}

static int
getCharacterClass (FileData *data, const struct CharacterClass **class, const wchar_t **token, int *length) {
  if (getToken(data, token, length, "character class name")) {
    if ((*class = findCharacterClass(*token, *length))) return 1;
    compileError(data, "character class not defined: %.*" PRIws, *length, *token);
  }
  return 0;
}

static int processFile (const char *fileName);
static int
includeFile (FileData *data, CharacterString *path) {
  const char *prefixAddress = data->fileName;
  int prefixLength = 0;
  const wchar_t *suffixAddress = path->characters;
  int suffixLength = path->length;

  if (*suffixAddress != WC_C('/')) {
    const char *ptr = strrchr(prefixAddress, '/');
    if (ptr) prefixLength = ptr - prefixAddress + 1;
  }

  {
    char file[prefixLength + suffixLength + 1];
    snprintf(file, sizeof(file), "%.*s%.*" PRIws,
             prefixLength, prefixAddress, suffixLength, suffixAddress);
    return processFile(file);
  }
}

static int
processWcharLine (FileData *data, const wchar_t *line) {
  int ok = 0;
  const wchar_t *token = line;
  int length = 0;			/*length of token */
  ContractionTableOpcode opcode;
  ContractionTableCharacterAttributes after = 0;
  ContractionTableCharacterAttributes before = 0;

doOpcode:
  if (!getToken(data, &token, &length, NULL)) return 1;			/*blank line */
  if (*token == WC_C('#')) return 1;
  opcode = getOpcode(data, token, length);

  switch (opcode) { /*Carry out operations */
    case CTO_None:
      break;

    case CTO_IncludeFile: {
      CharacterString path;
      if (getToken(data, &token, &length, "include file path"))
        if (parseCharacters(data, &path, token, length))
          if (!includeFile(data, &path))
            goto failure;
      break;
    }

    case CTO_Always:
    case CTO_LargeSign:
    case CTO_LastLargeSign:
    case CTO_WholeWord:
    case CTO_JoinableWord:
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
    case CTO_Repeated: {
      CharacterString find;
      ByteString replace;
      if (getFindText(data, &find, &token, &length))
        if (getReplacePattern(data, &replace, &token, &length))
          if (!addRule(data, opcode, &find, &replace, after, before))
            goto failure;
      break;
    }

    case CTO_Contraction:
    case CTO_Literal: {
      CharacterString find;
      if (getFindText(data, &find, &token, &length))
        if (!addRule(data, opcode, &find, NULL, after, before))
          goto failure;
      break;
    }

    case CTO_CapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "capital sign"))
        if (parseDots(data, &cells, token, length)) {
          ContractionTableOffset offset;
          if (!saveSequence(data, &offset, &cells))
            goto failure;
          tableHeader->capitalSign = offset;
        }
      break;
    }

    case CTO_BeginCapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "begin capital sign"))
        if (parseDots(data, &cells, token, length)) {
          ContractionTableOffset offset;
          if (!saveSequence(data, &offset, &cells))
            goto failure;
          tableHeader->beginCapitalSign = offset;
        }
      break;
    }

    case CTO_EndCapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "end capital sign"))
        if (parseDots(data, &cells, token, length)) {
          ContractionTableOffset offset;
          if (!saveSequence(data, &offset, &cells))
            goto failure;
          tableHeader->endCapitalSign = offset;
        }
      break;
    }

    case CTO_EnglishLetterSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "letter sign"))
        if (parseDots(data, &cells, token, length)) {
          ContractionTableOffset offset;
          if (!saveSequence(data, &offset, &cells))
            goto failure;
          tableHeader->englishLetterSign = offset;
        }
      break;
    }

    case CTO_NumberSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "number sign"))
        if (parseDots(data, &cells, token, length)) {
          ContractionTableOffset offset;
          if (!saveSequence(data, &offset, &cells))
            goto failure;
          tableHeader->numberSign = offset;
        }
      break;
    }

    case CTO_Class: {
      const struct CharacterClass *class;
      CharacterString characters;
      if (getToken(data, &token, &length, "character class name")) {
        if ((class = findCharacterClass(token, length))) {
          compileError(data, "character class already defined: %.*" PRIws, length, token);
        } else if ((class = addCharacterClass(data, token, length))) {
          if (getCharacters(data, &characters, &token, &length)) {
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

      if (getCharacterClass(data, &class, &token, &length)) {
        *attributes |= class->attribute;
        goto doOpcode;
      }
      break;
    }

    default:
      compileError(data, "unimplemented opcode: %.*" PRIws, length, token);
      break;
  }				/*end of loop for processing tableStream */
  ok = 1;

failure:
  return ok;
}

static int
processUtf8Line (char *line, void *dataAddress) {
  FileData *data = dataAddress;
  const char *byte = line;
  size_t length = strlen(line);
  wchar_t characters[length];
  wchar_t *character = characters;

  data->lineNumber++;

  while (length) {
    const char *start = byte;
    wint_t wc = convertUtf8ToWchar(&byte, &length);

    if (wc == WEOF) {
      compileError(data, "illegal UTF-8 character at offset %d", start-line);
      return 1;
    }

    *character++ = wc;
  }

  *character = 0;
  return processWcharLine(data, characters);
}

static int
processFile (const char *fileName) {
  int ok = 1;
  FILE *stream;

  FileData data;
  data.fileName = fileName;
  data.lineNumber = 0;
  LogPrint(LOG_DEBUG, "Including contraction table: %s", fileName);

  if ((stream = openDataFile(data.fileName, "r"))) {
    if (!processLines(stream, processUtf8Line, &data)) return 0;
    fclose(stream);
  } else {
    LogPrint(LOG_ERR,
             "Cannot open contraction table '%s': %s",
             data.fileName, strerror(errno));
  }
  return ok;
}				/*compilation completed */

void *
compileContractionTable (const char *fileName) { /*compile source table into a table in memory */
  int ok = 0;
  ContractionTableOffset headerOffset;
  errorCount = 0;

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
        if (processFile(fileName)) {
          if (saveCharacterTable()) {
            ok = 1;
          }
        }

        deallocateCharacterClasses();
      }
    } else {
      compileError(NULL, "contraction table header not allocated at offset 0.");
    }
  }

  if (!ok) {
    if (characterTable) {
      free(characterTable);
      characterTable = NULL;
    }

    if (tableHeader) {
      free(tableHeader);
      tableHeader = NULL;
    }
  }

  if (errorCount)
    LogPrint(LOG_WARNING, "%d %s in contraction table '%s'.",
             errorCount, ((errorCount == 1)? "error": "errors"), fileName);
  return tableHeader;
}

int
destroyContractionTable (void *contractionTable) {
  ContractionTableHeader *table = (ContractionTableHeader *) contractionTable;
  free(table);
  return 1;
}

void
fixContractionTablePath (char **path) {
  fixPath(path, CONTRACTION_TABLE_EXTENSION, CONTRACTION_TABLE_PREFIX);
}
