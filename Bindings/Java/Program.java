/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2006-2020 by
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

package org.a11y.brlapi;

public abstract class Program extends ProgramComponent implements Runnable {
  private final String[] programArguments;

  protected static class Option {
    public final static char PREFIX_CHARACTER = '-';

    public interface Handler {
      public void handleOption (String[] operands) throws OperandException;
    }

    private final Handler optionHandler;
    private final String[] operandDescriptions;

    public Option (Handler handler, String... operands) {
      optionHandler = handler;
      operandDescriptions = operands;
    }

    public final Handler getHandler () {
      return optionHandler;
    }

    public final String[] getOperandDescriptions () {
      return operandDescriptions;
    }
  }

  private final KeywordMap<Option> programOptions = new KeywordMap<>();

  protected final void addOption (String keyword, Option.Handler handler, String... operands) {
    programOptions.put(keyword, new Option(handler, operands));
  }

  protected Program (String[] arguments) {
    super();
    programArguments = arguments;
  }

  public String getName () {
    return getClass().getSimpleName();
  }

  protected final void writeProgramMessage (String format, Object... arguments) {
    System.err.println((getName() + ": " + String.format(format, arguments)));
  }

  protected final void syntaxError (String format, Object... arguments) {
    writeProgramMessage(format, arguments);
    System.exit(2);
  }

  protected final void semanticError (String format, Object... arguments) {
    writeProgramMessage(format, arguments);
    System.exit(3);
  }

  protected final void internalError (String format, Object... arguments) {
    writeProgramMessage(format, arguments);
    System.exit(4);
  }

  protected final void tooManyParameters () throws OperandException {
    throw new OperandException("too many parameters");
  }

  protected void processParameters (String[] parameters)
            throws OperandException
  {
    if (parameters.length > 0) tooManyParameters();
  }

  private final void processArguments (String[] arguments)
          throws OperandException
  {
    int argumentCount = arguments.length;
    int argumentIndex = 0;

    while (argumentIndex < argumentCount) {
      String argument = arguments[argumentIndex];
      if (argument.isEmpty()) break;
      if (argument.charAt(0) != Option.PREFIX_CHARACTER) break;

      if (argument.length() == 1) {
        argumentIndex += 1;
        break;
      }

      String keyword = argument.substring(1).toLowerCase();
      Option option = programOptions.get(keyword);
      if (option == null) syntaxError("unknown option: %s", argument);

      String[] operandDescriptions = option.getOperandDescriptions();
      int operandCount = operandDescriptions.length;

      {
        int index = argumentCount - argumentIndex - 1;

        if (index < operandCount) {
          syntaxError("missing %s: %s", operandDescriptions[index], argument);
        }
      }

      String[] operands = new String[operandCount];
      System.arraycopy(arguments, argumentIndex+1, operands, 0, operandCount);
      option.getHandler().handleOption(operands);

      argumentIndex += 1 + operandCount;
    }

    int parameterCount = argumentCount - argumentIndex;
    String[] parameters = new String[parameterCount];
    System.arraycopy(arguments, argumentIndex, parameters, 0, parameterCount);
    processParameters(parameters);
  }

  protected abstract void runProgram ();

  @Override
  public final void run () {
    try {
      processArguments(programArguments);
    } catch (OperandException exception) {
      syntaxError(exception.getMessage());
    }

    runProgram();
  }
}
