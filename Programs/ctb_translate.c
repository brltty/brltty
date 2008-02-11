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

#include <string.h>
 
#include "tbl.h"
#include "ctb.h"
#include "ctb_internal.h"
#include "brl.h"

static const ContractionTableHeader *table;	/*translation table */
static const wchar_t *src, *srcmin, *srcmax;
static BYTE *dest, *destmin, *destmax;
static int *offsets;
static wchar_t before, after;	/*the characters before and after a string */
static int currentFindLength;		/*length of current find string */
static ContractionTableOpcode currentOpcode;
static ContractionTableOpcode previousOpcode;
static const ContractionTableRule *currentRule;	/*pointer to current rule in table */

#define setOffset() offsets[src - srcmin] = dest - destmin

typedef struct {
  wchar_t value;
  wchar_t uppercase;
  wchar_t lowercase;
  ContractionTableCharacterAttributes attributes;
} CharacterDefinition;

static CharacterDefinition *characterDefinitions = NULL;
static int characterDefinitionsSize = 0;
static int characterDefinitionCount = 0;

static const ContractionTableCharacter *
getContractionTableCharacter (wchar_t character) {
  const ContractionTableCharacter *characters = (ContractionTableCharacter *)CTA(table, table->characters);
  int first = 0;
  int last = table->characterCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    const ContractionTableCharacter *ctc = &characters[current];

    if (ctc->value < character) {
      first = current + 1;
    } else if (ctc->value > character) {
      last = current - 1;
    } else {
      return ctc;
    }
  }

  return NULL;
}

static CharacterDefinition *
getCharacterDefinition (wchar_t character) {
  int first = 0;
  int last = characterDefinitionCount - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    CharacterDefinition *definition = &characterDefinitions[current];

    if (definition->value < character) {
      first = current + 1;
    } else if (definition->value > character) {
      last = current - 1;
    } else {
      return definition;
    }
  }

  if (characterDefinitionCount == characterDefinitionsSize) {
    int newSize = characterDefinitionsSize;
    newSize = newSize? newSize<<1: 0X80;

    {
      CharacterDefinition *newDefinitions = realloc(characterDefinitions, (newSize * sizeof(*newDefinitions)));
      if (!newDefinitions) return NULL;

      characterDefinitions = newDefinitions;
      characterDefinitionsSize = newSize;
    }
  }

  memmove(&characterDefinitions[first+1],
          &characterDefinitions[first],
          (characterDefinitionCount - first) * sizeof(*characterDefinitions));
  ++characterDefinitionCount;

  {
    CharacterDefinition *definition = &characterDefinitions[first];
    memset(definition, 0, sizeof(*definition));
    definition->value = definition->uppercase = definition->lowercase = character;

    if (iswspace(character)) {
      definition->attributes |= CTC_Space;
    } else if (iswalpha(character)) {
      definition->attributes |= CTC_Letter;

      if (iswupper(character)) {
        definition->attributes |= CTC_UpperCase;
        definition->lowercase = towlower(character);
      }

      if (iswlower(character)) {
        definition->attributes |= CTC_LowerCase;
        definition->uppercase = towupper(character);
      }
    } else if (iswdigit(character)) {
      definition->attributes |= CTC_Digit;
    } else if (iswpunct(character)) {
      definition->attributes |= CTC_Punctuation;
    }

    {
      const ContractionTableCharacter *ctc = getContractionTableCharacter(character);
      if (ctc) definition->attributes |= ctc->attributes;
    }

    return definition;
  }
}

static int
testCharacter (wchar_t character, ContractionTableCharacterAttributes attributes) {
  const CharacterDefinition *definition = getCharacterDefinition(character);
  return definition && (attributes & definition->attributes);
}

static wchar_t
toLowerCase (wchar_t character) {
  const CharacterDefinition *definition = getCharacterDefinition(character);
  return definition? definition->lowercase: character;
}

static int
checkCurrentRule (const wchar_t *source) {
  const wchar_t *character = currentRule->findrep;
  int count = currentFindLength;

  while (count) {
    if (toLowerCase(*source) != toLowerCase(*character)) return 0;
    --count, ++source, ++character;
  }
  return 1;
}

static void
setBefore (void) {
  before = (src == srcmin)? WC_C(' '): src[-1];
}

static void
setAfter (int length) {
  after = (src + length < srcmax)? src[length]: WC_C(' ');
}

static int
selectRule (int length) { /*check for valid contractions */
  int ruleOffset;
  int maximumLength;

  if (length < 1) return 0;
  if (length == 1) {
    const ContractionTableCharacter *ctc = getContractionTableCharacter(toLowerCase(*src));
    if (!ctc) return 0;
    ruleOffset = ctc->rules;
    maximumLength = 1;
  } else {
    wchar_t characters[2];
    characters[0] = toLowerCase(src[0]);
    characters[1] = toLowerCase(src[1]);
    ruleOffset = table->rules[CTH(characters)];
    maximumLength = 0;
  }

  while (ruleOffset) {
    currentRule = CTR(table, ruleOffset);
    currentOpcode = currentRule->opcode;
    currentFindLength = currentRule->findlen;

    if ((length == 1) ||
        ((currentFindLength <= length) &&
         checkCurrentRule(src))) {
      setAfter(currentFindLength);

      if (!maximumLength) {
        typedef enum {CS_Any, CS_Lower, CS_UpperSingle, CS_UpperMultiple} CapitalizationState;
#define STATE(c) (testCharacter((c), CTC_UpperCase)? CS_UpperSingle: testCharacter((c), CTC_LowerCase)? CS_Lower: CS_Any)
        CapitalizationState current = STATE(before);
        int i;
        maximumLength = currentFindLength;

        for (i=0; i<currentFindLength; ++i) {
          wchar_t character = src[i];
          CapitalizationState next = STATE(character);

          if ((i > 0) &&
              (((current == CS_Lower) && (next == CS_UpperSingle)) ||
               ((current == CS_UpperMultiple) && (next == CS_Lower)))) {
            maximumLength = i;
            break;
          }

          if ((current > CS_Lower) && (next == CS_UpperSingle)) {
            current = CS_UpperMultiple;
          } else if (next != CS_Any) {
            current = next;
          } else if (current == CS_Any) {
            current = CS_Lower;
          }
        }

#undef STATE
      }

      if ((currentFindLength <= maximumLength) &&
          (!currentRule->after || testCharacter(before, currentRule->after)) &&
          (!currentRule->before || testCharacter(after, currentRule->before))) {
        switch (currentOpcode) {
          case CTO_Always:
          case CTO_Repeated:
          case CTO_Literal:
            return 1;
          case CTO_LargeSign:
          case CTO_LastLargeSign:
            if (!testCharacter(before, CTC_Space) || !testCharacter(after, CTC_Space))
              currentOpcode = CTO_Always;
            return 1;
          case CTO_WholeWord:
          case CTO_Contraction:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_LowWord:
            if (testCharacter(before, CTC_Space) && testCharacter(after, CTC_Space) &&
                (previousOpcode != CTO_JoinableWord) &&
                ((dest == destmin) || !dest[-1]))
              return 1;
            break;
          case CTO_JoinableWord:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                (before != '-') &&
                testCharacter(after, CTC_Space) &&
                (dest + currentRule->replen < destmax)) {
              const wchar_t *ptr = src + currentFindLength + 1;
              while (ptr < srcmax) {
                if (!testCharacter(*ptr, CTC_Space)) {
                  if (testCharacter(*ptr, CTC_Letter)) return 1;
                  break;
                }
                ptr++;
              }
            }
            break;
          case CTO_SuffixableWord:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Space|CTC_Letter|CTC_Punctuation))
              return 1;
            break;
          case CTO_PrefixableWord:
            if (testCharacter(before, CTC_Space|CTC_Letter|CTC_Punctuation) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_BegWord:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Letter))
              return 1;
            break;
          case CTO_BegMidWord:
            if (testCharacter(before, CTC_Letter|CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Letter))
              return 1;
            break;
          case CTO_MidWord:
            if (testCharacter(before, CTC_Letter) && testCharacter(after, CTC_Letter))
              return 1;
            break;
          case CTO_MidEndWord:
            if (testCharacter(before, CTC_Letter) &&
                testCharacter(after, CTC_Letter|CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_EndWord:
            if (testCharacter(before, CTC_Letter) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          case CTO_BegNum:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Digit))
              return 1;
            break;
          case CTO_MidNum:
            if (testCharacter(before, CTC_Digit) && testCharacter(after, CTC_Digit))
              return 1;
            break;
          case CTO_EndNum:
            if (testCharacter(before, CTC_Digit) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;
          {
            int isPre;
          case CTO_PrePunc:
            isPre = 1;
            goto doPunc;
          case CTO_PostPunc:
            isPre = 0;
          doPunc:
            if (testCharacter(*src, CTC_Punctuation)) {
              const wchar_t *pre = src;
              const wchar_t *post = src + currentFindLength;
              while (--pre >= srcmin)
                if (!testCharacter(*pre, CTC_Punctuation))
                  break;
              while (post < srcmax) {
                if (!testCharacter(*post, CTC_Punctuation)) break;
                post++;
              }
              if (isPre) {
                if (((pre < srcmin) || testCharacter(*pre, CTC_Space)) &&
                    ((post < srcmax) && !testCharacter(*post, CTC_Space)))
                  return 1;
              } else {
                if (((pre >= srcmin) && !testCharacter(*pre, CTC_Space)) &&
                    ((post == srcmax) || testCharacter(*post, CTC_Space)))
                  return 1;
              }
            }
            break;
          }
          default:
            break;
        }
      }
    }				/*Done with checking this rule */

    ruleOffset = currentRule->next;
  }

  return 0;
}

static int
putBytes (const BYTE *bytes, int count) {
  if (dest + count > destmax) return 0;
  memcpy(dest, bytes, count);
  dest += count;
  return 1;
}

static int
putReplace (const ContractionTableRule *rule) {
  return putBytes((BYTE *)&rule->findrep[rule->findlen], rule->replen);
}

static int
putComputerBraille (wchar_t character) {
  BYTE cell = convertWcharToDots(textTable, character);
  return putBytes(&cell, 1);
}

static int
putCharacter (wchar_t character) {
  const ContractionTableCharacter *ctc = getContractionTableCharacter(character);
  if (ctc) {
    ContractionTableOffset offset = ctc->always;
    if (offset) {
      const ContractionTableRule *rule = CTR(table, offset);
      if (rule->replen) return putReplace(rule);
    }
  }
  return putComputerBraille(character);
}

static int
putSequence (ContractionTableOffset offset) {
  const BYTE *sequence = CTA(table, offset);
  return putBytes(sequence+1, *sequence);
}

int
contractText (
  void *contractionTable,
  const wchar_t *inputBuffer, int *inputLength,
  BYTE *outputBuffer, int *outputLength,
  int *offsetsMap, const int cursorOffset
) {
  const wchar_t *srcword = NULL;
  BYTE *destword = NULL; /* last word transla1ted */
  const wchar_t *literal = NULL;

  if (!(table = (ContractionTableHeader *) contractionTable)) return 0;
  srcmax = (srcmin = src = inputBuffer) + *inputLength;
  destmax = (destmin = dest = outputBuffer) + *outputLength;
  offsets = offsetsMap;
  previousOpcode = CTO_None;

  while (src < srcmax) { /*the main translation loop */
    setOffset();
    setBefore();

    if (literal)
      if (src >= literal)
        if (testCharacter(*src, CTC_Space) || testCharacter(src[-1], CTC_Space))
          literal = NULL;

    if ((!literal && selectRule(srcmax-src)) || selectRule(1)) {
      if ((cursorOffset >= (src - srcmin)) &&
          (cursorOffset < (src - srcmin + currentFindLength))) {
        currentOpcode = CTO_Literal;
      } 

      if (table->numberSign && previousOpcode != CTO_MidNum &&
                 !testCharacter(before, CTC_Digit) && testCharacter(*src, CTC_Digit)) {
        if (!putSequence(table->numberSign)) break;
      } else if (table->englishLetterSign && testCharacter(*src, CTC_Letter)) {
        if ((currentOpcode == CTO_Contraction) ||
            (currentOpcode != CTO_EndNum && testCharacter(before, CTC_Digit)) ||
            (currentOpcode == CTO_Always && currentFindLength == 1 && testCharacter(before, CTC_Space) &&
             (src + 1 == srcmax || testCharacter(src[1], CTC_Space) ||
              (testCharacter(src[1], CTC_Punctuation) && (src[1] != '.') && (src[1] != '\''))))) {
          if (!putSequence(table->englishLetterSign)) break;
        }
      }

      if (testCharacter(*src, CTC_UpperCase)) {
        if (!testCharacter(before, CTC_UpperCase)) {
          if (table->beginCapitalSign &&
              (src + 1 < srcmax) && testCharacter(src[1], CTC_UpperCase)) {
            if (!putSequence(table->beginCapitalSign)) break;
          } else if (table->capitalSign) {
            if (!putSequence(table->capitalSign)) break;
          }
        }
      } else if (testCharacter(*src, CTC_LowerCase)) {
        if (table->endCapitalSign && (src - 2 >= srcmin) &&
            testCharacter(src[-1], CTC_UpperCase) && testCharacter(src[-2], CTC_UpperCase)) {
          if (!putSequence(table->endCapitalSign)) break;
        }
      }

      /* pre processing */
      switch (currentOpcode) {
        case CTO_LargeSign:
        case CTO_LastLargeSign:
          if (previousOpcode == CTO_LargeSign) {
            int srcoff = src - srcmin;
            int destlen;
            while (dest > destmin && !dest[-1]) dest--;
            setOffset();
            destlen = dest - destmin;
            while (srcoff > 0) {
              int destoff = offsets[--srcoff];
              if (destoff != -1) {
                if (destoff < destlen) break;
                offsets[srcoff] = -1;
              }
            }
          }
          break;
        default:
          break;
      }				/*end of action */

      /* main processing */
      if (!literal && (currentOpcode == CTO_Literal)) {
        const wchar_t *srcorig = src;
        literal = src + currentFindLength;
        if (testCharacter(*src, CTC_Space)) continue;
        if (destword) {
          src = srcword;
          dest = destword;
        } else {
          src = srcmin;
          dest = destmin;
        }
        while (srcorig > src) offsets[--srcorig - srcmin] = -1;
        continue;
      } else if (currentRule->replen) {
        if (!putReplace(currentRule)) goto done;
        src += currentFindLength;
      } else {
        const wchar_t *srclim = src + currentFindLength;
        while (1) {
          if (!putCharacter(*src)) goto done;
          if (++src == srclim) break;
          setOffset();
        }
      }

      /* post processing */
      switch (currentOpcode) {
        case CTO_Repeated: {
          const wchar_t *srclim = srcmax - currentFindLength;
          setOffset();
          while ((src <= srclim) && checkCurrentRule(src)) {
            src += currentFindLength;
          }
          break;
        }
        case CTO_JoinableWord:
          while ((src < srcmax) && testCharacter(*src, CTC_Space)) src++;
          break;
        default:
          break;
      }
    } else {
      if (!putComputerBraille(*src)) break;
      src++;
    }

    if (((src > srcmin) && testCharacter(src[-1], CTC_Space) && (currentOpcode != CTO_JoinableWord))) {
      srcword = src;
      destword = dest;
    }
    if ((dest == destmin) || dest[-1])
      previousOpcode = currentOpcode;
  }				/*end of translation loop */
done:

  if ((src < srcmax) &&
      (destword != NULL) &&
      ((destmax - destword) <= ((destmax - destmin) / 4)) &&
      !testCharacter(*src, CTC_Space)) {
    src = srcword;
    dest = destword;
  }

  if (src < srcmax) {
    setOffset();
    while (testCharacter(*src, CTC_Space))
      if (++src == srcmax)
        break;
  }

  *inputLength = src - srcmin;
  *outputLength = dest - destmin;
  return 1;
}				/*translation completed */
