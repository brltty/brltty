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

