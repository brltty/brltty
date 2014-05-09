/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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
#include <unicode/unorm.h>
#endif /* HAVE_ICU */

#include "ctb.h"
#include "ctb_internal.h"
#include "ttb.h"
#include "prefs.h"
#include "unicode.h"
#include "charset.h"
#include "ascii.h"
#include "brl_dots.h"
#include "log.h"
#include "file.h"
#include "parse.h"

typedef struct {
  ContractionTable *const table;

  struct {
    const wchar_t *begin;
    const wchar_t *end;
    const wchar_t *current;
    const wchar_t *cursor;
    int *offsets;
  } input;

  struct {
    BYTE *begin;
    BYTE *end;
    BYTE *current;
  } output;

  struct {
    const ContractionTableRule *rule;
    ContractionTableOpcode opcode;
    int length;

    wchar_t before;
    wchar_t after;
  } current;

  struct {
    ContractionTableOpcode opcode;
  } previous;
} BrailleContractionData;

static inline void
assignOffset (BrailleContractionData *bcd, size_t value) {
  if (bcd->input.offsets) bcd->input.offsets[bcd->input.current - bcd->input.begin] = value;
}

static inline void
setOffset (BrailleContractionData *bcd) {
  assignOffset(bcd, bcd->output.current - bcd->output.begin);
}

static inline void
clearOffset (BrailleContractionData *bcd) {
  assignOffset(bcd, CTB_NO_OFFSET);
}

static inline ContractionTableHeader *
getContractionTableHeader (BrailleContractionData *bcd) {
  return bcd->table->data.internal.header.fields;
}

static inline const void *
getContractionTableItem (BrailleContractionData *bcd, ContractionTableOffset offset) {
  return &bcd->table->data.internal.header.bytes[offset];
}

static const ContractionTableCharacter *
getContractionTableCharacter (BrailleContractionData *bcd, wchar_t character) {
  const ContractionTableCharacter *characters = getContractionTableItem(bcd, getContractionTableHeader(bcd)->characters);
  int first = 0;
  int last = getContractionTableHeader(bcd)->characterCount - 1;

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

typedef struct {
  BrailleContractionData *bcd;
  CharacterEntry *character;
} SetAlwaysRuleData;

static int
setAlwaysRule (wchar_t character, void *data) {
  SetAlwaysRuleData *sar = data;
  const ContractionTableCharacter *ctc = getContractionTableCharacter(sar->bcd, character);

  if (ctc) {
    ContractionTableOffset offset = ctc->always;

    if (offset) {
      const ContractionTableRule *rule = getContractionTableItem(sar->bcd, offset);

      if (rule->replen) {
        sar->character->always = rule;
        return 1;
      }
    }
  }

  return 0;
}

static CharacterEntry *
getCharacterEntry (BrailleContractionData *bcd, wchar_t character) {
  int first = 0;
  int last = bcd->table->characters.count - 1;

  while (first <= last) {
    int current = (first + last) / 2;
    CharacterEntry *entry = &bcd->table->characters.array[current];

    if (entry->value < character) {
      first = current + 1;
    } else if (entry->value > character) {
      last = current - 1;
    } else {
      return entry;
    }
  }

  if (bcd->table->characters.count == bcd->table->characters.size) {
    int newSize = bcd->table->characters.size;
    newSize = newSize? newSize<<1: 0X80;

    {
      CharacterEntry *newArray = realloc(bcd->table->characters.array, (newSize * sizeof(*newArray)));

      if (!newArray) {
        logMallocError();
        return NULL;
      }

      bcd->table->characters.array = newArray;
      bcd->table->characters.size = newSize;
    }
  }

  memmove(&bcd->table->characters.array[first+1],
          &bcd->table->characters.array[first],
          (bcd->table->characters.count - first) * sizeof(*bcd->table->characters.array));
  bcd->table->characters.count += 1;

  {
    CharacterEntry *entry = &bcd->table->characters.array[first];
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

    if (!bcd->table->command) {
      const ContractionTableCharacter *ctc = getContractionTableCharacter(bcd, character);

      if (ctc) entry->attributes |= ctc->attributes;
    }

    {
      SetAlwaysRuleData sar = {
        .bcd = bcd,
        .character = entry
      };

      if (!handleBestCharacter(character, setAlwaysRule, &sar)) {
        entry->always = NULL;
      }
    }

    return entry;
  }
}

static const ContractionTableRule *
getAlwaysRule (BrailleContractionData *bcd, wchar_t character) {
  const CharacterEntry *entry = getCharacterEntry(bcd, character);

  return entry? entry->always: NULL;
}

static wchar_t
getBestCharacter (BrailleContractionData *bcd, wchar_t character) {
  const ContractionTableRule *rule = getAlwaysRule(bcd, character);

  return rule? rule->findrep[0]: 0;
}

static int
sameCharacters (BrailleContractionData *bcd, wchar_t character1, wchar_t character2) {
  wchar_t best1 = getBestCharacter(bcd, character1);

  return best1 && (best1 == getBestCharacter(bcd, character2));
}

static int
testCharacter (BrailleContractionData *bcd, wchar_t character, ContractionTableCharacterAttributes attributes) {
  const CharacterEntry *entry = getCharacterEntry(bcd, character);
  return entry && (attributes & entry->attributes);
}

static wchar_t
toLowerCase (BrailleContractionData *bcd, wchar_t character) {
  const CharacterEntry *entry = getCharacterEntry(bcd, character);
  return entry? entry->lowercase: character;
}

static int
checkCurrentRule (BrailleContractionData *bcd, const wchar_t *source) {
  const wchar_t *character = bcd->current.rule->findrep;
  int count = bcd->current.length;

  while (count) {
    if (toLowerCase(bcd, *source) != toLowerCase(bcd, *character)) return 0;
    --count, ++source, ++character;
  }
  return 1;
}

static void
setBefore (BrailleContractionData *bcd) {
  bcd->current.before = (bcd->input.current == bcd->input.begin)? WC_C(' '): bcd->input.current[-1];
}

static void
setAfter (BrailleContractionData *bcd, int length) {
  bcd->current.after = (bcd->input.current + length < bcd->input.end)? bcd->input.current[length]: WC_C(' ');
}

static int
isBeginning (BrailleContractionData *bcd) {
  const wchar_t *ptr = bcd->input.current;

  while (ptr > bcd->input.begin) {
    if (!testCharacter(bcd, *--ptr, CTC_Punctuation)) {
      if (!testCharacter(bcd, *ptr, CTC_Space)) return 0;
      break;
    }
  }

  return 1;
}

static int
isEnding (BrailleContractionData *bcd) {
  const wchar_t *ptr = bcd->input.current + bcd->current.length;

  while (ptr < bcd->input.end) {
    if (!testCharacter(bcd, *ptr, CTC_Punctuation)) {
      if (!testCharacter(bcd, *ptr, CTC_Space)) return 0;
      break;
    }

    ptr += 1;
  }

  return 1;
}

static int
selectRule (BrailleContractionData *bcd, int length) {
  int ruleOffset;
  int maximumLength;

  if (length < 1) return 0;
  if (length == 1) {
    const ContractionTableCharacter *ctc = getContractionTableCharacter(bcd, toLowerCase(bcd, *bcd->input.current));
    if (!ctc) return 0;
    ruleOffset = ctc->rules;
    maximumLength = 1;
  } else {
    wchar_t characters[2];
    characters[0] = toLowerCase(bcd, bcd->input.current[0]);
    characters[1] = toLowerCase(bcd, bcd->input.current[1]);
    ruleOffset = getContractionTableHeader(bcd)->rules[CTH(characters)];
    maximumLength = 0;
  }

  while (ruleOffset) {
    bcd->current.rule = getContractionTableItem(bcd, ruleOffset);
    bcd->current.opcode = bcd->current.rule->opcode;
    bcd->current.length = bcd->current.rule->findlen;

    if ((length == 1) ||
        ((bcd->current.length <= length) &&
         checkCurrentRule(bcd, bcd->input.current))) {
      setAfter(bcd, bcd->current.length);

      if (!maximumLength) {
        maximumLength = bcd->current.length;

        if (prefs.capitalizationMode != CTB_CAP_NONE) {
          typedef enum {CS_Any, CS_Lower, CS_UpperSingle, CS_UpperMultiple} CapitalizationState;
#define STATE(c) (testCharacter(bcd, (c), CTC_UpperCase)? CS_UpperSingle: testCharacter(bcd, (c), CTC_LowerCase)? CS_Lower: CS_Any)

          CapitalizationState current = STATE(bcd->current.before);
          int i;

          for (i=0; i<bcd->current.length; i+=1) {
            wchar_t character = bcd->input.current[i];
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

      if ((bcd->current.length <= maximumLength) &&
          (!bcd->current.rule->after || testCharacter(bcd, bcd->current.before, bcd->current.rule->after)) &&
          (!bcd->current.rule->before || testCharacter(bcd, bcd->current.after, bcd->current.rule->before))) {
        switch (bcd->current.opcode) {
          case CTO_Always:
          case CTO_Repeatable:
          case CTO_Literal:
            return 1;

          case CTO_LargeSign:
          case CTO_LastLargeSign:
            if (!isBeginning(bcd) || !isEnding(bcd)) bcd->current.opcode = CTO_Always;
            return 1;

          case CTO_WholeWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Space|CTC_Punctuation) &&
                testCharacter(bcd, bcd->current.after, CTC_Space|CTC_Punctuation))
              return 1;
            break;

          case CTO_Contraction:
            if ((bcd->input.current > bcd->input.begin) && sameCharacters(bcd, bcd->input.current[-1], WC_C('\''))) break;
            if (isBeginning(bcd) && isEnding(bcd)) return 1;
            break;

          case CTO_LowWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Space) && testCharacter(bcd, bcd->current.after, CTC_Space) &&
                (bcd->previous.opcode != CTO_JoinedWord) &&
                ((bcd->output.current == bcd->output.begin) || !bcd->output.current[-1]))
              return 1;
            break;

          case CTO_JoinedWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Space|CTC_Punctuation) &&
                !sameCharacters(bcd, bcd->current.before, WC_C('-')) &&
                (bcd->output.current + bcd->current.rule->replen < bcd->output.end)) {
              const wchar_t *end = bcd->input.current + bcd->current.length;
              const wchar_t *ptr = end;

              while (ptr < bcd->input.end) {
                if (!testCharacter(bcd, *ptr, CTC_Space)) {
                  if (!testCharacter(bcd, *ptr, CTC_Letter)) break;
                  if (ptr == end) break;
                  return 1;
                }

                if (ptr++ == bcd->input.cursor) break;
              }
            }
            break;

          case CTO_SuffixableWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Space|CTC_Punctuation) &&
                testCharacter(bcd, bcd->current.after, CTC_Space|CTC_Letter|CTC_Punctuation))
              return 1;
            break;

          case CTO_PrefixableWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Space|CTC_Letter|CTC_Punctuation) &&
                testCharacter(bcd, bcd->current.after, CTC_Space|CTC_Punctuation))
              return 1;
            break;

          case CTO_BegWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Space|CTC_Punctuation) &&
                testCharacter(bcd, bcd->current.after, CTC_Letter))
              return 1;
            break;

          case CTO_BegMidWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Letter|CTC_Space|CTC_Punctuation) &&
                testCharacter(bcd, bcd->current.after, CTC_Letter))
              return 1;
            break;

          case CTO_MidWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Letter) && testCharacter(bcd, bcd->current.after, CTC_Letter))
              return 1;
            break;

          case CTO_MidEndWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Letter) &&
                testCharacter(bcd, bcd->current.after, CTC_Letter|CTC_Space|CTC_Punctuation))
              return 1;
            break;

          case CTO_EndWord:
            if (testCharacter(bcd, bcd->current.before, CTC_Letter) &&
                testCharacter(bcd, bcd->current.after, CTC_Space|CTC_Punctuation))
              return 1;
            break;

          case CTO_BegNum:
            if (testCharacter(bcd, bcd->current.before, CTC_Space|CTC_Punctuation) &&
                testCharacter(bcd, bcd->current.after, CTC_Digit))
              return 1;
            break;

          case CTO_MidNum:
            if (testCharacter(bcd, bcd->current.before, CTC_Digit) && testCharacter(bcd, bcd->current.after, CTC_Digit))
              return 1;
            break;

          case CTO_EndNum:
            if (testCharacter(bcd, bcd->current.before, CTC_Digit) &&
                testCharacter(bcd, bcd->current.after, CTC_Space|CTC_Punctuation))
              return 1;
            break;

          case CTO_PrePunc:
            if (testCharacter(bcd, *bcd->input.current, CTC_Punctuation) && isBeginning(bcd) && !isEnding(bcd)) return 1;
            break;

          case CTO_PostPunc:
            if (testCharacter(bcd, *bcd->input.current, CTC_Punctuation) && !isBeginning(bcd) && isEnding(bcd)) return 1;
            break;

          default:
            break;
        }
      }
    }

    ruleOffset = bcd->current.rule->next;
  }

  return 0;
}

static int
putCells (BrailleContractionData *bcd, const BYTE *cells, int count) {
  if (bcd->output.current + count > bcd->output.end) return 0;
  bcd->output.current = mempcpy(bcd->output.current, cells, count);
  return 1;
}

static int
putCell (BrailleContractionData *bcd, BYTE byte) {
  return putCells(bcd, &byte, 1);
}

static int
putReplace (BrailleContractionData *bcd, const ContractionTableRule *rule, wchar_t character) {
  const BYTE *cells = (BYTE *)&rule->findrep[rule->findlen];
  int count = rule->replen;

  if ((prefs.capitalizationMode == CTB_CAP_DOT7) &&
      testCharacter(bcd, character, CTC_UpperCase)) {
    if (!putCell(bcd, *cells++ | BRL_DOT_7)) return 0;
    if (!(count -= 1)) return 1;
  }

  return putCells(bcd, cells, count);
}

static int
putCharacter (BrailleContractionData *bcd, wchar_t character) {
  {
    const ContractionTableRule *rule = getAlwaysRule(bcd, character);

    if (rule) return putReplace(bcd, rule, character);
  }

  {
#ifdef HAVE_WCHAR_H 
    const wchar_t replacementCharacter = UNICODE_REPLACEMENT_CHARACTER;
    if (getCharacterWidth(character) == 0) return 1;
#else /* HAVE_WCHAR_H */
    const wchar_t replacementCharacter = SUB;
#endif /* HAVE_WCHAR_H */

    if (replacementCharacter != character) {
      const ContractionTableRule *rule = getAlwaysRule(bcd, replacementCharacter);
      if (rule) return putReplace(bcd, rule, character);
    }
  }

  return putCell(bcd, BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6 | BRL_DOT_7 | BRL_DOT_8);
}

static int
putSequence (BrailleContractionData *bcd, ContractionTableOffset offset) {
  const BYTE *sequence = getContractionTableItem(bcd, offset);
  return putCells(bcd, sequence+1, *sequence);
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

static int
nextBaseCharacter (const UChar **current, const UChar *end) {
  do {
    if (*current == end) return 0;
  } while (u_getCombiningClass(*(*current)++));

  return 1;
}

static int
normalizeText (
  BrailleContractionData *bcd,
  const wchar_t *begin, const wchar_t *end,
  wchar_t *buffer, size_t *length,
  unsigned int *map
) {
  const size_t size = end - begin;
  UChar source[size];
  UChar target[size];
  int32_t count;

  {
    const wchar_t *wc = begin;
    UChar *uc = source;

    while (wc < end) {
      *uc++ = *wc++;
    }
  }

  {
    UErrorCode error = U_ZERO_ERROR;

    count = unorm_normalize(source, ARRAY_COUNT(source),
                            UNORM_NFC, 0,
                            target, ARRAY_COUNT(target),
                            &error);

    if (!U_SUCCESS(error)) return 0;
  }

  if (count == size) {
    if (memcmp(source, target, (count * sizeof(source[0]))) == 0) {
      return 0;
    }
  }

  {
    const UChar *src = source;
    const UChar *srcEnd = src + ARRAY_COUNT(source);
    const UChar *trg = target;
    const UChar *trgEnd = target + count;
    wchar_t *out = buffer;

    while (trg < trgEnd) {
      if (!nextBaseCharacter(&src, srcEnd)) return 0;
      *map++ = src - source - 1;
      *out++ = *trg++;
    }

    if (nextBaseCharacter(&src, srcEnd)) return 0;
    *map = src - source;
  }

  *length = count;
  return 1;
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
    int isSpace = testCharacter(bcd, characters[lbo->index], CTC_Space);
    opportunities[lbo->index] = lbo->wasSpace && !isSpace;

    lbo->wasSpace = isSpace;
    lbo->index += 1;
  }
}

static int
normalizeText (
  BrailleContractionData *bcd,
  const wchar_t *begin, const wchar_t *end,
  wchar_t *buffer, size_t *length
) {
  return 0;
}
#endif /* HAVE_ICU */

static int
contractTextInternally (BrailleContractionData *bcd) {
  const wchar_t *srcword = NULL;
  BYTE *destword = NULL;

  const wchar_t *srcjoin = NULL;
  BYTE *destjoin = NULL;

  BYTE *destlast = NULL;
  const wchar_t *literal = NULL;

  unsigned char lineBreakOpportunities[bcd->input.end - bcd->input.begin];
  LineBreakOpportunitiesState lbo;

  prepareLineBreakOpportunitiesState(&lbo);
  bcd->previous.opcode = CTO_None;

  while (bcd->input.current < bcd->input.end) {
    int wasLiteral = bcd->input.current == literal;

    destlast = bcd->output.current;
    setOffset(bcd);
    setBefore(bcd);

    if (literal)
      if (bcd->input.current >= literal)
        if (testCharacter(bcd, *bcd->input.current, CTC_Space) || testCharacter(bcd, bcd->input.current[-1], CTC_Space))
          literal = NULL;

    if ((!literal && selectRule(bcd, bcd->input.end-bcd->input.current)) || selectRule(bcd, 1)) {
      if (!literal &&
          ((bcd->current.opcode == CTO_Literal) ||
           (prefs.expandCurrentWord &&
            (bcd->input.cursor >= bcd->input.current) &&
            (bcd->input.cursor < (bcd->input.current + bcd->current.length))))) {
        literal = bcd->input.current + bcd->current.length;

        if (!testCharacter(bcd, *bcd->input.current, CTC_Space)) {
          if (destjoin) {
            bcd->input.current = srcjoin;
            bcd->output.current = destjoin;
          } else {
            bcd->input.current = bcd->input.begin;
            bcd->output.current = bcd->output.begin;
          }
        }

        continue;
      }

      if (getContractionTableHeader(bcd)->numberSign && (bcd->previous.opcode != CTO_MidNum) &&
          !testCharacter(bcd, bcd->current.before, CTC_Digit) && testCharacter(bcd, *bcd->input.current, CTC_Digit)) {
        if (!putSequence(bcd, getContractionTableHeader(bcd)->numberSign)) break;
      } else if (getContractionTableHeader(bcd)->englishLetterSign && testCharacter(bcd, *bcd->input.current, CTC_Letter)) {
        if ((bcd->current.opcode == CTO_Contraction) ||
            ((bcd->current.opcode != CTO_EndNum) && testCharacter(bcd, bcd->current.before, CTC_Digit)) ||
            (testCharacter(bcd, *bcd->input.current, CTC_Letter) &&
             (bcd->current.opcode == CTO_Always) &&
             (bcd->current.length == 1) &&
             testCharacter(bcd, bcd->current.before, CTC_Space) &&
             (((bcd->input.current + 1) == bcd->input.end) ||
              testCharacter(bcd, bcd->input.current[1], CTC_Space) ||
              (testCharacter(bcd, bcd->input.current[1], CTC_Punctuation) &&
               !sameCharacters(bcd, bcd->input.current[1], WC_C('.')) &&
               !sameCharacters(bcd, bcd->input.current[1], WC_C('\'')))))) {
          if (!putSequence(bcd, getContractionTableHeader(bcd)->englishLetterSign)) break;
        }
      }

      if (prefs.capitalizationMode == CTB_CAP_SIGN) {
        if (testCharacter(bcd, *bcd->input.current, CTC_UpperCase)) {
          if (!testCharacter(bcd, bcd->current.before, CTC_UpperCase)) {
            if (getContractionTableHeader(bcd)->beginCapitalSign &&
                (bcd->input.current + 1 < bcd->input.end) && testCharacter(bcd, bcd->input.current[1], CTC_UpperCase)) {
              if (!putSequence(bcd, getContractionTableHeader(bcd)->beginCapitalSign)) break;
            } else if (getContractionTableHeader(bcd)->capitalSign) {
              if (!putSequence(bcd, getContractionTableHeader(bcd)->capitalSign)) break;
            }
          }
        } else if (testCharacter(bcd, *bcd->input.current, CTC_LowerCase)) {
          if (getContractionTableHeader(bcd)->endCapitalSign && (bcd->input.current - 2 >= bcd->input.begin) &&
              testCharacter(bcd, bcd->input.current[-1], CTC_UpperCase) && testCharacter(bcd, bcd->input.current[-2], CTC_UpperCase)) {
            if (!putSequence(bcd, getContractionTableHeader(bcd)->endCapitalSign)) break;
          }
        }
      }

      switch (bcd->current.opcode) {
        case CTO_LargeSign:
        case CTO_LastLargeSign:
          if ((bcd->previous.opcode == CTO_LargeSign) && !wasLiteral) {
            while ((bcd->output.current > bcd->output.begin) && !bcd->output.current[-1]) bcd->output.current -= 1;
            setOffset(bcd);

            {
              BYTE **destptrs[] = {&destword, &destjoin, &destlast, NULL};
              BYTE ***destptr = destptrs;

              while (*destptr) {
                if (**destptr && (**destptr > bcd->output.current)) **destptr = bcd->output.current;
                destptr += 1;
              }
            }
          }
          break;

        default:
          break;
      }

      if (bcd->current.rule->replen &&
          !((bcd->current.opcode == CTO_Always) && (bcd->current.length == 1))) {
        const wchar_t *srcnxt = bcd->input.current + bcd->current.length;
        if (!putReplace(bcd, bcd->current.rule, *bcd->input.current)) goto done;
        while (++bcd->input.current != srcnxt) clearOffset(bcd);
      } else {
        const wchar_t *srclim = bcd->input.current + bcd->current.length;
        while (1) {
          if (!putCharacter(bcd, *bcd->input.current)) goto done;
          if (++bcd->input.current == srclim) break;
          setOffset(bcd);
        }
      }

      {
        const wchar_t *srcorig = bcd->input.current;
        const wchar_t *srcbeg = NULL;
        BYTE *destbeg = NULL;

        switch (bcd->current.opcode) {
          case CTO_Repeatable: {
            const wchar_t *srclim = bcd->input.end - bcd->current.length;

            srcbeg = bcd->input.current - bcd->current.length;
            destbeg = destlast;

            while ((bcd->input.current <= srclim) && checkCurrentRule(bcd, bcd->input.current)) {
              const wchar_t *srcnxt = bcd->input.current + bcd->current.length;

              do {
                clearOffset(bcd);
              } while (++bcd->input.current != srcnxt);
            }

            break;
          }

          case CTO_JoinedWord:
            srcbeg = bcd->input.current;
            destbeg = bcd->output.current;

            while ((bcd->input.current < bcd->input.end) && testCharacter(bcd, *bcd->input.current, CTC_Space)) {
              clearOffset(bcd);
              bcd->input.current += 1;
            }
            break;

          default:
            break;
        }

        if (srcbeg && (bcd->input.cursor >= srcbeg) && (bcd->input.cursor < bcd->input.current)) {
          int repeat = !literal;
          literal = bcd->input.current;

          if (repeat) {
            bcd->input.current = srcbeg;
            bcd->output.current = destbeg;
            continue;
          }

          bcd->input.current = srcorig;
        }
      }
    } else {
      bcd->current.opcode = CTO_Always;
      if (!putCharacter(bcd, *bcd->input.current)) break;
      bcd->input.current += 1;
    }

    findLineBreakOpportunities(&lbo, lineBreakOpportunities, bcd->input.begin, bcd->input.current-bcd->input.begin);
    if (lineBreakOpportunities[bcd->input.current-bcd->input.begin]) {
      srcjoin = bcd->input.current;
      destjoin = bcd->output.current;

      if (bcd->current.opcode != CTO_JoinedWord) {
        srcword = bcd->input.current;
        destword = bcd->output.current;
      }
    }

    if ((bcd->output.current == bcd->output.begin) || bcd->output.current[-1]) {
      bcd->previous.opcode = bcd->current.opcode;
    }
  }

done:
  if (bcd->input.current < bcd->input.end) {
    if (destword && (destword > bcd->output.begin) &&
        (!(testCharacter(bcd, bcd->input.current[-1], CTC_Space) || testCharacter(bcd, *bcd->input.current, CTC_Space)) ||
         (bcd->previous.opcode == CTO_JoinedWord))) {
      bcd->input.current = srcword;
      bcd->output.current = destword;
    } else if (destlast) {
      bcd->output.current = destlast;
    }
  }

  return 1;
}

static int
putExternalRequests (BrailleContractionData *bcd) {
  typedef enum {
    REQ_TEXT,
    REQ_NUMBER
  } ExternalRequestType;

  typedef struct {
    const char *name;
    ExternalRequestType type;

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
      .value.number = bcd->input.cursor? bcd->input.cursor-bcd->input.begin+1: 0
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
      .value.number = bcd->output.end - bcd->output.begin
    },

    { .name = "text",
      .type = REQ_TEXT,
      .value.text = {
        .start = bcd->input.begin,
        .count = bcd->input.end - bcd->input.begin
      }
    },

    { .name = NULL }
  };

  FILE *stream = bcd->table->data.external.standardInput;
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
        logMessage(LOG_WARNING, "unimplemented external contraction request property type: %s: %u (%s)", bcd->table->command, req->type, req->name);
        return 0;
    }

    if (fputc('\n', stream) == EOF) goto outputError;
    req += 1;
  }

  if (fflush(stream) == EOF) goto outputError;
  return 1;

outputError:
  logMessage(LOG_WARNING, "external contraction output error: %s: %s", bcd->table->command, strerror(errno));
  return 0;
}

static const unsigned char brfTable[0X40] = {
  /* 0X20   */ 0,
  /* 0X21 ! */ BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_6,
  /* 0X22 " */ BRL_DOT_5,
  /* 0X23 # */ BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X24 $ */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_6,
  /* 0X25 % */ BRL_DOT_1 | BRL_DOT_4 | BRL_DOT_6,
  /* 0X26 & */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_6,
  /* 0X27 ' */ BRL_DOT_3,
  /* 0X28 ( */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X29 ) */ BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X2A * */ BRL_DOT_1 | BRL_DOT_6,
  /* 0X2B + */ BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_6,
  /* 0X2C , */ BRL_DOT_6,
  /* 0X2D - */ BRL_DOT_3 | BRL_DOT_6,
  /* 0X2E . */ BRL_DOT_4 | BRL_DOT_6,
  /* 0X2F / */ BRL_DOT_3 | BRL_DOT_4,
  /* 0X30 0 */ BRL_DOT_3 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X31 1 */ BRL_DOT_2,
  /* 0X32 2 */ BRL_DOT_2 | BRL_DOT_3,
  /* 0X33 3 */ BRL_DOT_2 | BRL_DOT_5,
  /* 0X34 4 */ BRL_DOT_2 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X35 5 */ BRL_DOT_2 | BRL_DOT_6,
  /* 0X36 6 */ BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_5,
  /* 0X37 7 */ BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X38 8 */ BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_6,
  /* 0X39 9 */ BRL_DOT_3 | BRL_DOT_5,
  /* 0X3A : */ BRL_DOT_1 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X3B ; */ BRL_DOT_5 | BRL_DOT_6,
  /* 0X3C < */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_6,
  /* 0X3D = */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X3E > */ BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5,
  /* 0X3F ? */ BRL_DOT_1 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X40 @ */ BRL_DOT_4,
  /* 0X41 A */ BRL_DOT_1,
  /* 0X42 B */ BRL_DOT_1 | BRL_DOT_2,
  /* 0X43 C */ BRL_DOT_1 | BRL_DOT_4,
  /* 0X44 D */ BRL_DOT_1 | BRL_DOT_4 | BRL_DOT_5,
  /* 0X45 E */ BRL_DOT_1 | BRL_DOT_5,
  /* 0X46 F */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4,
  /* 0X47 G */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5,
  /* 0X48 H */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_5,
  /* 0X49 I */ BRL_DOT_2 | BRL_DOT_4,
  /* 0X4A J */ BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5,
  /* 0X4B K */  BRL_DOT_1 | BRL_DOT_3,
  /* 0X4C L */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3,
  /* 0X4D M */ BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_4,
  /* 0X4E N */ BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5,
  /* 0X4F O */ BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_5,
  /* 0X50 P */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4,
  /* 0X51 Q */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5,
  /* 0X52 R */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_5,
  /* 0X53 S */ BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4,
  /* 0X54 T */ BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5,
  /* 0X55 U */ BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_6,
  /* 0X56 V */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_6,
  /* 0X57 W */ BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X58 X */ BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_6,
  /* 0X59 Y */ BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X5A Z */ BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X5B [ */ BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_6,
  /* 0X5C \ */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X5D ] */ BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6,
  /* 0X5E ^ */ BRL_DOT_4 | BRL_DOT_5,
  /* 0X5F _ */ BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6
};

static int
handleExternalResponse_brf (BrailleContractionData *bcd, const char *value) {
  int useDot7 = prefs.capitalizationMode == CTB_CAP_DOT7;

  while (*value && (bcd->output.current < bcd->output.end)) {
    unsigned char brf = *value++ & 0XFF;
    unsigned char dots = 0;
    unsigned char superimpose = 0;

    if ((brf >= 0X60) && (brf <= 0X7F)) {
      brf -= 0X20;
    } else if ((brf >= 0X41) && (brf <= 0X5A)) {
      if (useDot7) superimpose |= BRL_DOT_7;
    }

    if ((brf >= 0X20) && (brf <= 0X5F)) dots = brfTable[brf - 0X20] | superimpose;
    *bcd->output.current++ = dots;
  }

  return 1;
}

static int
handleExternalResponse_consumedLength (BrailleContractionData *bcd, const char *value) {
  int length;

  if (!isInteger(&length, value)) return 0;
  if (length < 1) return 0;
  if (length > (bcd->input.end - bcd->input.begin)) return 0;

  bcd->input.current = bcd->input.begin + length;
  return 1;
}

static int
handleExternalResponse_outputOffsets (BrailleContractionData *bcd, const char *value) {
  if (bcd->input.offsets) {
    int previous = CTB_NO_OFFSET;
    unsigned int count = bcd->input.end - bcd->input.begin;
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
      if (offset >= (bcd->output.end - bcd->output.begin)) return 0;

      bcd->input.offsets[index++] = (offset == previous)? CTB_NO_OFFSET: offset;
      previous = offset;
    }
  }

  return 1;
}

typedef struct {
  const char *name;
  int (*handler) (BrailleContractionData *bcd, const char *value);
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
getExternalResponses (BrailleContractionData *bcd) {
  FILE *stream = bcd->table->data.external.standardOutput;

  while (readLine(stream, &bcd->table->data.external.input.buffer, &bcd->table->data.external.input.size)) {
    int ok = 0;
    int stop = 0;
    char *delimiter = strchr(bcd->table->data.external.input.buffer, '=');

    if (delimiter) {
      const char *value = delimiter + 1;
      const ExternalResponseEntry *rsp = externalResponseTable;

      char oldDelimiter = *delimiter;
      *delimiter = 0;

      while (rsp->name) {
        if (strcmp(bcd->table->data.external.input.buffer, rsp->name) == 0) {
          if (rsp->handler(bcd, value)) ok = 1;
          if (rsp->stop) stop = 1;
          break;
        }

        rsp += 1;
      }

      *delimiter = oldDelimiter;
    }

    if (!ok) logMessage(LOG_WARNING, "unexpected external contraction response: %s: %s", bcd->table->command, bcd->table->data.external.input.buffer);
    if (stop) return 1;
  }

  logMessage(LOG_WARNING, "incomplete external contraction response: %s", bcd->table->command);
  return 0;
}

static int
contractTextExternally (BrailleContractionData *bcd) {
  setOffset(bcd);
  while (++bcd->input.current < bcd->input.end) clearOffset(bcd);

  if (startContractionCommand(bcd->table)) {
    if (putExternalRequests(bcd)) {
      if (getExternalResponses(bcd)) {
        return 1;
      }
    }
  }

  stopContractionCommand(bcd->table);
  return 0;
}

static inline unsigned int
makeCachedInputCount (BrailleContractionData *bcd) {
  return bcd->input.end - bcd->input.begin;
}

static inline unsigned int
makeCachedOutputMaximum (BrailleContractionData *bcd) {
  return bcd->output.end - bcd->output.begin;
}

static inline int
makeCachedCursorOffset (BrailleContractionData *bcd) {
  return bcd->input.cursor? (bcd->input.cursor - bcd->input.begin): CTB_NO_CURSOR;
}

static int
checkCache (BrailleContractionData *bcd) {
  if (!bcd->table->cache.input.characters) return 0;
  if (!bcd->table->cache.output.cells) return 0;
  if (bcd->input.offsets && !bcd->table->cache.offsets.count) return 0;
  if (bcd->table->cache.output.maximum != makeCachedOutputMaximum(bcd)) return 0;
  if (bcd->table->cache.cursorOffset != makeCachedCursorOffset(bcd)) return 0;
  if (bcd->table->cache.expandCurrentWord != prefs.expandCurrentWord) return 0;
  if (bcd->table->cache.capitalizationMode != prefs.capitalizationMode) return 0;

  {
    unsigned int count = makeCachedInputCount(bcd);
    if (bcd->table->cache.input.count != count) return 0;
    if (wmemcmp(bcd->input.begin, bcd->table->cache.input.characters, count) != 0) return 0;
  }

  return 1;
}

static void
updateCache (BrailleContractionData *bcd) {
  {
    unsigned int count = makeCachedInputCount(bcd);

    if (count > bcd->table->cache.input.size) {
      unsigned int newSize = count | 0X7F;
      wchar_t *newCharacters = malloc(ARRAY_SIZE(newCharacters, newSize));

      if (!newCharacters) {
        logMallocError();
        bcd->table->cache.input.count = 0;
        goto inputDone;
      }

      if (bcd->table->cache.input.characters) free(bcd->table->cache.input.characters);
      bcd->table->cache.input.characters = newCharacters;
      bcd->table->cache.input.size = newSize;
    }

    wmemcpy(bcd->table->cache.input.characters, bcd->input.begin, count);
    bcd->table->cache.input.count = count;
    bcd->table->cache.input.consumed = bcd->input.current - bcd->input.begin;
  }
inputDone:

  {
    unsigned int count = bcd->output.current - bcd->output.begin;

    if (count > bcd->table->cache.output.size) {
      unsigned int newSize = count | 0X7F;
      unsigned char *newCells = malloc(ARRAY_SIZE(newCells, newSize));

      if (!newCells) {
        logMallocError();
        bcd->table->cache.output.count = 0;
        goto outputDone;
      }

      if (bcd->table->cache.output.cells) free(bcd->table->cache.output.cells);
      bcd->table->cache.output.cells = newCells;
      bcd->table->cache.output.size = newSize;
    }

    memcpy(bcd->table->cache.output.cells, bcd->output.begin, count);
    bcd->table->cache.output.count = count;
    bcd->table->cache.output.maximum = makeCachedOutputMaximum(bcd);
  }
outputDone:

  if (bcd->input.offsets) {
    unsigned int count = makeCachedInputCount(bcd);

    if (count > bcd->table->cache.offsets.size) {
      unsigned int newSize = count | 0X7F;
      int *newArray = malloc(ARRAY_SIZE(newArray, newSize));

      if (!newArray) {
        logMallocError();
        bcd->table->cache.offsets.count = 0;
        goto offsetsDone;
      }

      if (bcd->table->cache.offsets.array) free(bcd->table->cache.offsets.array);
      bcd->table->cache.offsets.array = newArray;
      bcd->table->cache.offsets.size = newSize;
    }

    memcpy(bcd->table->cache.offsets.array, bcd->input.offsets, ARRAY_SIZE(bcd->input.offsets, count));
    bcd->table->cache.offsets.count = count;
  } else {
    bcd->table->cache.offsets.count = 0;
  }
offsetsDone:

  bcd->table->cache.cursorOffset = makeCachedCursorOffset(bcd);
  bcd->table->cache.expandCurrentWord = prefs.expandCurrentWord;
  bcd->table->cache.capitalizationMode = prefs.capitalizationMode;
}

void
contractText (
  ContractionTable *contractionTable,
  const wchar_t *inputBuffer, int *inputLength,
  BYTE *outputBuffer, int *outputLength,
  int *offsetsMap, const int cursorOffset
) {
  BrailleContractionData bcd = {
    .table = contractionTable,

    .input = {
      .begin = inputBuffer,
      .current = inputBuffer,
      .end = inputBuffer + *inputLength,
      .cursor = (cursorOffset == CTB_NO_CURSOR)? NULL: &inputBuffer[cursorOffset],
      .offsets = offsetsMap
    },

    .output = {
      .begin = outputBuffer,
      .end = outputBuffer + *outputLength,
      .current = outputBuffer
    }
  };

  if (checkCache(&bcd)) {
    bcd.input.current = bcd.input.begin + bcd.table->cache.input.consumed;

    if (bcd.input.offsets) {
      memcpy(bcd.input.offsets, bcd.table->cache.offsets.array,
             ARRAY_SIZE(bcd.input.offsets, bcd.table->cache.offsets.count));
    }

    bcd.output.current = bcd.output.begin + bcd.table->cache.output.count;
    memcpy(bcd.output.begin, bcd.table->cache.output.cells,
           ARRAY_SIZE(bcd.output.begin, bcd.table->cache.output.count));
  } else {
    int contracted;

    {
      int (*const contract) (BrailleContractionData *bcd) = bcd.table->command? contractTextExternally: contractTextInternally;
      const size_t size = bcd.input.end - bcd.input.begin;
      wchar_t buffer[size];
      unsigned int map[size + 1];
      size_t length;

      if (normalizeText(&bcd, bcd.input.begin, bcd.input.end, buffer, &length, map)) {
        const wchar_t *origmin = bcd.input.begin;
        const wchar_t *origmax = bcd.input.end;

        bcd.input.begin = buffer;
        bcd.input.current = bcd.input.begin + (bcd.input.current - origmin);
        bcd.input.end = bcd.input.begin + length;

        if (bcd.input.cursor) {
          ptrdiff_t offset = bcd.input.cursor - origmin;
          unsigned int mapIndex;

          bcd.input.cursor = NULL;

          for (mapIndex=0; mapIndex<=length; mapIndex+=1) {
            unsigned int mappedIndex = map[mapIndex];

            if (mappedIndex > offset) break;
            bcd.input.cursor = &bcd.input.begin[mappedIndex];
          }
        }

        contracted = contract(&bcd);

        if (bcd.input.offsets) {
          size_t mapIndex = length;
          size_t offsetsIndex = origmax - origmin;

          while (mapIndex > 0) {
            size_t mappedIndex = map[--mapIndex];
            int offset = bcd.input.offsets[mapIndex];

            if (offset != CTB_NO_OFFSET) {
              while (--offsetsIndex > mappedIndex) bcd.input.offsets[offsetsIndex] = CTB_NO_OFFSET;
              bcd.input.offsets[offsetsIndex] = offset;
            }
          }

          while (offsetsIndex > 0) bcd.input.offsets[--offsetsIndex] = CTB_NO_OFFSET;
        }

        bcd.input.begin = origmin;
        bcd.input.current = bcd.input.begin + map[bcd.input.current - buffer];
        bcd.input.end = origmax;
      } else {
        contracted = contract(&bcd);
      }
    }

    if (!contracted) {
      bcd.input.current = bcd.input.begin;
      bcd.output.current = bcd.output.begin;

      while ((bcd.input.current < bcd.input.end) && (bcd.output.current < bcd.output.end)) {
        setOffset(&bcd);
        *bcd.output.current++ = convertCharacterToDots(textTable, *bcd.input.current++);
      }
    }

    if (bcd.input.current < bcd.input.end) {
      const wchar_t *srcorig = bcd.input.current;
      int done = 1;

      setOffset(&bcd);
      while (1) {
        if (done && !testCharacter(&bcd, *bcd.input.current, CTC_Space)) {
          done = 0;

          if (!bcd.input.cursor || (bcd.input.cursor < srcorig) || (bcd.input.cursor >= bcd.input.current)) {
            setOffset(&bcd);
            srcorig = bcd.input.current;
          }
        }

        if (++bcd.input.current == bcd.input.end) break;
        clearOffset(&bcd);
      }

      if (!done) bcd.input.current = srcorig;
    }

    updateCache(&bcd);
  }

  *inputLength = bcd.input.current - bcd.input.begin;
  *outputLength = bcd.output.current - bcd.output.begin;
}
