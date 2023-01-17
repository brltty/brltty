/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2023 by
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

package org.a11y.brlapi.commands;
import org.a11y.brlapi.*;
import org.a11y.brlapi.programs.*;
import org.a11y.brlapi.clients.*;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

public class ApiToolCommand extends Program {
  private final static KeywordMap<Class<? extends Program>> knownPrograms =
                   new KeywordMap<>();

  private static void addProgram (Class<? extends Program> type) {
    String name = type.getSimpleName();

    if (isClient(type)) {
      name = Strings.replaceAll(name, "Client$", "");
    } else {
      name = Strings.replaceAll(name, "Program$", "");
    }

    name = toOperandName(Strings.wordify(name));
    knownPrograms.put(name, type);
  }

  static {
    addProgram(ApiErrorClient.class);
    addProgram(ApiExceptionClient.class);
    addProgram(BoundCommandsClient.class);
    addProgram(ComputerBrailleClient.class);
    addProgram(DriverKeysClient.class);
    addProgram(EchoClient.class);
    addProgram(GetDriverClient.class);
    addProgram(GetModelClient.class);
    addProgram(GetSizeClient.class);
    addProgram(ListParametersClient.class);
    addProgram(PauseClient.class);
    addProgram(SetParameterClient.class);
    addProgram(VersionProgram.class);
    addProgram(WriteArgumentsClient.class);
    addProgram(WriteDotsClient.class);
    addProgram(WriteTextClient.class);
  }

  public ApiToolCommand (String... arguments) {
    super(arguments);
    addRequiredParameters("program/client");
    addRepeatingParameter("argument");
  }

  @Override
  public String getPurpose () {
    return "Command line access to the functionality provided by the BrlAPI interface.";
  }

  @Override
  protected final void extendUsageSummary (StringBuilder usage) {
    super.extendUsageSummary(usage);

    if (knownPrograms.isEmpty()) {
      usage.append("\nNo programs or clients have been defined.");
    } else {
      usage.append("\nThese programs and clients have been defined")
           .append(" (each has its own -help option):");

      for (String name : knownPrograms.getKeywords()) {
        usage.append("\n  ").append(name);
      }
    }
  }

  private String programName = null;
  private Class<? extends Program> programType = null;
  private String[] programArguments = null;

  @Override
  protected final void processParameters (String[] parameters)
            throws SyntaxException
  {
    int count = parameters.length;

    if (count == 0) {
      throw new SyntaxException("missing program/client name");
    }

    programName = parameters[0];
    programType = knownPrograms.get(programName);

    if (programType == null) {
      throw new SyntaxException("unknown program/client: %s", programName);
    }

    count -= 1;
    programArguments = new String[count];
    System.arraycopy(parameters, 1, programArguments, 0, count);
  }

  @Override
  protected final void runProgram () throws ProgramException {
    String objectName = getObjectName();
    Program program = null;

    try {
      Constructor<? extends Program> constructor = programType.getConstructor(
        programArguments.getClass()
      );

      program = (Program)constructor.newInstance((Object)programArguments);
    } catch (NoSuchMethodException exception) {
      throw new ProgramException(
        "constructor not found: %s",
        objectName
      );
    } catch (InstantiationException exception) {
      throw new ProgramException(
        "instantiation failed: %s: %s",
        objectName, exception.getMessage()
      );
    } catch (IllegalAccessException exception) {
      throw new ProgramException(
        "access denied: %s: %s",
        objectName, exception.getMessage()
      );
    } catch (InvocationTargetException exception) {
      Throwable cause = exception.getCause();

      String message = String.format(
        "construction failed: %s: %s",
        objectName, cause.getClass().getSimpleName()
      );

      {
        String problem = cause.getMessage();
        if (problem != null) message += ": " + problem;
      }

      throw new ProgramException(message);
    }

    program.setProgramName(programName)
           .run();
  }

  public static void main (String... arguments) {
    try {
      new ApiToolCommand(arguments)
         .setProgramName("apitool")
         .run();
    } catch (ExitException exception) {
      System.exit(exception.getExitCode());
    }
  }
}
