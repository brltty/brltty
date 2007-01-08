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

#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
 
#include "misc.h"
#include "ctb.h"
#include "ctb_internal.h"
#include "brl.h"

typedef struct {
  BYTE length;
  BYTE bytes[0XFF];
} ByteString;

static int errorCount;

static ContractionTableHeader *tableHeader;
static ContractionTableOffset tableSize;
static ContractionTableOffset tableUsed;

static const char *const characterClassNames[] = {
  "space",
  "letter",
  "digit",
  "punctuation",
  "uppercase",
  "lowercase",
  NULL
};
struct CharacterClass {
  struct CharacterClass *next;
  ContractionTableCharacterAttributes attribute;
  BYTE length;
  char name[1];
};
static struct CharacterClass *characterClasses;
static ContractionTableCharacterAttributes characterClassAttribute;

static const char *const opcodeNames[CTO_None] = {
  "include",
  "locale",

  "capsign",
  "begcaps",
  "endcaps",

  "letsign",
  "numsign",

  "literal",
  "replace",
  "always",
  "repeated",

  "largesign",
  "lastlargesign",
  "word",
  "joinword",
  "lowword",
  "contraction",

  "sufword",
  "prfword",
  "begword",
  "begmidword",
  "midword",
  "midendword",
  "endword",

  "prepunc",
  "postpunc",

  "begnum",
  "midnum",
  "endnum",

  "class",
  "after",
  "before"
};
static unsigned char opcodeLengths[CTO_None] = {0};

static const char *originalLocale;
static int noLocale;

typedef struct {
  const char *fileName;
  int lineNumber;		/*line number in table */
} FileData;

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
saveBytes (FileData *data, ContractionTableOffset *offset, const BYTE *bytes, BYTE length) {
  if (allocateBytes(data, offset, length+1, __alignof__(BYTE))) {
    BYTE *address = CTA(tableHeader, *offset);
    memcpy(address+1, bytes, (*address = length));
    return 1;
  }
  return 0;
}

static int
saveString (FileData *data, ContractionTableOffset *offset, const char *string) {
  return saveBytes(data, offset, (const unsigned char *)string, strlen(string));
}

static int
saveByteString (FileData *data, ContractionTableOffset *offset, const ByteString *string) {
  return saveBytes(data, offset, string->bytes, string->length);
}

static ContractionTableRule *
addRule (
  FileData *data,
  ContractionTableOpcode opcode,
  ByteString *find,
  ByteString *replace,
  ContractionTableCharacterAttributes after,
  ContractionTableCharacterAttributes before
) {
  ContractionTableOffset ruleOffset;
  int ruleSize = sizeof(ContractionTableRule) - 1;
  if (find) ruleSize += find->length;
  if (replace) ruleSize += replace->length;
  if (allocateBytes(data, &ruleOffset, ruleSize, __alignof__(ContractionTableRule))) {
    ContractionTableRule *newRule = CTR(tableHeader, ruleOffset);

    newRule->opcode = opcode;
    newRule->after = after;
    newRule->before = before;
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

    /*link new rule into table.*/
    {
      ContractionTableOffset *offsetAddress;

      if (newRule->findlen == 1) {
        ContractionTableCharacter *character = &tableHeader->characters[newRule->findrep[0]];
        if (newRule->opcode == CTO_Always) character->always = ruleOffset;
        offsetAddress = &character->rules;
      } else {
        offsetAddress = &tableHeader->rules[hash(newRule->findrep)];
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
findCharacterClass (const unsigned char *name, int length) {
  const struct CharacterClass *class = characterClasses;
  while (class) {
    if ((length == class->length) &&
        (memcmp(name, class->name, length) == 0))
      return class;
    class = class->next;
  }
  return NULL;
}

static struct CharacterClass *
addCharacterClass (FileData *data, const unsigned char *name, int length) {
  struct CharacterClass *class;
  if (characterClassAttribute) {
    if ((class = malloc(sizeof(*class) + length - 1))) {
      memset(class, 0, sizeof(*class));
      memcpy(class->name, name, (class->length = length));

      class->attribute = characterClassAttribute;
      characterClassAttribute <<= 1;

      class->next = characterClasses;
      characterClasses = class;
      return class;
    }
  }
  compileError(data, "character class table overflow: %.*s", length, name);
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
  const char *const *name = characterClassNames;
  characterClasses = NULL;
  characterClassAttribute = 1;
  while (*name) {
    if (!addCharacterClass(NULL, (const unsigned char *)*name, strlen(*name))) {
      deallocateCharacterClasses();
      return 0;
    }
    ++name;
  }
  return 1;
}

static ContractionTableOpcode
getOpcode (FileData *data, const unsigned char *token, int length) {
  ContractionTableOpcode opcode;
  for (opcode=0; opcode<CTO_None; ++opcode)
    if (length == opcodeLengths[opcode])
      if (memcmp(token, opcodeNames[opcode], length) == 0)
        return opcode;
  compileError(data, "opcode not defined: %.*s", length, token);
  return CTO_None;
}

static int
getToken (FileData *data, const unsigned char **token, int *length, const char *description) { /*find the next string of contiguous nonblank characters */
  const unsigned char *start = *token + *length;
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
hexadecimalDigit (FileData *data, int *value, char digit) {
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
octalDigit (FileData *data, int *value, char digit) {
  if (digit < '0' || digit > '7') return 0;
  *value = digit - '0';
  return 1;
}

static int
parseText (FileData *data, ByteString *result, const unsigned char *token, const int length) {	/*interpret find string */
  int count = 0;		/*loop counters */
  int index;		/*loop counters */
  for (index = 0; index < length; index++) {
    char character = token[index];
    if (character == '\\') { /* escape sequence */
      int ok = 0;
      int start = index;
      if (++index < length) {
        switch (character = token[index]) {
          case '\\':
            ok = 1;
            break;
          case 'f':
            character = '\f';
            ok = 1;
            break;
          case 'n':
            character = '\n';
            ok = 1;
            break;
          case 'o':
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
          case 'r':
            character = '\r';
            ok = 1;
            break;
          case 's':
            character = ' ';
            ok = 1;
            break;
          case 't':
            character = '\t';
            ok = 1;
            break;
          case 'v':
            character = '\v';
            ok = 1;
            break;
          case 'x':
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
        compileError(data, "invalid escape sequence: %.*s",
                     index-start, &token[start]);
        return 0;
      }
    }
    if (!character) character = ' ';
    result->bytes[count++] = character;
  }
  result->length = count;
  return 1;
}				/*find string interpreted */

static int
parseDots (FileData *data, ByteString *cells, const unsigned char *token, const int length) {	/*get dot patterns */
  BYTE cell = 0;		/*assembly place for dots */
  int count = 0;		/*loop counters */
  int index;		/*loop counters */
  int start = 0;

  for (index = 0; index < length; index++) {
    int started = index != start;
    char character = token[index];
    switch (character) { /*or dots to make up Braille cell */
      {
        int dot;
      case '1':
        dot = BRL_DOT1;
        goto haveDot;
      case '2':
        dot = BRL_DOT2;
        goto haveDot;
      case '3':
        dot = BRL_DOT3;
        goto haveDot;
      case '4':
        dot = BRL_DOT4;
        goto haveDot;
      case '5':
        dot = BRL_DOT5;
        goto haveDot;
      case '6':
        dot = BRL_DOT6;
        goto haveDot;
      case '7':
        dot = BRL_DOT7;
        goto haveDot;
      case '8':
        dot = BRL_DOT8;
      haveDot:
        if (started && !cell) goto invalid;
        if (cell & dot) {
          compileError(data, "dot specified more than once: %c", character);
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
        compileError(data, "invalid dot number: %c", character);
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
getCharacters (FileData *data, ByteString *characters, const unsigned char **token, int *length) {
  if (getToken(data, token, length, "characters"))
    if (parseText(data, characters, *token, *length))
      return 1;
  return 0;
}

static int
getFindText (FileData *data, ByteString *find, const unsigned char **token, int *length) {
  if (getToken(data, token, length, "find text"))
    if (parseText(data, find, *token, *length))
      return 1;
  return 0;
}

static int
getReplaceText (FileData *data, ByteString *replace, const unsigned char **token, int *length) {
  if (getToken(data, token, length, "replacement text"))
    if (parseText(data, replace, *token, *length))
      return 1;
  return 0;
}

static int
getReplacePattern (FileData *data, ByteString *replace, const unsigned char **token, int *length) {
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

static const char *
setLocale (const char *locale) {
  return setlocale(LC_CTYPE, locale);
}

static int
getCharacterClass (FileData *data, const struct CharacterClass **class, const unsigned char **token, int *length) {
  if (getToken(data, token, length, "character class name")) {
    if ((*class = findCharacterClass(*token, *length))) return 1;
    compileError(data, "character class not defined: %.*s", *length, *token);
  }
  return 0;
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
  const unsigned char *suffixAddress = path->bytes;
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
processLine (FileData *data, const char *line) {
  int ok = 0;
  const unsigned char *token = (const unsigned char *)line;
  int length = 0;			/*length of token */
  ContractionTableOpcode opcode;
  ContractionTableCharacterAttributes after = 0;
  ContractionTableCharacterAttributes before = 0;

doOpcode:
  if (!getToken(data, &token, &length, NULL)) return 1;			/*blank line */
  if (*token == '#') return 1;
  opcode = getOpcode(data, token, length);

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
          if (strcmp(string, "-") == 0)
            noLocale = 1;
          else if (!setLocale(string))
            compileError(data, "locale not available: %s", string);
          else
            noLocale = 0;
        }
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
      ByteString find;
      ByteString replace;
      if (getFindText(data, &find, &token, &length))
        if (getReplacePattern(data, &replace, &token, &length))
          if (!addRule(data, opcode, &find, &replace, after, before))
            goto failure;
      break;
    }

    case CTO_Replace: {
      ByteString find;
      ByteString replace;
      if (getFindText(data, &find, &token, &length))
        if (getReplaceText(data, &replace, &token, &length))
          if (!addRule(data, opcode, &find, &replace, after, before))
            goto failure;
      break;
    }

    case CTO_Contraction:
    case CTO_Literal: {
      ByteString find;
      if (getFindText(data, &find, &token, &length))
        if (!addRule(data, opcode, &find, NULL, after, before))
          goto failure;
      break;
    }

    case CTO_CapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "capital sign"))
        if (parseDots(data, &cells, token, length))
          if (!saveByteString(data, &tableHeader->capitalSign, &cells))
            goto failure;
      break;
    }

    case CTO_BeginCapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "begin capital sign"))
        if (parseDots(data, &cells, token, length))
          if (!saveByteString(data, &tableHeader->beginCapitalSign, &cells))
            goto failure;
      break;
    }

    case CTO_EndCapitalSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "end capital sign"))
        if (parseDots(data, &cells, token, length))
          if (!saveByteString(data, &tableHeader->endCapitalSign, &cells))
            goto failure;
      break;
    }

    case CTO_EnglishLetterSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "letter sign"))
        if (parseDots(data, &cells, token, length))
          if (!saveByteString(data, &tableHeader->englishLetterSign, &cells))
            goto failure;
      break;
    }

    case CTO_NumberSign: {
      ByteString cells;
      if (getToken(data, &token, &length, "number sign"))
        if (parseDots(data, &cells, token, length))
          if (!saveByteString(data, &tableHeader->numberSign, &cells))
            goto failure;
      break;
    }

    case CTO_Class: {
      const struct CharacterClass *class;
      ByteString characters;
      if (getToken(data, &token, &length, "character class name")) {
        if ((class = findCharacterClass(token, length))) {
          compileError(data, "character class already defined: %.*s", length, token);
        } else if ((class = addCharacterClass(data, token, length))) {
          if (getCharacters(data, &characters, &token, &length)) {
            int index;
            for (index=0; index<characters.length; ++index) {
              ContractionTableCharacter *character = &tableHeader->characters[characters.bytes[index]];
              character->attributes |= class->attribute;
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
      compileError(data, "unimplemented opcode: %.*s", length, token);
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

  if ((stream = openDataFile(data.fileName, "r"))) {
    int size = 0X80;
    char *buffer = malloc(size);
    if (buffer) {
      while (1) { /*Process lines in table */
        char *line = fgets(buffer, size, stream);
        int length;
        if (!line) {
          if (ferror(stream))
            compileError(&data, "read error: %s", strerror(errno));
          break;
        }
        data.lineNumber++;

        while (buffer[(length = strlen(line)) - 1] != '\n') {
          if (length + 1 == size) {
            char *newBuffer = realloc(buffer, size<<=1);
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
  int ok = 0;
  ContractionTableOffset headerOffset;
  errorCount = 0;

  tableHeader = NULL;
  tableSize = 0;
  tableUsed = 0;

  if (!opcodeLengths[0]) {
    ContractionTableOpcode opcode;
    for (opcode=0; opcode<CTO_None; ++opcode)
      opcodeLengths[opcode] = strlen(opcodeNames[opcode]);
  }

  originalLocale = getLocale();
  setLocale("C");
  noLocale = 0;

  if (allocateBytes(NULL, &headerOffset, sizeof(*tableHeader), __alignof__(*tableHeader))) {
    if (headerOffset == 0) {
      if (allocateCharacterClasses()) {
        if (processFile(fileName)) {
          if (auditTable()) {
            ok = 1;
            if (!noLocale) {
              if (saveString(NULL, &tableHeader->locale, getLocale()))
                cacheCharacterAttributes();
              else
                ok = 0;
            }
          }
        }
        deallocateCharacterClasses();
      }
    } else {
      compileError(NULL, "contraction table header not allocated at offset 0.");
    }
  }

  setLocale(originalLocale);

  if (!ok) {
    free(tableHeader);
    tableHeader = NULL;
  }

  if (errorCount)
    LogPrint(LOG_WARNING, "%d %s in contraction table '%s'.",
             errorCount, ((errorCount == 1)? "error": "errors"), fileName);
  return (void *)tableHeader;
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
