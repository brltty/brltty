/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

package org.a11y.BRLTTY.Core;

import java.util.List;
import java.util.ArrayList;

public class ArgumentsBuilder {
  private LogLevel logLevel = null;
  public LogLevel setLogLevel (LogLevel newValue) {
    LogLevel oldValue = logLevel;
    logLevel = newValue;
    return oldValue;
  }

  private String logFilePath = null;
  public String setLogFilePath (String newValue) {
    String oldValue = logFilePath;
    logFilePath = newValue;
    return oldValue;
  }

  private boolean foregroundExecution = false;
  public boolean setForegroundExecution (boolean newValue) {
    boolean oldValue = foregroundExecution;
    foregroundExecution = newValue;
    return oldValue;
  }

  protected void addOption (List<String> arguments, String option, String value) {
    if (value != null) {
      if (value.length() > 0) {
        arguments.add(option);
        arguments.add(value);
      }
    }
  }

  protected void addOption (List<String> arguments, String option, Enum value) {
    if (value != null) {
      addOption(arguments, option, value.name());
    }
  }

  protected void addOption (List<String> arguments, String option, boolean value) {
    if (value) {
      arguments.add(option);
    }
  }

  public String[] getArguments () {
    List<String> arguments = new ArrayList<String>();
    addOption(arguments, "-l", logLevel);
    addOption(arguments, "-L", logFilePath);
    addOption(arguments, "-n", foregroundExecution);
    return arguments.toArray(new String[arguments.size()]);
  }
}
