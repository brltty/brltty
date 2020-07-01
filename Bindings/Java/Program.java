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

import java.util.List;
import java.util.ArrayList;

public abstract class Program extends ProgramComponent implements Runnable {
  protected abstract void runProgram () throws ProgramException;

  public final String getName () {
    return getClass().getSimpleName();
  }

  protected final void writeProgramMessage (String format, Object... arguments) {
    System.err.println((getName() + ": " + String.format(format, arguments)));
  }

  protected static class Option {
    public final static char PREFIX_CHARACTER = '-';

    public interface Handler {
      public void handleOption (String[] operands) throws SyntaxException;
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

    public final String[] getOperands () {
      int count = operandDescriptions.length;
      String[] result = new String[count];
      System.arraycopy(operandDescriptions, 0, result, 0, count);
      return result;
    }
  }

  private final String[] programArguments;
  private final KeywordMap<Option> programOptions = new KeywordMap<>();

  protected final void addOption (String keyword, Option.Handler handler, String... operands) {
    programOptions.put(keyword, new Option(handler, operands));
  }

  private List<String> requiredParameters = null;
  private List<String> optionalParameters = null;

  protected final void addRequiredParameters (String... parameters) {
    if (optionalParameters != null) {
      throw new IllegalStateException("optional parameter(s) already added");
    }

    if (requiredParameters == null) {
      requiredParameters = new ArrayList<>();
    }

    for (String parameter : parameters) {
      requiredParameters.add(parameter);
    }
  }

  protected final void addOptionalParameters (String... parameters) {
    if (optionalParameters == null) {
      optionalParameters = new ArrayList<>();
    }

    for (String parameter : parameters) {
      optionalParameters.add(parameter);
    }
  }

  protected void extendUsageSummary (StringBuilder usage) {
  }

  public final String getUsageSummary () {
    StringBuilder usage = new StringBuilder();

    usage.append("Usage Summary for ");
    usage.append(getName());

    usage.append("\nSyntax:");
    boolean haveOptions = !programOptions.isEmpty();

    if (haveOptions) {
      usage.append(" [");
      usage.append(Option.PREFIX_CHARACTER);
      usage.append("option ...]");
    }

    if (requiredParameters != null) {
      for (String parameter : requiredParameters) {
        usage.append(' ');
        usage.append(toName(parameter));
      }
    }

    if (optionalParameters != null) {
      for (String parameter : optionalParameters) {
        usage.append(" [");
        usage.append(toName(parameter));
      }

      for (int i=optionalParameters.size(); i>0; i-=1) {
        usage.append(']');
      }
    }

    if (haveOptions) {
      usage.append("\n\nThese options may be specified:");

      for (String keyword : programOptions.getKeywords()) {
        Option option = programOptions.get(keyword);

        usage.append("\n  ");
        usage.append(Option.PREFIX_CHARACTER);
        usage.append(keyword);

        for (String operand : option.getOperands()) {
          usage.append(' ');
          usage.append(toName(operand));
        }
      }
    }

    {
      StringBuilder extension = new StringBuilder();
      extendUsageSummary(extension);

      extension.setLength(findTrailingWhitespace(extension));
      extension.delete(0, findNonemptyLine(extension));

      if (extension.length() > 0) {
        usage.append("\n\n");
        usage.append(extension);
      }
    }

    return usage.toString();
  }

  public final void showUsageSummary () {
    System.out.println(getUsageSummary());
    System.exit(0);
  }

  protected Program (String... arguments) {
    super();
    programArguments = arguments;

    addOption("help",
      (operands) -> {
        showUsageSummary();
      }
    );
  }

  protected void processParameters (String[] parameters)
            throws SyntaxException
  {
    if (parameters.length > 0) {
      throw new TooManyParametersException(parameters, 0);
    }
  }

  private final void processArguments (String[] arguments)
          throws SyntaxException
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

      if (option == null) {
        throw new SyntaxException("unknown option: %s", argument);
      }

      String[] operandDescriptions = option.getOperands();
      int operandCount = operandDescriptions.length;

      {
        int index = argumentCount - argumentIndex - 1;

        if (index < operandCount) {
          throw new SyntaxException(
            "missing %s: %s", operandDescriptions[index], argument
          );
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

  protected void onProgramException (ProgramException exception) {
    writeProgramMessage("%s", exception.getMessage());
    int exitCode;

    if (exception instanceof SyntaxException) {
      exitCode = 2;
    } else if (exception instanceof SemanticException) {
      exitCode = 3;
    } else {
      exitCode = 4;
    }

    System.exit(exitCode);
  }

  @Override
  public final void run () {
    try {
      processArguments(programArguments);
      runProgram();
    } catch (ProgramException exception) {
      onProgramException(exception);
    }
  }
}
