#if defined(HAVE_PKG_CURSES)
#define USE_CURSES
#include <curses.h>
#elif defined(HAVE_PKG_NCURSES)
#define USE_CURSES
#include <ncurses.h>
#elif defined(HAVE_PKG_NCURSESW)
#define USE_CURSES
#define USE_FUNC_GET_WCH
#include <ncursesw/ncurses.h>
#else
#warning curses package either unspecified or unsupported
#define printw printf
#define clear() printf("\r\n\v")
#define refresh() fflush(stdout)
#define beep() printf("\a")
#endif /* HAVE_PKG_CURSES */

#ifdef ENABLE_API
#define BRLAPI_NO_DEPRECATED
#include "brlapi.h"
#endif /* ENABLE_API */

#ifdef ENABLE_API
static wchar_t *
makeCharacterDescription (TranslationTable table, unsigned char byte, size_t *length, unsigned char *braille) {
  char buffer[0X100];
  int characterIndex;
  int brailleIndex;
  int descriptionLength;

  wint_t character = convertCharToWchar(byte);
  unsigned char dots;
  wchar_t printableCharacter;
  char printablePrefix;

  if (character == WEOF) character = WC_C('?');
  dots = convertWcharToDots(table, character);

  printableCharacter = character;
  if (iswLatin1(printableCharacter)) {
    printablePrefix = '^';
    if (!(printableCharacter & 0X60)) {
      printableCharacter |= 0X40;
      if (printableCharacter & 0X80) printablePrefix = '~';
    } else if (printableCharacter == 0X7F) {
      printableCharacter ^= 0X40;       
    } else if (printableCharacter != printablePrefix) {
      printablePrefix = ' ';
    }
  } else {
    printablePrefix = ' ';
  }

#define DOT(n) ((dots & BRLAPI_DOT##n)? ((n) + '0'): ' ')
  snprintf(buffer, sizeof(buffer),
           "%02X %3u %c%nx %nx [%c%c%c%c%c%c%c%c]%n",
           byte, byte, printablePrefix, &characterIndex, &brailleIndex,
           DOT(1), DOT(2), DOT(3), DOT(4), DOT(5), DOT(6), DOT(7), DOT(8),
           &descriptionLength);
#undef DOT

#ifdef HAVE_ICU
  {
    char *name = buffer + descriptionLength + 1;
    int size = buffer + sizeof(buffer) - name;
    UErrorCode error = U_ZERO_ERROR;

    u_charName(character, U_EXTENDED_CHAR_NAME, name, size, &error);
    if (U_SUCCESS(error)) {
      descriptionLength += strlen(name) + 1;
      *--name = ' ';
    }
  }
#endif /* HAVE_ICU */

  {
    wchar_t *description = calloc(descriptionLength+1, sizeof(*description));
    if (description) {
      int i;
      for (i=0; i<descriptionLength; i+=1) {
        wint_t wc = convertCharToWchar(buffer[i]);
        description[i] = (wc != WEOF)? wc: WC_C(' ');
      }

      description[characterIndex] = printableCharacter;
      description[brailleIndex] = BRL_UC_ROW | dots;
      description[descriptionLength] = 0;

      if (length) *length = descriptionLength;
      if (braille) *braille = dots;
      return description;
    }
  }

  return NULL;
}

static void
printCharacterString (const wchar_t *wcs) {
  while (*wcs) {
    wint_t wc = *wcs++;
    printw("%" PRIwc, wc);
  }
}

typedef struct {
  TranslationTable translationTable;
  unsigned char currentCharacter;

  brlapi_fileDescriptor brlapiFileDescriptor;
  unsigned int displayWidth;
  unsigned int displayHeight;
  const char *brlapiErrorFunction;
  const char *brlapiErrorMessage;
} EditTableData;

static int
haveBrailleDisplay (EditTableData *data) {
  return data->brlapiFileDescriptor != (brlapi_fileDescriptor)-1;
}

static void
setBrlapiError (EditTableData *data, const char *function) {
  if ((data->brlapiErrorFunction = function)) {
    data->brlapiErrorMessage = brlapi_strerror(&brlapi_error);
  } else {
    data->brlapiErrorMessage = NULL;
  }
}

static void
releaseBrailleDisplay (EditTableData *data) {
  brlapi_closeConnection();
  data->brlapiFileDescriptor = (brlapi_fileDescriptor)-1;
}

static int
claimBrailleDisplay (EditTableData *data) {
  data->brlapiFileDescriptor = brlapi_openConnection(NULL, NULL);
  if (haveBrailleDisplay(data)) {
    if (brlapi_getDisplaySize(&data->displayWidth, &data->displayHeight) != -1) {
      if (brlapi_enterTtyMode(BRLAPI_TTY_DEFAULT, NULL) != -1) {
        setBrlapiError(data, NULL);
        return 1;
      } else {
        setBrlapiError(data, "brlapi_enterTtyMode");
      }
    } else {
      setBrlapiError(data, "brlapi_getDisplaySize");
    }

    releaseBrailleDisplay(data);
  } else {
    setBrlapiError(data, "brlapi_openConnection");
  }

  return 0;
}

static int
updateCharacterDescription (EditTableData *data) {
  int ok = 0;

  size_t descriptionLength;
  unsigned char descriptionDots;
  wchar_t *descriptionText = makeCharacterDescription(
    data->translationTable,
    data->currentCharacter,
    &descriptionLength,
    &descriptionDots
  );

  if (descriptionText) {
    ok = 1;
    clear();

#if defined(USE_CURSES)
    printw("F2:Save F8:Exit\n");
    printw("Up:Up1 Dn:Down1 PgUp:Up16 PgDn:Down16 Home:First End:Last\n");
    printw("\n");
#else /* standard input/output */
#endif /* clear screen */

    printCharacterString(descriptionText);
    printw("\n");

#define DOT(n) ((descriptionDots & BRLAPI_DOT##n)? '#': ' ')
    printw(" +---+ \n");
    printw("1|%c %c|4\n", DOT(1), DOT(4));
    printw("2|%c %c|5\n", DOT(2), DOT(5));
    printw("3|%c %c|6\n", DOT(3), DOT(6));
    printw("7|%c %c|8\n", DOT(7), DOT(8));
    printw(" +---+ \n");
#undef DOT

    if (data->brlapiErrorFunction) {
      printw("BrlAPI error: %s: %s\n",
             data->brlapiErrorFunction, data->brlapiErrorMessage);
      setBrlapiError(data, NULL);
    }

    refresh();

    if (haveBrailleDisplay(data)) {
      brlapi_writeArguments_t args = BRLAPI_WRITEARGUMENTS_INITIALIZER;
      wchar_t text[data->displayWidth];
      size_t length = MIN(data->displayWidth, descriptionLength);

      wmemcpy(text, descriptionText, length);
      wmemset(&text[length], WC_C(' '), data->displayWidth-length);

      args.regionBegin = 1;
      args.regionSize = data->displayWidth;
      args.text = (char *)text;
      args.textSize = data->displayWidth * sizeof(text[0]);
      args.charset = "WCHAR_T";

      if (brlapi_write(&args) == -1) {
        setBrlapiError(data, "brlapi_write");
        releaseBrailleDisplay(data);
      }
    }

    free(descriptionText);
  }

  return ok;
}

static int
doKeyboardCommand (EditTableData *data) {
#if defined(USE_CURSES)
#ifdef USE_FUNC_GET_WCH
  wint_t ch;
  int ret = get_wch(&ch);

  if (ret == KEY_CODE_YES)
#else /* USE_FUNC_GET_WCH */
  int ch = getch();

  if (ch >= 0X100)
#endif /* USE_FUNC_GET_WCH */
  {
    switch (ch) {
      case KEY_UP:
        data->currentCharacter -= 1;
        break;

      case KEY_DOWN:
        data->currentCharacter += 1;
        break;

      case KEY_PPAGE:
        data->currentCharacter -= 0X10;
        break;

      case KEY_NPAGE:
        data->currentCharacter += 0X10;
        break;

      case KEY_HOME:
        data->currentCharacter = 0;
        break;

      case KEY_END:
        data->currentCharacter = 0XFF;
        break;

      case KEY_F(2): {
        FILE *outputFile;

        if (!outputPath) outputPath = inputPath;
        if (!outputFormat) outputFormat = inputFormat;
        outputFile = openTable(&outputPath, "w", NULL, stdout, "<standard-output>");

        if (outputFile) {
          outputFormat->write(outputPath, outputFile, data->translationTable, outputFormat->data);
          fclose(outputFile);
        }

        break;
      }

      case KEY_F(8):
        return 0;

      default:
        beep();
        break;
    }
  } else

#ifdef USE_FUNC_GET_WCH
  if (ret == OK)
#endif /* USE_FUNC_GET_WCH */
#else /* standard input/output */
  wint_t ch = fgetwc(stdin);
  if (ch == WEOF) return 0;
#endif /* read character */
  {
    if ((ch >= BRL_UC_ROW) && (ch <= (BRL_UC_ROW|0xFF))) {
      /* Set braille pattern */
      data->translationTable[data->currentCharacter] = ch & 0XFF;
    } else {
      /* Switch to char */
      int c = convertWcharToChar(ch);
      if (c != EOF) {
        data->currentCharacter = c;
      } else {
        beep();
      }
    }
  }

  return 1;
}

static int
doBrailleCommand (EditTableData *data) {
  if (haveBrailleDisplay(data)) {
    brlapi_keyCode_t key;
    int ret = brlapi_readKey(0, &key);

    if (ret == 1) {
      unsigned long code = key & BRLAPI_KEY_CODE_MASK;

      switch (key & BRLAPI_KEY_TYPE_MASK) {
        case BRLAPI_KEY_TYPE_CMD:
          switch (code & BRLAPI_KEY_CMD_BLK_MASK) {
            case 0:
              switch (code) {
                case BRLAPI_KEY_CMD_LNUP:
                  data->currentCharacter -= 1;
                  break;

                case BRLAPI_KEY_CMD_LNDN:
                  data->currentCharacter += 1;
                  break;

                case BRLAPI_KEY_CMD_PRPGRPH:
                  data->currentCharacter -= 0X10;
                  break;

                case BRLAPI_KEY_CMD_NXPGRPH:
                  data->currentCharacter += 0X10;
                  break;

                case BRLAPI_KEY_CMD_TOP_LEFT:
                case BRLAPI_KEY_CMD_TOP:
                  data->currentCharacter = 0;
                  break;

                case BRLAPI_KEY_CMD_BOT_LEFT:
                case BRLAPI_KEY_CMD_BOT:
                  data->currentCharacter = 0XFF;
                  break;

                default:
                  beep();
                  break;
              }
              break;

            case BRLAPI_KEY_CMD_PASSDOTS:
              data->translationTable[data->currentCharacter] = code & BRLAPI_KEY_CMD_ARG_MASK;
              break;

            default:
              beep();
              break;
          }
          break;

        case BRLAPI_KEY_TYPE_SYM: {
          /* latin1 */
          if (code < 0X100) code |= BRLAPI_KEY_SYM_UNICODE;

          if ((code & 0X1f000000) == BRLAPI_KEY_SYM_UNICODE) {
            /* unicode */
            if ((code & 0Xffff00) == BRL_UC_ROW) {
              /* Set braille pattern */
              data->translationTable[data->currentCharacter] = code & 0XFF;
            } else {
              /* Switch to char */
              int c = convertWcharToChar(code & 0XFFFFFF);
              if (c != EOF) {
                data->currentCharacter = c;
              } else {
                beep();
              }
            }
          } else {
            switch (code) {
              case BRLAPI_KEY_SYM_UP:
                data->currentCharacter -= 1;
                break;

              case BRLAPI_KEY_SYM_DOWN:
                data->currentCharacter += 1;
                break;

              case BRLAPI_KEY_SYM_PAGE_UP:
                data->currentCharacter -= 0X10;
                break;

              case BRLAPI_KEY_SYM_PAGE_DOWN:
                data->currentCharacter += 0X10;
                break;

              case BRLAPI_KEY_SYM_HOME:
                data->currentCharacter = 0;
                break;

              case BRLAPI_KEY_SYM_END:
                data->currentCharacter = 0XFF;
                break;

              default:
                beep();
                break;
            }
          }
          break;
        }

        default:
          beep();
          break;
      }
    } else if (ret == -1) {
      setBrlapiError(data, "brlapi_readKey");
      releaseBrailleDisplay(data);
    }
  }

  return 1;
}

static int
editTable (void) {
  int status;
  EditTableData data;

  memset(data.translationTable, 0xFF, sizeof(data.translationTable));

  {
    FILE *inputFile = openTable(&inputPath, "r", opt_dataDirectory, NULL, NULL);

    if (inputFile) {
      if (inputFormat->read(inputPath, inputFile, data.translationTable, inputFormat->data)) {
        status = 0;
      } else {
        status = 4;
      }

      fclose(inputFile);
    } else {
      status = 3;
    }
  }

  if (!status) {
    claimBrailleDisplay(&data);

#if defined(USE_CURSES)
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
#else /* standard input/output */
#endif /* initialize keyboard and screen */

    data.currentCharacter = 0;
    while (updateCharacterDescription(&data)) {
      fd_set set;
      FD_ZERO(&set);

      {
        int maximumFileDescriptor = STDIN_FILENO;
        FD_SET(STDIN_FILENO, &set);

        if (haveBrailleDisplay(&data)) {
          FD_SET(data.brlapiFileDescriptor, &set);
          if (data.brlapiFileDescriptor > maximumFileDescriptor)
            maximumFileDescriptor = data.brlapiFileDescriptor;
        }

        select(maximumFileDescriptor+1, &set, NULL, NULL, NULL);
      }

      if (FD_ISSET(STDIN_FILENO, &set))
        if (!doKeyboardCommand(&data))
          break;

      if (haveBrailleDisplay(&data) && FD_ISSET(data.brlapiFileDescriptor, &set))
        if (!doBrailleCommand(&data))
          break;
    }

    clear();
    refresh();

#if defined(USE_CURSES)
    endwin();
#else /* standard input/output */
#endif /* restore keyboard and screen */

    if (haveBrailleDisplay(&data)) releaseBrailleDisplay(&data);
  }

  return status;
}
#endif /* ENABLE_API */

static int
translateText (void) {
  int status;
  FILE *inputFile = openTable(&inputPath, "r", opt_dataDirectory, NULL, NULL);

  if (inputFile) {
    TranslationTable inputTable;

    if (inputFormat->read(inputPath, inputFile, inputTable, inputFormat->data)) {
      if (outputPath) {
        FILE *outputFile = openTable(&outputPath, "r", opt_dataDirectory, NULL, NULL);

        if (outputFile) {
          TranslationTable outputTable;

          if (outputFormat->read(outputPath, outputFile, outputTable, outputFormat->data)) {
            TranslationTable table;

            {
              TranslationTable unoutputTable;
              int byte;
              reverseTranslationTable(outputTable, unoutputTable);
              memset(&table, 0, sizeof(table));
              for (byte=TRANSLATION_TABLE_SIZE-1; byte>=0; byte--)
                table[byte] = unoutputTable[inputTable[byte]];
            }

            status = 0;
            while (1) {
              int character;

              if ((character = fgetc(stdin)) == EOF) {
                if (ferror(stdin)) {
                  LogPrint(LOG_ERR, "input error: %s", strerror(errno));
                  status = 6;
                }
                break;
              }

              if (fputc(table[character], stdout) == EOF) {
                LogPrint(LOG_ERR, "output error: %s", strerror(errno));
                status = 7;
                break;
              }
            }
          } else {
            status = 4;
          }

          if (fclose(outputFile) == EOF) {
            if (!status) {
              LogPrint(LOG_ERR, "output error: %s", strerror(errno));
              status = 7;
            }
          }
        } else {
          status = 3;
        }
      } else {
        LogPrint(LOG_ERR, "output table not specified.");
        status = 2;
      }
    } else {
      status = 4;
    }

    fclose(inputFile);
  } else {
    status = 3;
  }

  return status;
}

