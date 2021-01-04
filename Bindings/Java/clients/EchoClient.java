/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2021 by
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *   Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brlapi.clients;
import org.a11y.brlapi.*;

import java.io.InterruptedIOException;
import java.util.concurrent.TimeoutException;

public class EchoClient extends Client {
  private boolean echoDriverKeys = false;
  private boolean echoNames = false;

  public final static int MINIMUM_READ_COUNT =  1;
  public final static int DEFAULT_READ_COUNT = 10;

  public final static String NO_TIMEOUT = "none";
  public final static int FOREVER_READ_TIMEOUT =  0;
  public final static int MINIMUM_READ_TIMEOUT =  1;
  public final static int DEFAULT_READ_TIMEOUT = 10;

  private int readCount = DEFAULT_READ_COUNT;
  private final OperandUsage readCountUsage = new OperandUsage("read count")
    .setDefault(readCount)
    .setRangeMinimum(MINIMUM_READ_COUNT)
    ;

  private int readTimeout = DEFAULT_READ_TIMEOUT;
  private final OperandUsage readTimeoutUsage = new OperandUsage("read timeout")
    .setDefault(readTimeout)
    .setRangeMinimum(MINIMUM_READ_TIMEOUT)
    .setRangeUnits("seconds")
    .setRangeComment("readKeyWithTimeout is used")
    .addWord(NO_TIMEOUT, FOREVER_READ_TIMEOUT, "readKey is used")
    ;

  public EchoClient (String... arguments) {
    super(arguments);
    addRepeatingParameter("tty");

    addOption("commands",
      (operands) -> {
        echoDriverKeys = false;
      }
    );

    addOption("keys",
      (operands) -> {
        echoDriverKeys = true;
      }
    );

    addOption("values",
      (operands) -> {
        echoNames = false;
      }
    );

    addOption("names",
      (operands) -> {
        echoNames = true;
      }
    );

    addOption("reads",
      (operands) -> {
        readCount = Parse.asInt(
          readCountUsage.getOperandDescription(),
          operands[0], MINIMUM_READ_COUNT
        );
      },
      "count"
    );

    addOption("timeout",
      (operands) -> {
        String operand = operands[0];

        if (Strings.isAbbreviation(NO_TIMEOUT, operand)) {
          readTimeout = FOREVER_READ_TIMEOUT;
        } else {
          readTimeout = Parse.asInt(
            readTimeoutUsage.getOperandDescription(),
            operand, MINIMUM_READ_TIMEOUT
          );
        }
      },
      "seconds"
    );
  }

  @Override
  protected final void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    readCountUsage.appendTo(usage);
    readTimeoutUsage.appendTo(usage);
  }

  private int[] ttyPath = null;

  @Override
  protected void processParameters (String[] parameters)
            throws SyntaxException
  {
    int count = parameters.length;
    ttyPath = new int[count];

    for (int i=0; i<count; i+=1) {
      ttyPath[i] = Parse.asInt("tty path component", parameters[i]);
    }
  }

  private final void show (Connection connection, String text) {
    printf("%s\n", text);
    connection.write(text, Constants.CURSOR_OFF);
  }

  private final String toString (Connection connection, long code) {
    if (echoDriverKeys) {
      DriverKeycode keycode = new DriverKeycode(code);
      String string = keycode.toString();

      if (echoNames) {
        String name = connection.getParameters().driverKeycodeName.get(keycode.getValue());

        if ((name != null) && !name.isEmpty()) {
          string += " Name:" + name;
        }
      }

      return string;
    } else {
      CommandKeycode keycode = new CommandKeycode(code);
      if (!echoNames) return keycode.toString();

      StringBuilder builder = new StringBuilder();
      builder.append(Keycode.toString(code));

      {
        String type = keycode.getTypeName();
        if (type != null) builder.append(" Type:").append(type);
      }

      {
        String command = keycode.getCommandName();
        if (command != null) builder.append(" Cmd:").append(command);
      }

      {
        int argument = keycode.getArgument();

        if (argument > 0) {
          builder.append(
            String.format(
              " Arg:%04X(%d)", argument, argument
            )
          );
        }
      }

      {
        String[] flags = keycode.getFlagNames();

        if (flags != null) {
          int count = flags.length;
          boolean first = true;

          for (int index=0; index<count; index+=1) {
            String flag = flags[index];
            if (flag == null) continue;
            if (flag.isEmpty()) continue;

            if (first) {
              first = false;
              builder.append(" Flg:");
            } else {
              builder.append(',');
            }

            builder.append(flag);
          }
        }
      }

      return builder.toString();
    }
  }

  @Override
  protected final void runClient (Connection connection) throws ProgramException {
    ttyMode(
      connection, echoDriverKeys,
      (con) -> {
        show(con,
          String.format(
            "press keys (timeout is %d seconds)", readTimeout
          )
        );

        while (readCount-- > 0) {
          long code;

          try {
            if (readTimeout == FOREVER_READ_TIMEOUT) {
              code = con.readKey();
            } else {
              code = con.readKeyWithTimeout((readTimeout * 1000));
            }
          } catch (InterruptedIOException exception) {
            continue;
          } catch (TimeoutException exception) {
            break;
          }

          show(con, toString(con, code));
        }
      },
      ttyPath
    );
  }
}
