/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#ifdef HAVE_ICU
#include <unicode/uchar.h>
#endif /* HAVE_ICU */

#include "ctb.h"
#include "ctb_internal.h"
#include "ttb.h"
#include "prefs.h"
#include "unicode.h"
#include "charset.h"
#include "ascii.h"
#include "brldots.h"
#include "log.h"
#include "file.h"
#include "parse.h"

static ContractionTable *table;
static const wchar_t *src, *srcmin, *srcmax, *cursor;
static BYTE *dest, *destmin, *destmax;
static int *offsets;

static wchar_t before, after;	/*the characters before and after a string */
static int currentFindLength;		/*length of current find string */
static ContractionTableOpcode currentOpcode;
static ContractionTableOpcode previousOpcode;
static const ContractionTableRule *currentRule;	/*pointer to current rule in table */

static inline void
assignOffset (size_t value) {
  if (offsets) offsets[src - srcmin] = value;
}

static inline void
setOffset (void) {
  assignOffset(dest - destmin);
}

static inline void
clearOffset (void) {
  assignOffset(CTB_NO_OFFSET);
}

static inline ContractionTableHeader *
getContractionTableHeader (void) {
  return table->data.internal.header.fields;
}

static inline const void *
getContractionTableItem (ContractionTableOffset offset) {
  return &table->data.internal.header.bytes[offset];
}

static const ContractionTableCharacter *
getContractionTableCharacter (wchar_t character) {
  const ContractionTableCharacter *characters = getContractionTableItem(getContractionTableHeader()->characters);
  int first = 0;
  int last = getContractionTableHeader()->characterCount - 1;

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

static CharacterEntry *
getCharacterEntry (wchar_t character) {
  int first = 0;
  int last = table->characters.count - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    CharacterEntry *entry = &table->characters.array[current];

    if (entry->value < character) {
      first = current + 1;
    } else if (entry->value > character) {
      last = current - 1;
    } else {
      return entry;
    }
  }

  if (table->characters.count == table->characters.size) {
    int newSize = table->characters.size;
    newSize = newSize? newSize<<1: 0X80;

    {
      CharacterEntry *newArray = realloc(table->characters.array, (newSize * sizeof(*newArray)));

      if (!newArray) {
        logMallocError();
        return NULL;
      }

      table->characters.array = newArray;
      table->characters.size = newSize;
    }
  }

  memmove(&table->characters.array[first+1],
          &table->characters.array[first],
          (table->characters.count - first) * sizeof(*table->characters.array));
  table->characters.count += 1;

  {
    CharacterEntry *entry = &table->characters.array[first];
    memset(entry, 0, sizeof(*entry));
    entry->value = entry->uppercase = entry->lowercase = character;

    if (iswspace(character)) {
      entry->attributes |= CTC_Space;
    } else if (iswalpha(character)) {
      entry->attributes |= CTC_Letter;

      if (iswupper(character)) {
        entry->attributes |= CTC_UpperCase;
        entry->lowercase = towlower(character);
      }

      if (iswlower(character)) {
        entry->attributes |= CTC_LowerCase;
        entry->uppercase = towupper(character);
      }
    } else if (iswdigit(character)) {
      entry->attributes |= CTC_Digit;
    } else if (iswpunct(character)) {
      entry->attributes |= CTC_Punctuation;
    }

    if (!table->command) {
      const ContractionTableCharacter *ctc = getContractionTableCharacter(character);
      if (ctc) entry->attributes |= ctc->attributes;
    }

    return entry;
  }
}

static int
testCharacter (wchar_t character, ContractionTableCharacterAttributes attributes) {
  const CharacterEntry *entry = getCharacterEntry(character);
  return entry && (attributes & entry->attributes);
}

static wchar_t
toLowerCase (wchar_t character) {
  const CharacterEntry *entry = getCharacterEntry(character);
  return entry? entry->lowercase: character;
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
isBeginning (void) {
  const wchar_t *ptr = src;

  while (ptr > srcmin) {
    if (!testCharacter(*--ptr, CTC_Punctuation)) {
      if (!testCharacter(*ptr, CTC_Space)) return 0;
      break;
    }
  }

  return 1;
}

static int
isEnding (void) {
  const wchar_t *ptr = src + currentFindLength;

  while (ptr < srcmax) {
    if (!testCharacter(*ptr, CTC_Punctuation)) {
      if (!testCharacter(*ptr, CTC_Space)) return 0;
      break;
    }

    ptr += 1;
  }

  return 1;
}

static int
selectRule (int length) {
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
    ruleOffset = getContractionTableHeader()->rules[CTH(characters)];
    maximumLength = 0;
  }

  while (ruleOffset) {
    currentRule = getContractionTableItem(ruleOffset);
    currentOpcode = currentRule->opcode;
    currentFindLength = currentRule->findlen;

    if ((length == 1) ||
        ((currentFindLength <= length) &&
         checkCurrentRule(src))) {
      setAfter(currentFindLength);

      if (!maximumLength) {
        maximumLength = currentFindLength;

        if (prefs.capitalizationMode != CTB_CAP_NONE) {
          typedef enum {CS_Any, CS_Lower, CS_UpperSingle, CS_UpperMultiple} CapitalizationState;
#define STATE(c) (testCharacter((c), CTC_UpperCase)? CS_UpperSingle: testCharacter((c), CTC_LowerCase)? CS_Lower: CS_Any)

          CapitalizationState current = STATE(before);
          int i;

          for (i=0; i<currentFindLength; i+=1) {
            wchar_t character = src[i];
            CapitalizationState next = STATE(character);

            if (i > 0) {
              if (((current == CS_Lower) && (next == CS_UpperSingle)) ||
                  ((current == CS_UpperMultiple) && (next == CS_Lower))) {
                maximumLength = i;
                break;
              }

              if ((prefs.capitalizationMode != CTB_CAP_SIGN) &&
                  (next == CS_UpperSingle)) {
                maximumLength = i;
                break;
              }
            }

            if ((prefs.capitalizationMode == CTB_CAP_SIGN) && (current > CS_Lower) && (next == CS_UpperSingle)) {
              current = CS_UpperMultiple;
            } else if (next != CS_Any) {
              current = next;
            } else if (current == CS_Any) {
              current = CS_Lower;
            }
          }

#undef STATE
        }
      }

      if ((currentFindLength <= maximumLength) &&
          (!currentRule->after || testCharacter(before, currentRule->after)) &&
          (!currentRule->before || testCharacter(after, currentRule->before))) {
        switch (currentOpcode) {
          case CTO_Always:
          case CTO_Repeatable:
          case CTO_Literal:
            return 1;

          case CTO_LargeSign:
          case CTO_LastLargeSign:
            if (!isBeginning() || !isEnding()) currentOpcode = CTO_Always;
            return 1;

          case CTO_WholeWord:
          case CTO_Contraction:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                testCharacter(after, CTC_Space|CTC_Punctuation))
              return 1;
            break;

          case CTO_LowWord:
            if (testCharacter(before, CTC_Space) && testCharacter(after, CTC_Space) &&
                (previousOpcode != CTO_JoinedWord) &&
                ((dest == destmin) || !dest[-1]))
              return 1;
            break;

          case CTO_JoinedWord:
            if (testCharacter(before, CTC_Space|CTC_Punctuation) &&
                (before != '-') &&
                (dest + currentRule->replen < destmax)) {
              const wchar_t *end = src + currentFindLength;
              const wchar_t *ptr = end;

              while (ptr < srcmax) {
                if (!testCharacter(*ptr, CTC_Space)) {
                  if (!testCharacter(*ptr, CTC_Letter)) break;
                  if (ptr == end) break;
                  return 1;
                }

                if (ptr++ == cursor) break;
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

          case CTO_PrePunc:
            if (testCharacter(*src, CTC_Punctuation) && isBeginning() && !isEnding()) return 1;
            break;

          case CTO_PostPunc:
            if (testCharacter(*src, CTC_Punctuation) && !isBeginning() && isEnding()) return 1;
            break;

          default:
            break;
        }
      }
    }

    ruleOffset = currentRule->next;
  }

  return 0;
}

static int
putCells (const BYTE *cells, int count) {
  if (dest + count > destmax) return 0;
  memcpy(dest, cells, count);
  dest += count;
  return 1;
}

static int
putCell (BYTE byte) {
  return putCells(&byte, 1);
}

static int
putReplace (const ContractionTableRule *rule, wchar_t character) {
  const BYTE *cells = (BYTE *)&rule->findrep[rule->findlen];
  int count = rule->replen;

  if ((prefs.capitalizationMode == CTB_CAP_DOT7) &&
      testCharacter(character, CTC_UpperCase)) {
    if (!putCell(*cells++ | BRL_DOT7)) return 0;
    if (!(count -= 1)) return 1;
  }

  return putCells(cells, count);
}

static const ContractionTableRule *
getAlwaysRule (wchar_t character) {
  const ContractionTableCharacter *ctc = getContractionTableCharacter(character);
  if (ctc) {
    ContractionTableOffset offset = ctc->always;
    if (offset) {
      const ContractionTableRule *rule = getContractionTableItem(offset);
      if (rule->replen) return rule;
    }
  }
  return NULL;
}

typedef struct {
  const ContractionTableRule *rule;
} SetCharacterRuleData;

static int
setCharacterRule (wchar_t character, void *data) {
  const ContractionTableRule *rule = getAlwaysRule(character);

  if (rule) {
    SetCharacterRuleData *scr = data;
    scr->rule = rule;
    return 1;
  }

  return 0;
}

static int
putCharacter (wchar_t character) {
  {
    SetCharacterRuleData scr = {
      .rule = NULL
    };

    if (handleBestCharacter(character, setCharacterRule, &scr)) {
      return putReplace(scr.rule, character);
    }
  }

  {
#ifdef HAVE_WCHAR_H 
    const wchar_t replacementCharacter = UNICODE_REPLACEMENT_CHARACTER;
    if (getCharacterWidth(character) == 0) return 1;
#else /* HAVE_WCHAR_H */
    const wchar_t replacementCharacter = SUB;
#endif /* HAVE_WCHAR_H */

    if (replacementCharacter != character) {
      const ContractionTableRule *rule = getAlwaysRule(replacementCharacter);
      if (rule) return putReplace(rule, character);
    }
  }

  return putCell(BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8);
}

static int
putSequence (ContractionTableOffset offset) {
  const BYTE *sequence = getContractionTableItem(offset);
  return putCells(sequence+1, *sequence);
}

#ifdef HAVE_ICU
typedef struct {
  unsigned int index;
  ULineBreak after;
  ULineBreak before;
  ULineBreak previous;
  ULineBreak indirect;
} LineBreakOpportunitiesState;

static void
prepareLineBreakOpportunitiesState (LineBreakOpportunitiesState *lbo) {
  lbo->index = 0;
  lbo->after = U_LB_SPACE;
  lbo->before = lbo->after;
  lbo->previous = lbo->before;
  lbo->indirect = U_LB_SPACE;
}

static void
findLineBreakOpportunities (
  LineBreakOpportunitiesState *lbo,
  unsigned char *opportunities,
  const wchar_t *characters, unsigned int limit
) {
  /* UAX #14: Line Breaking Properties
   * http://unicode.org/reports/tr14/
   * Section 6: Line Breaking Algorithm
   *
   * !  Mandatory break at the indicated position
   * ^  No break allowed at the indicated position
   * _  Break allowed at the indicated position
   *
   * H  ideographs
   * h  small kana
   * 9  digits
   */

  while (lbo->index <= limit) {
    unsigned char *opportunity = &opportunities[lbo->index];

    lbo->previous = lbo->before;
    lbo->before = lbo->after;
    lbo->after = u_getIntPropertyValue(characters[lbo->index], UCHAR_LINE_BREAK);
    lbo->index += 1;

    /* LB9  Do not break a combining character sequence.
     */
    if (lbo->after == U_LB_COMBINING_MARK) {
      /* LB10: Treat any remaining combining mark as AL.
       */
      if ((lbo->before == U_LB_MANDATORY_BREAK) ||
          (lbo->before == U_LB_CARRIAGE_RETURN) ||
          (lbo->before == U_LB_LINE_FEED) ||
          (lbo->before == U_LB_NEXT_LINE) ||
          (lbo->before == U_LB_SPACE) ||
          (lbo->before == U_LB_ZWSPACE)) {
        lbo->before = U_LB_ALPHABETIC;
      }

      /* treat it as if it has the line breaking class of the base character
       */
      lbo->after = lbo->before;
      *opportunity = 0;
      continue;
    }

    if (lbo->before != U_LB_SPACE) lbo->indirect = lbo->before;

    /* LB2: Never break at the start of text.
     * sot ×
     */
    if (opportunity == opportunities) {
      *opportunity = 0;
      continue;
    }

    /* LB4: Always break after hard line breaks
     * BK !
     */
    if (lbo->before == U_LB_MANDATORY_BREAK) {
      *opportunity = 1;
      continue;
    }

    /* LB5: Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks.
     * CR ^ LF
     * CR !
     * LF !
     * NL !
     */
    if ((lbo->before == U_LB_CARRIAGE_RETURN) && (lbo->after == U_LB_LINE_FEED)) {
      *opportunity = 0;
      continue;
    }
    if ((lbo->before == U_LB_CARRIAGE_RETURN) ||
        (lbo->before == U_LB_LINE_FEED) ||
        (lbo->before == U_LB_NEXT_LINE)) {
      *opportunity = 1;
      continue;
    }

    /* LB6: Do not break before hard line breaks.
     * ^ ( BK | CR | LF | NL )
     */
    if ((lbo->after == U_LB_MANDATORY_BREAK) ||
        (lbo->after == U_LB_CARRIAGE_RETURN) ||
        (lbo->after == U_LB_LINE_FEED) ||
        (lbo->after == U_LB_NEXT_LINE)) {
      *opportunity = 0;
      continue;
    }

    /* LB7: Do not break before spaces or zero width space.
     * ^ SP
     * ^ ZW
     */
    if ((lbo->after == U_LB_SPACE) || (lbo->after == U_LB_ZWSPACE)) {
      *opportunity = 0;
      continue;
    }

    /* LB8: Break after zero width space.
     * ZW _
     */
    if (lbo->before == U_LB_ZWSPACE) {
      *opportunity = 1;
      continue;
    }

    /* LB11: Do not break before or after Word joiner and related characters.
     * ^ WJ
     * WJ ^
     */
    if ((lbo->before == U_LB_WORD_JOINER) || (lbo->after == U_LB_WORD_JOINER)) {
      *opportunity = 0;
      continue;
    }

    /* LB12: Do not break before or after NBSP and related characters.
     * [^SP] ^ GL
     * GL ^
     */
    if ((lbo->before != U_LB_SPACE) && (lbo->after == U_LB_GLUE)) {
      *opportunity = 0;
      continue;
    }
    if (lbo->before == U_LB_GLUE) {
      *opportunity = 0;
      continue;
    }

    /* LB13: Do not break before ‘]' or ‘!' or ‘;' or ‘/', even after spaces.
     * ^ CL
     * ^ EX
     * ^ IS
     * ^ SY
     */
    if ((lbo->after == U_LB_CLOSE_PUNCTUATION) ||
        (lbo->after == U_LB_EXCLAMATION) ||
        (lbo->after == U_LB_INFIX_NUMERIC) ||
        (lbo->after == U_LB_BREAK_SYMBOLS)) {
      *opportunity = 0;
      continue;
    }

    /* LB14: Do not break after ‘[', even after spaces.
     * OP SP* ^
     */
    if (lbo->indirect == U_LB_OPEN_PUNCTUATION) {
      *opportunity = 0;
      continue;
    }

    /* LB15: Do not break within ‘"[', even with intervening spaces.
     * QU SP* ^ OP
     */
    if ((lbo->indirect == U_LB_QUOTATION) && (lbo->after == U_LB_OPEN_PUNCTUATION)) {
      *opportunity = 0;
      continue;
    }

    /* LB16: Do not break within ‘]h', even with intervening spaces.
     * CL SP* ^ NS
     */
    if ((lbo->indirect == U_LB_CLOSE_PUNCTUATION) && (lbo->after == U_LB_NONSTARTER)) {
      *opportunity = 0;
      continue;
    }

    /* LB17: Do not break within ‘ــ', even with intervening spaces.
     * B2 SP* ^ B2
     */
    if ((lbo->indirect == U_LB_BREAK_BOTH) && (lbo->after == U_LB_BREAK_BOTH)) {
      *opportunity = 0;
      continue;
    }

    /* LB18: Break after spaces.
     * SP _
     */
    if (lbo->before == U_LB_SPACE) {
      *opportunity = 1;
      continue;
    }

    /* LB19: Do not break before or after  quotation marks.
     * ^ QU
     * QU ^
     */
    if ((lbo->before == U_LB_QUOTATION) || (lbo->after == U_LB_QUOTATION)) {
      *opportunity = 0;
      continue;
    }

    /* LB20: Break before and after unresolved.
     * _ CB
     * CB _
     */
    if ((lbo->after == U_LB_CONTINGENT_BREAK) || (lbo->before == U_LB_CONTINGENT_BREAK)) {
      *opportunity = 1;
      continue;
    }

    /* LB21: Do not break before hyphen-minus, other hyphens,
     *       fixed-width spaces, small kana, and other non-starters,
     *       or lbo->after acute accents.
     * ^ BA
     * ^ HY
     * ^ NS
     * BB ^
     */
    if ((lbo->after == U_LB_BREAK_AFTER) ||
        (lbo->after == U_LB_HYPHEN) ||
        (lbo->after == U_LB_NONSTARTER) ||
        (lbo->before == U_LB_BREAK_BEFORE)) {
      *opportunity = 0;
      continue;
    }

    /* LB22: Do not break between two ellipses,
     *       or between letters or numbers and ellipsis.
     * AL ^ IN
     * ID ^ IN
     * IN ^ IN
     * NU ^ IN
     */
    if ((lbo->after == U_LB_INSEPARABLE) &&
        ((lbo->before == U_LB_ALPHABETIC) ||
         (lbo->before == U_LB_IDEOGRAPHIC) ||
         (lbo->before == U_LB_INSEPARABLE) ||
         (lbo->before == U_LB_NUMERIC))) {
      *opportunity = 0;
      continue;
    }

    /* LB23: Do not break within ‘a9', ‘3a', or ‘H%'.
     * ID ^ PO
     * AL ^ NU
     * NU ^ AL
     */
    if (((lbo->before == U_LB_IDEOGRAPHIC) && (lbo->after == U_LB_POSTFIX_NUMERIC)) ||
        ((lbo->before == U_LB_ALPHABETIC) && (lbo->after == U_LB_NUMERIC)) ||
        ((lbo->before == U_LB_NUMERIC) && (lbo->after == U_LB_ALPHABETIC))) {
      *opportunity = 0;
      continue;
    }

    /* LB24: Do not break between prefix and letters or ideographs.
     * PR ^ ID
     * PR ^ AL
     * PO ^ AL
     */
    if (((lbo->before == U_LB_PREFIX_NUMERIC) && (lbo->after == U_LB_IDEOGRAPHIC)) ||
        ((lbo->before == U_LB_PREFIX_NUMERIC) && (lbo->after == U_LB_ALPHABETIC)) ||
        ((lbo->before == U_LB_POSTFIX_NUMERIC) && (lbo->after == U_LB_ALPHABETIC))) {
      *opportunity = 0;
      continue;
    }

    /* LB25:  Do not break between the following pairs of classes relevant to numbers:
     * CL ^ PO
     * CL ^ PR
     * NU ^ PO
     * NU ^ PR
     * PO ^ OP
     * PO ^ NU
     * PR ^ OP
     * PR ^ NU
     * HY ^ NU
     * IS ^ NU
     * NU ^ NU
     * SY ^ NU
     */
    if (((lbo->before == U_LB_CLOSE_PUNCTUATION) && (lbo->after == U_LB_POSTFIX_NUMERIC)) ||
        ((lbo->before == U_LB_CLOSE_PUNCTUATION) && (lbo->after == U_LB_PREFIX_NUMERIC)) ||
        ((lbo->before == U_LB_NUMERIC) && (lbo->after == U_LB_POSTFIX_NUMERIC)) ||
        ((lbo->before == U_LB_NUMERIC) && (lbo->after == U_LB_PREFIX_NUMERIC)) ||
        ((lbo->before == U_LB_POSTFIX_NUMERIC) && (lbo->after == U_LB_OPEN_PUNCTUATION)) ||
        ((lbo->before == U_LB_POSTFIX_NUMERIC) && (lbo->after == U_LB_NUMERIC)) ||
        ((lbo->before == U_LB_PREFIX_NUMERIC) && (lbo->after == U_LB_OPEN_PUNCTUATION)) ||
        ((lbo->before == U_LB_PREFIX_NUMERIC) && (lbo->after == U_LB_NUMERIC)) ||
        ((lbo->before == U_LB_HYPHEN) && (lbo->after == U_LB_NUMERIC)) ||
        ((lbo->before == U_LB_INFIX_NUMERIC) && (lbo->after == U_LB_NUMERIC)) ||
        ((lbo->before == U_LB_NUMERIC) && (lbo->after == U_LB_NUMERIC)) ||
        ((lbo->before == U_LB_BREAK_SYMBOLS) && (lbo->after == U_LB_NUMERIC))) {
      *opportunity = 0;
      continue;
    }

    /* LB26: Do not break a Korean syllable.
     * JL ^ (JL | JV | H2 | H3)
     * (JV | H2) ^ (JV | JT)
     * (JT | H3) ^ JT
     */
    if ((lbo->before == U_LB_JL) &&
        ((lbo->after == U_LB_JL) ||
         (lbo->after == U_LB_JV) ||
         (lbo->after == U_LB_H2) ||
         (lbo->after == U_LB_H3))) {
      *opportunity = 0;
      continue;
    }
    if (((lbo->before == U_LB_JV) || (lbo->before == U_LB_H2)) &&
        ((lbo->after == U_LB_JV) || (lbo->after == U_LB_JT))) {
      *opportunity = 0;
      continue;
    }
    if (((lbo->before == U_LB_JT) || (lbo->before == U_LB_H3)) &&
        (lbo->after == U_LB_JT)) {
      *opportunity = 0;
      continue;
    }

    /* LB27: Treat a Korean Syllable Block the same as ID.
     * (JL | JV | JT | H2 | H3) ^ IN
     * (JL | JV | JT | H2 | H3) ^ PO
     * PR ^ (JL | JV | JT | H2 | H3)
     */
    if (((lbo->before == U_LB_JL) || (lbo->before == U_LB_JV) || (lbo->before == U_LB_JT) ||
         (lbo->before == U_LB_H2) || (lbo->before == U_LB_H3)) &&
        (lbo->after == U_LB_INSEPARABLE)) {
      *opportunity = 0;
      continue;
    }
    if (((lbo->before == U_LB_JL) || (lbo->before == U_LB_JV) || (lbo->before == U_LB_JT) ||
         (lbo->before == U_LB_H2) || (lbo->before == U_LB_H3)) &&
        (lbo->after == U_LB_POSTFIX_NUMERIC)) {
      *opportunity = 0;
      continue;
    }
    if ((lbo->before == U_LB_PREFIX_NUMERIC) &&
        ((lbo->after == U_LB_JL) || (lbo->after == U_LB_JV) || (lbo->after == U_LB_JT) ||
         (lbo->after == U_LB_H2) || (lbo->after == U_LB_H3))) {
      *opportunity = 0;
      continue;
    }

    /* LB28: Do not break between alphabetics.
     * AL ^ AL
     */
    if ((lbo->before == U_LB_ALPHABETIC) && (lbo->after == U_LB_ALPHABETIC)) {
      *opportunity = 0;
      continue;
    }

    /* LB29: Do not break between numeric punctuation and alphabetics.
     * IS ^ AL
     */
    if ((lbo->before == U_LB_INFIX_NUMERIC) && (lbo->after == U_LB_ALPHABETIC)) {
      *opportunity = 0;
      continue;
    }

    /* LB30: Do not break between letters, numbers, or ordinary symbols
     *       and opening or closing punctuation.
     * (AL | NU) ^ OP
     * CL ^ (AL | NU)
     */
    if (((lbo->before == U_LB_ALPHABETIC) || (lbo->before == U_LB_NUMERIC)) &&
        (lbo->after == U_LB_OPEN_PUNCTUATION)) {
      *opportunity = 0;
      continue;
    }
    if ((lbo->before == U_LB_CLOSE_PUNCTUATION) &&
        ((lbo->after == U_LB_ALPHABETIC) || (lbo->after == U_LB_NUMERIC))) {
      *opportunity = 0;
      continue;
    }

    /* Unix options begin with a minus sign. */
    if ((lbo->before == U_LB_HYPHEN) &&
        (lbo->after != U_LB_SPACE) &&
        (lbo->previous == U_LB_SPACE)) {
      *opportunity = 0;
      continue;
    }

    /* LB31: Break everywhere else.
     * ALL _
     * _ ALL
     */
    *opportunity = 1;
  }
}

#else /* HAVE_ICU */
typedef struct {
  unsigned int index;
  int wasSpace;
} LineBreakOpportunitiesState;

static void
prepareLineBreakOpportunitiesState (LineBreakOpportunitiesState *lbo) {
  lbo->index = 0;
  lbo->wasSpace = 0;
}

static void
findLineBreakOpportunities (
  LineBreakOpportunitiesState *lbo,
  unsigned char *opportunities,
  const wchar_t *characters, unsigned int limit
) {
  while (lbo->index <= limit) {
    int isSpace = testCharacter(characters[lbo->index], CTC_Space);
    opportunities[lbo->index] = lbo->wasSpace && !isSpace;

    lbo->wasSpace = isSpace;
    lbo->index += 1;
  }
}
#endif /* HAVE_ICU */

static int
contractTextInternally (void) {
  const wchar_t *srcword = NULL;
  BYTE *destword = NULL;

  const wchar_t *srcjoin = NULL;
  BYTE *destjoin = NULL;

  BYTE *destlast = NULL;
  const wchar_t *literal = NULL;

  unsigned char lineBreakOpportunities[srcmax - srcmin];
  LineBreakOpportunitiesState lbo;

  prepareLineBreakOpportunitiesState(&lbo);
  previousOpcode = CTO_None;

  while (src < srcmax) {
    int wasLiteral = src == literal;

    destlast = dest;
    setOffset();
    setBefore();

    if (literal)
      if (src >= literal)
        if (testCharacter(*src, CTC_Space) || testCharacter(src[-1], CTC_Space))
          literal = NULL;

    if ((!literal && selectRule(srcmax-src)) || selectRule(1)) {
      if (!literal &&
          ((currentOpcode == CTO_Literal) ||
           (prefs.expandCurrentWord && (cursor >= src) && (cursor < (src + currentFindLength))))) {
        literal = src + currentFindLength;

        if (!testCharacter(*src, CTC_Space)) {
          if (destjoin) {
            src = srcjoin;
            dest = destjoin;
          } else {
            src = srcmin;
            dest = destmin;
          }
        }

        continue;
      }

      if (getContractionTableHeader()->numberSign && (previousOpcode != CTO_MidNum) &&
          !testCharacter(before, CTC_Digit) && testCharacter(*src, CTC_Digit)) {
        if (!putSequence(getContractionTableHeader()->numberSign)) break;
      } else if (getContractionTableHeader()->englishLetterSign && testCharacter(*src, CTC_Letter)) {
        if ((currentOpcode == CTO_Contraction) ||
            ((currentOpcode != CTO_EndNum) && testCharacter(before, CTC_Digit)) ||
            (testCharacter(*src, CTC_Letter) &&
             (currentOpcode == CTO_Always) &&
             (currentFindLength == 1) &&
             testCharacter(before, CTC_Space) &&
             (((src + 1) == srcmax) ||
              testCharacter(src[1], CTC_Space) ||
              (testCharacter(src[1], CTC_Punctuation) && (src[1] != '.') && (src[1] != '\''))))) {
          if (!putSequence(getContractionTableHeader()->englishLetterSign)) break;
        }
      }

      if (prefs.capitalizationMode == CTB_CAP_SIGN) {
        if (testCharacter(*src, CTC_UpperCase)) {
          if (!testCharacter(before, CTC_UpperCase)) {
            if (getContractionTableHeader()->beginCapitalSign &&
                (src + 1 < srcmax) && testCharacter(src[1], CTC_UpperCase)) {
              if (!putSequence(getContractionTableHeader()->beginCapitalSign)) break;
            } else if (getContractionTableHeader()->capitalSign) {
              if (!putSequence(getContractionTableHeader()->capitalSign)) break;
            }
          }
        } else if (testCharacter(*src, CTC_LowerCase)) {
          if (getContractionTableHeader()->endCapitalSign && (src - 2 >= srcmin) &&
              testCharacter(src[-1], CTC_UpperCase) && testCharacter(src[-2], CTC_UpperCase)) {
            if (!putSequence(getContractionTableHeader()->endCapitalSign)) break;
          }
        }
      }

      switch (currentOpcode) {
        case CTO_LargeSign:
        case CTO_LastLargeSign:
          if ((previousOpcode == CTO_LargeSign) && !wasLiteral) {
            while ((dest > destmin) && !dest[-1]) dest -= 1;
            setOffset();

            {
              BYTE **destptrs[] = {&destword, &destjoin, &destlast, NULL};
              BYTE ***destptr = destptrs;

              while (*destptr) {
                if (**destptr && (**destptr > dest)) **destptr = dest;
                destptr += 1;
              }
            }
          }
          break;

        default:
          break;
      }

      if (currentRule->replen &&
          !((currentOpcode == CTO_Always) && (currentFindLength == 1))) {
        const wchar_t *srcnxt = src + currentFindLength;
        if (!putReplace(currentRule, *src)) goto done;
        while (++src != srcnxt) clearOffset();
      } else {
        const wchar_t *srclim = src + currentFindLength;
        while (1) {
          if (!putCharacter(*src)) goto done;
          if (++src == srclim) break;
          setOffset();
        }
      }

      {
        const wchar_t *srcorig = src;
        const wchar_t *srcbeg = NULL;
        BYTE *destbeg = NULL;

        switch (currentOpcode) {
          case CTO_Repeatable: {
            const wchar_t *srclim = srcmax - currentFindLength;

            srcbeg = src - currentFindLength;
            destbeg = destlast;

            while ((src <= srclim) && checkCurrentRule(src)) {
              const wchar_t *srcnxt = src + currentFindLength;

              do {
                clearOffset();
              } while (++src != srcnxt);
            }

            break;
          }

          case CTO_JoinedWord:
            srcbeg = src;
            destbeg = dest;

            while ((src < srcmax) && testCharacter(*src, CTC_Space)) {
              clearOffset();
              src += 1;
            }
            break;

          default:
            break;
        }

        if (srcbeg && (cursor >= srcbeg) && (cursor < src)) {
          int repeat = !literal;
          literal = src;

          if (repeat) {
            src = srcbeg;
            dest = destbeg;
            continue;
          }

          src = srcorig;
        }
      }
    } else {
      currentOpcode = CTO_Always;
      if (!putCharacter(*src)) break;
      src += 1;
    }

    findLineBreakOpportunities(&lbo, lineBreakOpportunities, srcmin, src-srcmin);
    if (lineBreakOpportunities[src-srcmin]) {
      srcjoin = src;
      destjoin = dest;

      if (currentOpcode != CTO_JoinedWord) {
        srcword = src;
        destword = dest;
      }
    }

    if ((dest == destmin) || dest[-1]) {
      previousOpcode = currentOpcode;
    }
  }

done:
  if (src < srcmax) {
    if (destword && (destword > destmin) &&
        (!(testCharacter(src[-1], CTC_Space) || testCharacter(*src, CTC_Space)) ||
         (previousOpcode == CTO_JoinedWord))) {
      src = srcword;
      dest = destword;
    } else if (destlast) {
      dest = destlast;
    }
  }

  return 1;
}

static int
putExternalRequests (void) {
  typedef enum {
    REQ_TEXT,
    REQ_NUMBER
  } ExternalRequestType;

  typedef struct {
    const char *name;
    unsigned char type;

    union {
      struct {
        const wchar_t *start;
        size_t count;
      } text;

      unsigned int number;
    } value;
  } ExternalRequestEntry;

  const ExternalRequestEntry externalRequestTable[] = {
    { .name = "cursor-position",
      .type = REQ_NUMBER,
      .value.number = cursor? cursor-srcmin+1: 0
    },

    { .name = "expand-current-word",
      .type = REQ_NUMBER,
      .value.number = prefs.expandCurrentWord
    },

    { .name = "capitalization-mode",
      .type = REQ_NUMBER,
      .value.number = prefs.capitalizationMode
    },

    { .name = "maximum-length",
      .type = REQ_NUMBER,
      .value.number = destmax - destmin
    },

    { .name = "text",
      .type = REQ_TEXT,
      .value.text = {
        .start = srcmin,
        .count = srcmax - srcmin
      }
    },

    { .name = NULL }
  };

  FILE *stream = table->data.external.standardInput;
  const ExternalRequestEntry *req = externalRequestTable;

  while (req->name) {
    if (fputs(req->name, stream) == EOF) goto outputError;
    if (fputc('=', stream) == EOF) goto outputError;

    switch (req->type) {
      case REQ_TEXT: {
        const wchar_t *character = req->value.text.start;
        const wchar_t *end = character + req->value.text.count;

        while (character < end) {
          Utf8Buffer utf8;
          size_t utfs = convertWcharToUtf8(*character++, utf8);

          if (!utfs) return 0;
          if (fputs(utf8, stream) == EOF) goto outputError;
        }

        break;
      }

      case REQ_NUMBER:
        if (fprintf(stream, "%u", req->value.number) == EOF) goto outputError;
        break;

      default:
        logMessage(LOG_WARNING, "unimplemented external contraction request property type: %s: %u (%s)", table->command, req->type, req->name);
        return 0;
    }

    if (fputc('\n', stream) == EOF) goto outputError;
    req += 1;
  }

  if (fflush(stream) == EOF) goto outputError;
  return 1;

outputError:
  logMessage(LOG_WARNING, "external contraction output error: %s: %s", table->command, strerror(errno));
  return 0;
}

static const unsigned char brfTable[0X40] = {
  /* 0X20   */ 0,
  /* 0X21 ! */ BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6,
  /* 0X22 " */ BRL_DOT5,
  /* 0X23 # */ BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6,
  /* 0X24 $ */ BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6,
  /* 0X25 % */ BRL_DOT1 | BRL_DOT4 | BRL_DOT6,
  /* 0X26 & */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6,
  /* 0X27 ' */ BRL_DOT3,
  /* 0X28 ( */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6,
  /* 0X29 ) */ BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6,
  /* 0X2A * */ BRL_DOT1 | BRL_DOT6,
  /* 0X2B + */ BRL_DOT3 | BRL_DOT4 | BRL_DOT6,
  /* 0X2C , */ BRL_DOT6,
  /* 0X2D - */ BRL_DOT3 | BRL_DOT6,
  /* 0X2E . */ BRL_DOT4 | BRL_DOT6,
  /* 0X2F / */ BRL_DOT3 | BRL_DOT4,
  /* 0X30 0 */ BRL_DOT3 | BRL_DOT5 | BRL_DOT6,
  /* 0X31 1 */ BRL_DOT2,
  /* 0X32 2 */ BRL_DOT2 | BRL_DOT3,
  /* 0X33 3 */ BRL_DOT2 | BRL_DOT5,
  /* 0X34 4 */ BRL_DOT2 | BRL_DOT5 | BRL_DOT6,
  /* 0X35 5 */ BRL_DOT2 | BRL_DOT6,
  /* 0X36 6 */ BRL_DOT2 | BRL_DOT3 | BRL_DOT5,
  /* 0X37 7 */ BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6,
  /* 0X38 8 */ BRL_DOT2 | BRL_DOT3 | BRL_DOT6,
  /* 0X39 9 */ BRL_DOT3 | BRL_DOT5,
  /* 0X3A : */ BRL_DOT1 | BRL_DOT5 | BRL_DOT6,
  /* 0X3B ; */ BRL_DOT5 | BRL_DOT6,
  /* 0X3C < */ BRL_DOT1 | BRL_DOT2 | BRL_DOT6,
  /* 0X3D = */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6,
  /* 0X3E > */ BRL_DOT3 | BRL_DOT4 | BRL_DOT5,
  /* 0X3F ? */ BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6,
  /* 0X40 @ */ BRL_DOT4,
  /* 0X41 A */ BRL_DOT1,
  /* 0X42 B */ BRL_DOT1 | BRL_DOT2,
  /* 0X43 C */ BRL_DOT1 | BRL_DOT4,
  /* 0X44 D */ BRL_DOT1 | BRL_DOT4 | BRL_DOT5,
  /* 0X45 E */ BRL_DOT1 | BRL_DOT5,
  /* 0X46 F */ BRL_DOT1 | BRL_DOT2 | BRL_DOT4,
  /* 0X47 G */ BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5,
  /* 0X48 H */ BRL_DOT1 | BRL_DOT2 | BRL_DOT5,
  /* 0X49 I */ BRL_DOT2 | BRL_DOT4,
  /* 0X4A J */ BRL_DOT2 | BRL_DOT4 | BRL_DOT5,
  /* 0X4B K */  BRL_DOT1 | BRL_DOT3,
  /* 0X4C L */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3,
  /* 0X4D M */ BRL_DOT1 | BRL_DOT3 | BRL_DOT4,
  /* 0X4E N */ BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5,
  /* 0X4F O */ BRL_DOT1 | BRL_DOT3 | BRL_DOT5,
  /* 0X50 P */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4,
  /* 0X51 Q */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5,
  /* 0X52 R */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5,
  /* 0X53 S */ BRL_DOT2 | BRL_DOT3 | BRL_DOT4,
  /* 0X54 T */ BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5,
  /* 0X55 U */ BRL_DOT1 | BRL_DOT3 | BRL_DOT6,
  /* 0X56 V */ BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6,
  /* 0X57 W */ BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6,
  /* 0X58 X */ BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6,
  /* 0X59 Y */ BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6,
  /* 0X5A Z */ BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6,
  /* 0X5B [ */ BRL_DOT2 | BRL_DOT4 | BRL_DOT6,
  /* 0X5C \ */ BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6,
  /* 0X5D ] */ BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6,
  /* 0X5E ^ */ BRL_DOT4 | BRL_DOT5,
  /* 0X5F _ */ BRL_DOT4 | BRL_DOT5 | BRL_DOT6
};

static int
handleExternalResponse_brf (const char *value) {
  int useDot7 = prefs.capitalizationMode == CTB_CAP_DOT7;

  while (*value && (dest < destmax)) {
    unsigned char brf = *value++ & 0XFF;
    unsigned char dots = 0;
    unsigned char superimpose = 0;

    if ((brf >= 0X60) && (brf <= 0X7F)) {
      brf -= 0X20;
    } else if ((brf >= 0X41) && (brf <= 0X5A)) {
      if (useDot7) superimpose |= BRL_DOT7;
    }

    if ((brf >= 0X20) && (brf <= 0X5F)) dots = brfTable[brf - 0X20] | superimpose;
    *dest++ = dots;
  }

  return 1;
}

static int
handleExternalResponse_consumedLength (const char *value) {
  int length;

  if (!isInteger(&length, value)) return 0;
  if (length < 1) return 0;
  if (length > (srcmax - srcmin)) return 0;

  src = srcmin + length;
  return 1;
}

static int
handleExternalResponse_outputOffsets (const char *value) {
  if (offsets) {
    int previous = CTB_NO_OFFSET;
    unsigned int count = srcmax - srcmin;
    unsigned int index = 0;

    while (*value && (index < count)) {
      int offset;

      {
        char *delimiter = strchr(value, ',');

        if (delimiter) {
          int ok;

          {
            char oldDelimiter = *delimiter;
            *delimiter = 0;
            ok = isInteger(&offset, value);
            *delimiter = oldDelimiter;
          }

          if (!ok) return 0;
          value = delimiter + 1;
        } else if (isInteger(&offset, value)) {
          value += strlen(value);
        } else {
          return 0;
        }
      }

      if (offset < ((index == 0)? 0: previous)) return 0;
      if (offset >= (destmax - destmin)) return 0;

      offsets[index++] = (offset == previous)? CTB_NO_OFFSET: offset;
      previous = offset;
    }
  }

  return 1;
}

typedef struct {
  const char *name;
  int (*handler) (const char *value);
  unsigned stop:1;
} ExternalResponseEntry;

static const ExternalResponseEntry externalResponseTable[] = {
  { .name = "brf",
    .stop = 1,
    .handler = handleExternalResponse_brf
  },

  { .name = "consumed-length",
    .handler = handleExternalResponse_consumedLength
  },

  { .name = "output-offsets",
    .handler = handleExternalResponse_outputOffsets
  },

  { .name = NULL }
};

static int
getExternalResponses (void) {
  static char *buffer = NULL;
  static size_t size = 0;

  FILE *stream = table->data.external.standardOutput;

  while (readLine(stream, &buffer, &size)) {
    int ok = 0;
    int stop = 0;
    char *delimiter = strchr(buffer, '=');

    if (delimiter) {
      const char *value = delimiter + 1;
      const ExternalResponseEntry *rsp = externalResponseTable;

      char oldDelimiter = *delimiter;
      *delimiter = 0;

      while (rsp->name) {
        if (strcmp(buffer, rsp->name) == 0) {
          if (rsp->handler(value)) ok = 1;
          if (rsp->stop) stop = 1;
          break;
        }

        rsp += 1;
      }

      *delimiter = oldDelimiter;
    }

    if (!ok) logMessage(LOG_WARNING, "unexpected external contraction response: %s: %s", table->command, buffer);
    if (stop) return 1;
  }

  logMessage(LOG_WARNING, "incomplete external contraction response: %s", table->command);
  return 0;
}

static int
contractTextExternally (void) {
  setOffset();
  while (++src < srcmax) clearOffset();

  if (startContractionCommand(table)) {
    if (putExternalRequests()) {
      if (getExternalResponses()) {
        return 1;
      }
    }
  }

  stopContractionCommand(table);
  return 0;
}

static inline unsigned int
makeCachedInputCount (void) {
  return srcmax - srcmin;
}

static inline unsigned int
makeCachedOutputMaximum (void) {
  return destmax - destmin;
}

static inline int
makeCachedCursorOffset (void) {
  return cursor? (cursor - srcmin): CTB_NO_CURSOR;
}

static int
checkCache (void) {
  if (!table->cache.input.characters) return 0;
  if (!table->cache.output.cells) return 0;
  if (offsets && !table->cache.offsets.count) return 0;
  if (table->cache.output.maximum != makeCachedOutputMaximum()) return 0;
  if (table->cache.cursorOffset != makeCachedCursorOffset()) return 0;
  if (table->cache.expandCurrentWord != prefs.expandCurrentWord) return 0;
  if (table->cache.capitalizationMode != prefs.capitalizationMode) return 0;

  {
    unsigned int count = makeCachedInputCount();
    if (table->cache.input.count != count) return 0;
    if (wmemcmp(srcmin, table->cache.input.characters, count) != 0) return 0;
  }

  return 1;
}

static void
updateCache (void) {
  {
    unsigned int count = makeCachedInputCount();

    if (count > table->cache.input.size) {
      unsigned int newSize = count | 0X7F;
      wchar_t *newCharacters = malloc(ARRAY_SIZE(newCharacters, newSize));

      if (!newCharacters) {
        logMallocError();
        table->cache.input.count = 0;
        goto inputDone;
      }

      if (table->cache.input.characters) free(table->cache.input.characters);
      table->cache.input.characters = newCharacters;
      table->cache.input.size = newSize;
    }

    wmemcpy(table->cache.input.characters, srcmin, count);
    table->cache.input.count = count;
    table->cache.input.consumed = src - srcmin;
  }
inputDone:

  {
    unsigned int count = dest - destmin;

    if (count > table->cache.output.size) {
      unsigned int newSize = count | 0X7F;
      unsigned char *newCells = malloc(ARRAY_SIZE(newCells, newSize));

      if (!newCells) {
        logMallocError();
        table->cache.output.count = 0;
        goto outputDone;
      }

      if (table->cache.output.cells) free(table->cache.output.cells);
      table->cache.output.cells = newCells;
      table->cache.output.size = newSize;
    }

    memcpy(table->cache.output.cells, destmin, count);
    table->cache.output.count = count;
    table->cache.output.maximum = makeCachedOutputMaximum();
  }
outputDone:

  if (offsets) {
    unsigned int count = makeCachedInputCount();

    if (count > table->cache.offsets.size) {
      unsigned int newSize = count | 0X7F;
      int *newArray = malloc(ARRAY_SIZE(newArray, newSize));

      if (!newArray) {
        logMallocError();
        table->cache.offsets.count = 0;
        goto offsetsDone;
      }

      if (table->cache.offsets.array) free(table->cache.offsets.array);
      table->cache.offsets.array = newArray;
      table->cache.offsets.size = newSize;
    }

    memcpy(table->cache.offsets.array, offsets, ARRAY_SIZE(offsets, count));
    table->cache.offsets.count = count;
  } else {
    table->cache.offsets.count = 0;
  }
offsetsDone:

  table->cache.cursorOffset = makeCachedCursorOffset();
  table->cache.expandCurrentWord = prefs.expandCurrentWord;
  table->cache.capitalizationMode = prefs.capitalizationMode;
}

void
contractText (
  ContractionTable *contractionTable,
  const wchar_t *inputBuffer, int *inputLength,
  BYTE *outputBuffer, int *outputLength,
  int *offsetsMap, const int cursorOffset
) {
  table = contractionTable;
  srcmax = (srcmin = src = inputBuffer) + *inputLength;
  destmax = (destmin = dest = outputBuffer) + *outputLength;
  offsets = offsetsMap;
  cursor = (cursorOffset == CTB_NO_CURSOR)? NULL: &src[cursorOffset];

  if (checkCache()) {
    src = srcmin + table->cache.input.consumed;
    if (offsets)
      memcpy(offsets, table->cache.offsets.array,
             ARRAY_SIZE(offsets, table->cache.offsets.count));

    dest = destmin + table->cache.output.count;
    memcpy(destmin, table->cache.output.cells,
           ARRAY_SIZE(destmin, table->cache.output.count));
  } else {
    if (!(table->command? contractTextExternally(): contractTextInternally())) {
      src = srcmin;
      dest = destmin;

      while ((src < srcmax) && (dest < destmax)) {
        setOffset();
        *dest++ = convertCharacterToDots(textTable, *src++);
      }
    }

    if (src < srcmax) {
      const wchar_t *srcorig = src;
      int done = 1;

      setOffset();
      while (1) {
        if (done && !testCharacter(*src, CTC_Space)) {
          done = 0;

          if (!cursor || (cursor < srcorig) || (cursor >= src)) {
            setOffset();
            srcorig = src;
          }
        }

        if (++src == srcmax) break;
        clearOffset();
      }

      if (!done) src = srcorig;
    }

    updateCache();
  }

  *inputLength = src - srcmin;
  *outputLength = dest - destmin;
}
