/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
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

package org.a11y.brltty.core;

import java.util.List;
import java.util.ArrayList;

/* options not yet supported:
 * -E           --environment-variables
 * -A arg,...   --api-parameters=
 * -N           --no-api
 * -B arg,...   --braille-parameters=
 * -r           --release-device
 * -K arg,...   --keyboard-properties=
 * -s driver    --speech-driver=
 * -S arg,...   --speech-parameters=
 * -i file      --speech-input=
 * -x driver    --screen-driver=
 * -X arg,...   --screen-parameters=
 * -p device    --pcm-device=
 * -m device    --midi-device=
 * -U csecs     --update-interval=
 * -M csecs     --message-delay=
 * -e           --standard-error
 * -v           --verify
 */

public class ArgumentsBuilder {
  private boolean foregroundExecution = false;
  private boolean releaseDevice = false;
  private String logLevel = null;
  private String logFile = null;
  private String configurationFile = null;
  private String preferencesFile = null;
  private String driversDirectory = null;
  private String writableDirectory = null;
  private String updatableDirectory = null;
  private String tablesDirectory = null;
  private String textTable = null;
  private String attributesTable = null;
  private String contractionTable = null;
  private String keyboardTable = null;
  private String brailleDriver = null;
  private String brailleDevice = null;
  private String speechDriver = null;
  private boolean quietIfNoBraille = false;
  private boolean apiEnabled = false;
  private String apiParameters = null;

  public final boolean getForegroundExecution () {
    return  foregroundExecution;
  }

  public final ArgumentsBuilder setForegroundExecution (boolean value) {
    foregroundExecution = value;
    return this;
  }

  public final boolean getReleaseDevice () {
    return  releaseDevice;
  }

  public final ArgumentsBuilder setReleaseDevice (boolean value) {
    releaseDevice = value;
    return this;
  }

  public final String getLogLevel () {
    return  logLevel;
  }

  public final ArgumentsBuilder setLogLevel (String value) {
    logLevel = value;
    return this;
  }

  public final String getLogFile () {
    return  logFile;
  }

  public final ArgumentsBuilder setLogFile (String value) {
    logFile = value;
    return this;
  }

  public final String getConfigurationFile () {
    return  configurationFile;
  }

  public final ArgumentsBuilder setConfigurationFile (String value) {
    configurationFile = value;
    return this;
  }

  public final String getPreferencesFile () {
    return  preferencesFile;
  }

  public final ArgumentsBuilder setPreferencesFile (String value) {
    preferencesFile = value;
    return this;
  }

  public final String getDriversDirectory () {
    return  driversDirectory;
  }

  public final ArgumentsBuilder setDriversDirectory (String value) {
    driversDirectory = value;
    return this;
  }

  public final String getWritableDirectory () {
    return  writableDirectory;
  }

  public final ArgumentsBuilder setWritableDirectory (String value) {
    writableDirectory = value;
    return this;
  }

  public final String getUpdatableDirectory () {
    return  updatableDirectory;
  }

  public final ArgumentsBuilder setUpdatableDirectory (String value) {
    updatableDirectory = value;
    return this;
  }

  public final String getTablesDirectory () {
    return  tablesDirectory;
  }

  public final ArgumentsBuilder setTablesDirectory (String value) {
    tablesDirectory = value;
    return this;
  }

  public final String getTextTable () {
    return  textTable;
  }

  public final ArgumentsBuilder setTextTable (String value) {
    textTable = value;
    return this;
  }

  public final String getAttributesTable () {
    return  attributesTable;
  }

  public final ArgumentsBuilder setAttributesTable (String value) {
    attributesTable = value;
    return this;
  }

  public final String getContractionTable () {
    return  contractionTable;
  }

  public final ArgumentsBuilder setContractionTable (String value) {
    contractionTable = value;
    return this;
  }

  public final String getKeyboardTable () {
    return  keyboardTable;
  }

  public final ArgumentsBuilder setKeyboardTable (String value) {
    keyboardTable = value;
    return this;
  }

  public final String getBrailleDriver () {
    return  brailleDriver;
  }

  public final ArgumentsBuilder setBrailleDriver (String value) {
    brailleDriver = value;
    return this;
  }

  public final String getBrailleDevice () {
    return  brailleDevice;
  }

  public final ArgumentsBuilder setBrailleDevice (String value) {
    brailleDevice = value;
    return this;
  }

  public final String getSpeechDriver () {
    return  speechDriver;
  }

  public final ArgumentsBuilder setSpeechDriver (String value) {
    speechDriver = value;
    return this;
  }

  public final boolean getQuietIfNoBraille () {
    return  quietIfNoBraille;
  }

  public final ArgumentsBuilder setQuietIfNoBraille (boolean value) {
    quietIfNoBraille = value;
    return this;
  }

  public final boolean getApiEnabled () {
    return  apiEnabled;
  }

  public final ArgumentsBuilder setApiEnabled (boolean value) {
    apiEnabled = value;
    return this;
  }

  public final String getApiParameters () {
    return apiParameters;
  }

  public final ArgumentsBuilder setApiParameters (String value) {
    apiParameters = value;
    return this;
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
    addOption(arguments, "-n", foregroundExecution);

    addOption(arguments, "-l", logLevel);
    addOption(arguments, "-L", logFile);

    addOption(arguments, "-f", configurationFile);
    addOption(arguments, "-F", preferencesFile);

    addOption(arguments, "-D", driversDirectory);
    addOption(arguments, "-W", writableDirectory);
    addOption(arguments, "-U", updatableDirectory);

    addOption(arguments, "-T", tablesDirectory);
    addOption(arguments, "-t", textTable);
    addOption(arguments, "-a", attributesTable);
    addOption(arguments, "-c", contractionTable);
    addOption(arguments, "-k", keyboardTable);

    addOption(arguments, "-b", brailleDriver);
    addOption(arguments, "-d", brailleDevice);
    addOption(arguments, "-r", releaseDevice);

    addOption(arguments, "-s", speechDriver);
    addOption(arguments, "-Q", quietIfNoBraille);

    addOption(arguments, "-N", !apiEnabled);
    addOption(arguments, "-A", apiParameters);

    return arguments.toArray(new String[arguments.size()]);
  }
}
