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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
 
#include "misc.h"
#include "contract.h"
#include "ctb_definitions.h"
#include "brl.h"


typedef struct {
  BYTE length;
  BYTE bytes[0XFF];
} ByteString;

static int errorCount;

static ContractionTableHeader *tableHeader;
static ContractionTableOffset tableSize;
static ContractionTableOffset tableUsed;

static BYTE *opcodesTable;
static int opcodesSize;
static int opcodesUsed;

static const char *originalLocale;

typedef struct {
  const char *fileName;
  int lineNumber;		/*line number in table */
} FileData;

static void
compileError (FileData *data, char *format, ...) {
  char buffer[0X100];
  va_list arguments;
  va_start(arguments, format);
  vsnprintf(buffer, sizeof(buffer), format, arguments);
  va_end(arguments);

  if (data)
    LogPrint(LOG_WARNING, "File %s line %d: %s",
             data->fileName, data->lineNumber, buffer);
  else
    LogPrint(LOG_WARNING, "%s", buffer);
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
addRule (FileData *data, ContractionTableOpcode opcode, ByteString *find, ByteString *replace) {
  ContractionTableOffset ruleOffset;
  int ruleSize = sizeof(ContractionTableRule) - 1;
  if (find) ruleSize += find->length;
  if (replace) ruleSize += replace->length;
  if (allocateBytes(data, &ruleOffset, ruleSize, __alignof__(ContractionTableRule))) {
    ContractionTableRule *newRule = CTR(tableHeader, ruleOffset);
    newRule->opcode = opcode;
    if (find)
      memcpy(&newRule->findrep[0], &find->bytes[0],
             (newRule->findlen = find->length));
    else
      newRule->findlen = 0;
    if (replace)
      memcpy(&newRule->findrep[newRule->findlen], &replace->bytes[0],
             (newRule->replen = replace->length));
    else
      newRule->replen = 0;
    newRule->next = 0;

    /*link new rule into table.*/
    if (newRule->findlen) {
      ContractionTableOffset bucket;

      /* first, handle single-character find strings. */
      if (newRule->findlen == 1) {
        ContractionTableCharacter *character = &tableHeader->characters[newRule->findrep[0]];
        if (newRule->opcode == CTO_Always) {
          character->always = ruleOffset;
        }
        if ((bucket = character->rules)) {
          ContractionTableRule *currentRule = CTR(tableHeader, bucket);
          while (currentRule->next)
            currentRule = CTR(tableHeader, currentRule->next);
          currentRule->next = ruleOffset;
        } else {
          character->rules = ruleOffset;
        }
      } else {
        /* Now, work through the various cases for multi-byte find strings. */
        bucket = hash(newRule->findrep);

        /* case 1, start new hash chain. */
        if (!tableHeader->rules[bucket]) {
          tableHeader->rules[bucket] = ruleOffset;
        } else {
          /* Case 2, longest find string goes at head of chain. */
          ContractionTableRule *currentRule = CTR(tableHeader, tableHeader->rules[bucket]);
          if (newRule->findlen > currentRule->findlen) {
            newRule->next = tableHeader->rules[bucket];
            tableHeader->rules[bucket] = ruleOffset;
          } else {
            /* Case 3, new rule goes somewhere in chain. */
            while (currentRule->next) { /*loop through chain */
              ContractionTableRule *previousRule = currentRule;
              currentRule = CTR(tableHeader, currentRule->next);
              if (newRule->findlen > currentRule->findlen) {
                newRule->next = previousRule->next;
                previousRule->next = ruleOffset;
                break;
              }
            }
            if (!newRule->next) {
              /* Case 4, new rule goes at end of chain*/
              currentRule->next = ruleOffset;
            }
          }
        }
      }
    }

    return 1;
  }
  return 0;
}

static int
addString (FileData *data, ContractionTableOffset *offset, ByteString *string) {
  if (allocateBytes(data, offset, string->length+1, __alignof__(BYTE))) {
    BYTE *address = CTA(tableHeader, *offset);
    memcpy(address+1, string->bytes, (*address = string->length));
    return 1;
  }
  return 0;
}

static ContractionTableOpcode
getOpcode (FileData *data, const char *token, int length) {
  BYTE *entry = opcodesTable;		/*pointer for looking things up in opcodesTable */
  if (entry) {
    const BYTE *end = entry + opcodesUsed;
    while (entry < end) {
      BYTE len = *entry++;
      if (len == length)
        if (memcmp(entry, token, length) == 0)
          return *(entry + len);
      entry += len + 1;
    }
  }
  compileError(data, "opcode not defined: %.*s", length, token);
  return CTO_None;
}

static int
addOpcode (FileData *data, const char *token, int length, ContractionTableOpcode opcode) {
  int size = opcodesUsed + length + 2;
  if (size > opcodesSize) {
    void *table = realloc(opcodesTable, size|=0XFF);
    if (!table) {
      compileError(data, "Not enough memory for opcodes table.");
      return 0;
    }
    opcodesTable = table;
    opcodesSize = size;
  }
  opcodesTable[opcodesUsed++] = length;
  memcpy(&opcodesTable[opcodesUsed], token, length);
  opcodesUsed += length;
  opcodesTable[opcodesUsed++] = opcode;
  return 1;
}

static int
getToken (FileData *data, const BYTE **token, int *length, const char *description) { /*find the next string of contiguous nonblank characters */
  const BYTE *start = *token + *length;
  int count = 0;

  while (isspace(*start)) start++;
  while (start[count] && !isspace(start[count])) count++;
  *token = start;
  if (!(*length = count)) {
    if (description)
      compileError(data, "%s not specified.", description);
    return 0;
  }
  return 1;
}				/*token obtained */

static int
hexadecimalDigit (FileData *data, int *value, BYTE digit) {
  if (digit >= '0' && digit <= '9')
    *value = digit - '0';
  else if (digit >= 'a' && digit <= 'f')
    *value = digit - 'a' + 10;
  else if (digit >= 'A' && digit <= 'F')
    *value = digit - 'A' + 10;
  else
    return 0;
  return 1;
}

static int
octalDigit (FileData *data, int *value, BYTE digit) {
  if (digit < '0' || digit > '7') return 0;
  *value = digit - '0';
  return 1;
}

static int
parseText (FileData *data, ByteString *result, const BYTE *token, const int length) {	/*interpret find string */
  int count = 0;		/*loop counters */
  int index;		/*loop counters */
  for (index = 0; index < length; index++) {
    BYTE byte = token[index];
    if (byte == '\\') { /* escape sequence */
      int ok = 0;
      int start = index;
      if (++index < length) {
        switch (byte = token[index]) {
          case '\\':
            ok = 1;
            break;
          case 'f':
            byte = '\f';
            ok = 1;
            break;
          case 'n':
            byte = '\n';
            ok = 1;
            break;
          case 'o':
            if (length - index > 3) {
              int high, middle, low;
              if (octalDigit(data, &high, token[++index]))
                if (high < 04)
                  if (octalDigit(data, &middle, token[++index]))
                    if (octalDigit(data, &low, token[++index])) {
                      byte = (high << 6) | (middle << 3) | low;
                      ok = 1;
                    }
            }
            break;
          case 'r':
            byte = '\r';
            ok = 1;
            break;
          case 's':
            byte = ' ';
            ok = 1;
            break;
          case 't':
            byte = '\t';
            ok = 1;
            break;
          case 'v':
            byte = '\v';
            ok = 1;
            break;
          case 'x':
            if (length - index > 2) {
              int high, low;
              if (hexadecimalDigit(data, &high, token[++index]))
                if (hexadecimalDigit(data, &low, token[++index])) {
                  byte = (high << 4) | low;
                  ok = 1;
                }
            }
            break;
        }
      }
      if (!ok) {
        index++;
        compileError(data, "invalid escape sequence: %.*s",
                     index-start, &token[start]);
        return 0;
      }
    }
    if (!byte) {
      byte = ' ';
    }
    result->bytes[count++] = byte;
  }
  result->length = count;
  return 1;
}				/*find string interpreted */

static int
parseDots (FileData *data, ByteString *cells, const BYTE *token, const int length) {	/*get dot patterns */
  BYTE cell = 0;		/*assembly place for dots */
  int count = 0;		/*loop counters */
  int index;		/*loop counters */
  int start = 0;

  for (index = 0; index < length; index++) {
    int started = index != start;
    BYTE byte = token[index];
    switch (byte) { /*or dots to make up Braille cell */
      {
        int dot;
      case '1':
        dot = B1;
        goto haveDot;
      case '2':
        dot = B2;
        goto haveDot;
      case '3':
        dot = B3;
        goto haveDot;
      case '4':
        dot = B4;
        goto haveDot;
      case '5':
        dot = B5;
        goto haveDot;
      case '6':
        dot = B6;
        goto haveDot;
      case '7':
        dot = B7;
        goto haveDot;
      case '8':
        dot = B8;
      haveDot:
        if (started && !cell) goto invalid;
        if (cell & dot) {
          compileError(data, "dot specified more than once: %c", byte);
          return 0;
        }
        cell |= dot;
        break;
      }
      case '0':			/*blank */
        if (started) goto invalid;
        break;
      case '-':			/*got all dots for this cell */
        if (!started) {
          compileError(data, "missing cell specification: %.*s",
                       length-index, &token[index]);
          return 0;
        }
        cells->bytes[count++] = cell;
        cell = 0;
        start = index + 1;
        break;
      default:
      invalid:
        compileError(data, "invalid dot number: %c", byte);
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
getFindText (FileData *data, ByteString *find, const BYTE **token, int *length) {
  if (getToken(data, token, length, "find text"))
    if (parseText(data, find, *token, *length))
      return 1;
  return 0;
}

static int
getReplaceText (FileData *data, ByteString *replace, const BYTE **token, int *length) {
  if (getToken(data, token, length, "replacement text"))
    if (parseText(data, replace, *token, *length))
      return 1;
  return 0;
}

static int
getReplacePattern (FileData *data, ByteString *replace, const BYTE **token, int *length) {
  if (getToken(data, token, length, "replacement pattern")) {
    if (*length == 1 && **token == '=') {
      replace->length = 0;
      return 1;
    }
    if (parseDots(data, replace, *token, *length))
      return 1;
  }
  return 0;
}

static int
integerToken (FileData *data, int *integer, const char *token, int length,
              const char *description, const int *minimum, const int *maximum) {
  char *end;
  unsigned long value = strtol(token, &end, 0);
  if (!minimum) {
    static const int limit = 0;
    minimum = &limit;
  }
  if (!maximum) {
    static const int limit = 0X7FFFFFFF;
    maximum = &limit;
  }
  if ((end != (token + length)) || (value < *minimum) || (value > *maximum)) {
    compileError(data, "invalid %s: %.*s", description, length, token);
    return 0;
  }
  *integer = value;
  return 1;
}

static int
opcodeToken (FileData *data, ContractionTableOpcode *opcode, const char *token, int length) {
  static const int minimum = 0;
  static const int maximum = CTO_None - 1;
  int integer;
  if (!integerToken(data, &integer, token, length, "opcode number", &minimum, &maximum))
    return 0;
  *opcode = integer;
  return 1;
}

static const char *
setLocale (const char *locale) {
  return setlocale(LC_CTYPE, locale);
}

static const char *
getLocale (void) {
  return setLocale(NULL);
}

static int processFile (const char *fileName);
static int
includeFile (FileData *data, ByteString *path) {
  const char *prefixAddress = data->fileName;
  int prefixLength = 0;
  const BYTE *suffixAddress = path->bytes;
  int suffixLength = path->length;
  if (*suffixAddress != '/') {
    const char *ptr = strrchr(prefixAddress, '/');
    if (ptr) prefixLength = ptr - prefixAddress + 1;
  }
  {
    char file[prefixLength + suffixLength + 1];
    snprintf(file, sizeof(file), "%.*s%.*s",
             prefixLength, prefixAddress, suffixLength, suffixAddress);
    return processFile(file);
  }
}

static int
processLine (FileData *data, const BYTE *line) {
  int ok = 0;
  const BYTE *token = line;
  int length = 0;			/*length of token */
  ContractionTableOpcode opcode;

  if (!getToken(data, &token, &length, NULL)) return 1;			/*blank line */
  if (*token == '#') return 1;

  if (*token < '0' || *token > '9') { /*look up word in opcode table */
    opcode = getOpcode(data, token, length);
  } else if (!opcodeToken(data, &opcode, token, length))
    return 1;
  switch (opcode) { /*Carry out operations */
    case CTO_None:
      break;
    case CTO_IncludeFile: {
      ByteString path;
      if (getToken(data, &token, &length, "include file path"))
        if (parseText(data, &path, token, length))
          if (!includeFile(data, &path))
            goto failure;
      break;
    }
    case CTO_Locale: {
      ByteString locale;
      if (getToken(data, &token, &length, "locale specification"))
        if (parseText(data, &locale, token, length)) {
          char string[locale.length + 1];
          snprintf(string, sizeof(string), "%.*s", locale.length, locale.bytes);
          if (!setLocale(string))
            compileError(data, "locale not available: %s", string);
          else if (!addString(data, &tableHeader->locale, &locale))
            goto failure;
        }
      break;
    }
    case CTO_Synonym:
      if (getToken(data, &token, &length, "opcode number"))
        if (opcodeToken(data, &opcode, token, length))
          if (getToken(data, &token, &length, "opcode name"))
            if (opcode != CTO_None)
              if (!addOpcode(data, token, length, opcode))
                goto failure;
      break;
    case CTO_Always:
    case CTO_WholeWord:
    case CTO_LowWord:
    case CTO_JoinableWord:
    case CTO_LargeSign:
    case CTO_SuffixableWord:
    case CTO_BegWord:
    case CTO_MidWord:
    case CTO_MidEndWord:
    case CTO_EndWord:
    case CTO_BegNum:
    case CTO_MidNum:
    case CTO_EndNum:
    case CTO_PrePunc:
    case CTO_PostPunc:
    case CTO_Repeated: {
      ByteString find;
      ByteString replace;
      if (getFindText(data, &find, &token, &length))
        if (getReplacePattern(data, &replace, &token, &length))
          if (!addRule(data, opcode, &find, &replace))
            goto failure;
      break;
    }
    case CTO_Replace: {
      ByteString find;
      ByteString replace;
      if (getFindText(data, &find, &token, &length))
        if (getReplaceText(data, &replace, &token, &length))
          if (!addRule(data, opcode, &find, &replace))
            goto failure;
      break;
    }
    case CTO_Contraction:
    case CTO_Literal: {
      ByteString find;
      if (getFindText(data, &find, &token, &length))
        if (!addRule(data, opcode, &find, NULL))
          goto failure;
      break;
    }
    case CTO_CapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "capital sign"))
        if (parseDots(data, &cells, token, length))
          if (!addString(data, &tableHeader->capitalSign, &cells))
            goto failure;
      break;
    }
    case CTO_BeginCapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "begin capital sign"))
        if (parseDots(data, &cells, token, length))
          if (!addString(data, &tableHeader->beginCapitalSign, &cells))
            goto failure;
      break;
    }
    case CTO_EndCapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "end capital sign"))
        if (parseDots(data, &cells, token, length))
          if (!addString(data, &tableHeader->endCapitalSign, &cells))
            goto failure;
      break;
    }
    case CTO_EnglishLetterSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "letter sign"))
        if (parseDots(data, &cells, token, length))
          if (!addString(data, &tableHeader->englishLetterSign, &cells))
            goto failure;
      break;
    }
    case CTO_NumberSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "number sign"))
        if (parseDots(data, &cells, token, length))
          if (!addString(data, &tableHeader->numberSign, &cells))
            goto failure;
      break;
    }
    default:
      compileError(data, "unimplemented opcode: %d", opcode);
      break;
  }				/*end of loop for processing tableStream */
  ok = 1;

failure:
  return ok;
}				/*compilation completed */

static int
processFile (const char *fileName) {
  int ok = 1;
  FILE *stream;

  FileData data;
  data.fileName = fileName;
  data.lineNumber = 0;
  LogPrint(LOG_DEBUG, "Including contraction table: %s", fileName);

  if ((stream = fopen(data.fileName, "r"))) {
    int size = 0X80;
    BYTE *buffer = malloc(size);
    if (buffer) {
      while (1) { /*Process lines in table */
        BYTE *line = fgets(buffer, size, stream);
        int length;
        if (!line) {
          if (ferror(stream))
            compileError(&data, "read error: %s", strerror(errno));
          break;
        }
        data.lineNumber++;

        while (buffer[(length = strlen(line)) - 1] != '\n') {
          if (length + 1 == size) {
            BYTE *newBuffer = realloc(buffer, size<<=1);
            if (!newBuffer) {
              compileError(&data, "cannot extend input buffer.");
              ok = 0;
              goto done;
            }
            buffer = newBuffer;
          }
          if (!(line = fgets(buffer+length, size-length, stream))) {
            if (ferror(stream))
              compileError(&data, "read error: %s", strerror(errno));
            else
              compileError(&data, "unterminated final line.");
            goto done;
          }
          length += strlen(line);
          line = buffer;
        }
        buffer[length-1] = 0;

        if (!processLine(&data, line)) {
          ok = 0;
          break;
        }
      }				/*end of loop for processing tableStream */
    done:
      free(buffer);
    } else {
      compileError(&data, "cannot allocate input buffer.");
      ok = 0;
    }
    fclose(stream);
  } else {
    LogPrint(LOG_ERR,
             "Cannot open contraction table '%s': %s",
             data.fileName, strerror(errno));
  }
  return ok;
}				/*compilation completed */

static void
cacheCharacterAttributes (void) {
  int character;
  for (character = 0; character < 0X100; character++) {
    ContractionTableCharacter *c = &tableHeader->characters[character];
    c->uppercase = c->lowercase = character;
    if (isspace(character))
      c->attributes |= CTC_Space;
    else if (isdigit(character))
      c->attributes |= CTC_Digit;
    else if (ispunct(character))
      c->attributes |= CTC_Punctuation;
    else if (isalpha(character)) {
      c->attributes |= CTC_Letter;
      if (isupper(character)) {
        c->attributes |= CTC_UpperCase;
        c->lowercase = tolower(character);
      } else if (islower(character)) {
        c->attributes |= CTC_LowerCase;
        c->uppercase = toupper(character);
      }
    }
  }
}

static int
auditCharacters (BYTE *encountered, const BYTE *start, BYTE length) {
   const BYTE *address = start;
   BYTE count = length;
  while (count--) {
    BYTE character = *address++;
    if (!tableHeader->characters[character].always && !encountered[character]) {
      encountered[character] = 1;
      compileError(NULL, "character representation not defined: %c (%.*s)",
                   character, length, start);
      return 0;
    }
  }
  return 1;
}

static int
auditTable (void) {
  int ok = 1;
  BYTE characterEncountered[0X100];
  int hashIndex;
  memset(characterEncountered, 0, sizeof(characterEncountered));
  for (hashIndex = 0; hashIndex < HASHNUM; hashIndex++) {
    ContractionTableOffset bucket = tableHeader->rules[hashIndex];
    while (bucket) {
      const ContractionTableRule *rule = CTR(tableHeader, bucket);
      switch (rule->opcode) {
        default:
          if (!rule->replen) {
            if (!auditCharacters(characterEncountered, &rule->findrep[0], rule->findlen)) {
              ok = 0;
            }
          }
          break;
        case CTO_Replace:
          if (!auditCharacters(characterEncountered, &rule->findrep[rule->findlen], rule->replen)) {
            ok = 0;
          }
          break;
        case CTO_Literal:
          break;
      }
      bucket = rule->next;
    }
  }
  return ok;
}

void *
compileContractionTable (const char *fileName) { /*compile source table into a table in memory */
  ContractionTableOffset headerOffset;
  errorCount = 0;

  tableHeader = NULL;
  tableSize = 0;
  tableUsed = 0;

  opcodesTable = NULL;
  opcodesSize = 0;
  opcodesUsed = 0;

  originalLocale = setLocale("C");

  if (allocateBytes(NULL, &headerOffset, sizeof(*tableHeader), __alignof__(*tableHeader))) {
    if (headerOffset == 0) {
      if (processFile(fileName))
        auditTable();
      if (errorCount)
        LogPrint(LOG_ERR, "%d %s in contraction table '%s'.",
                 errorCount, ((errorCount == 1)? "error": "errors"), fileName);
    } else {
      compileError(NULL, "contraction table header not allocated at offset 0.");
    }
  }

  cacheCharacterAttributes();
  setLocale(originalLocale);

  if (opcodesTable) {
    free(opcodesTable);
    opcodesTable = NULL;
  }

  return (void *)tableHeader;
}

int
destroyContractionTable (void *contractionTable) {
  ContractionTableHeader *table = (ContractionTableHeader *) contractionTable;
  free(table);
  return 1;
}
