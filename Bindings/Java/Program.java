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

package org.a11y.brlapi;

import java.util.List;
import java.util.ArrayList;

public abstract class Program extends ProgramComponent implements Runnable {
  protected abstract void runProgram () throws ProgramException;

  public final boolean isClient () {
    return isClient(this);
  }

  public final String getName () {
    return getProgramName(getClass());
  }

  protected final void writeProgramMessage (String format, Object... arguments) {
    System.err.println((getName() + ": " + String.format(format, arguments)));
  }

  protected static class Option {
    public final static char PREFIX_CHARACTER = '-';

    public interface Handler {
      public void handleOption (String[] operands) throws SyntaxException;
    }

    private final String optionName;
    private final Handler optionHandler;
    private final String[] operandDescriptions;

    public Option (String name, Handler handler, String... operands) {
      optionName = name;
      optionHandler = handler;
      operandDescriptions = operands;
    }

    public final String getName () {
      return optionName;
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

  protected final void addOption (String name, Option.Handler handler, String... operands) {
    programOptions.put(name, new Option(name, handler, operands));
  }

  private List<String> requiredParameters = null;
  private List<String> optionalParameters = null;
  private boolean haveRepeatingParameter = false;

  protected final void addRequiredParameters (String... parameters) {
    if (optionalParameters != null) {
      throw new IllegalStateException("optional parameters already added");
    }

    if (requiredParameters == null) {
      requiredParameters = new ArrayList<>();
    }

    for (String parameter : parameters) {
      requiredParameters.add(parameter);
    }
  }

  protected final void addOptionalParameters (String... parameters) {
    if (haveRepeatingParameter) {
      throw new IllegalStateException("repeating parameter already added");
    }

    if (optionalParameters == null) {
      optionalParameters = new ArrayList<>();
    }

    for (String parameter : parameters) {
      optionalParameters.add(parameter);
    }
  }

  protected final void addRepeatingParameter (String parameter) {
    addOptionalParameters(parameter);
    haveRepeatingParameter = true;
  }

  public final static char USAGE_OPTIONAL_BEGIN = '[';
  public final static char USAGE_OPTIONAL_END = ']';
  public final static String USAGE_REPEATING_INDICATOR = "...";

  protected void extendUsageSummary (StringBuilder usage) {
  }

  public final String getUsageSummary () {
    StringBuilder usage = new StringBuilder();
    usage.append("Usage Summary for ").append(getName());
    boolean haveOptions = !programOptions.isEmpty();

    {
      usage.append("\nSyntax:");
      int start = usage.length();

      if (haveOptions) {
        usage.append(' ').append(USAGE_OPTIONAL_BEGIN)
             .append(Option.PREFIX_CHARACTER).append("option")
             .append(' ').append(USAGE_REPEATING_INDICATOR)
             .append(USAGE_OPTIONAL_END);
      }

      if (requiredParameters != null) {
        for (String parameter : requiredParameters) {
          usage.append(' ').append(toOperandName(parameter));
        }
      }

      if (optionalParameters != null) {
        for (String parameter : optionalParameters) {
          usage.append(' ').append(USAGE_OPTIONAL_BEGIN)
               .append(toOperandName(parameter));
        }

        if (haveRepeatingParameter) {
          usage.append(' ').append(USAGE_REPEATING_INDICATOR);
        }

        for (int i=optionalParameters.size(); i>0; i-=1) {
          usage.append(USAGE_OPTIONAL_END);
        }
      }

      if (usage.length() == start) usage.append(" (no arguments)");
    }

    if (haveOptions) {
      usage.append("\n\nThese options may be specified:");

      for (String name : programOptions.getKeywords()) {
        Option option = programOptions.get(name);
        usage.append("\n  ").append(Option.PREFIX_CHARACTER).append(name);

        for (String operand : option.getOperands()) {
          usage.append(' ').append(toOperandName(operand));
        }
      }
    }

    {
      StringBuilder extension = new StringBuilder();
      extendUsageSummary(extension);

      String text = Strings.formatParagraphs(extension.toString());
      if (!text.isEmpty()) usage.append("\n\n").append(text);
    }

    return usage.toString();
  }

  protected Program (String... arguments) {
    super();
    programArguments = arguments;

    addOption("help",
      (operands) -> {
        printf("%s\n", getUsageSummary());
        throw new ExitException(EXIT_CODE_SUCCESS);
      }
    );
  }

  protected void processParameters (String[] parameters)
            throws SyntaxException
  {
    if (parameters.length > 0) {
      throw new TooManyParametersException(parameters);
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

      String name = argument.substring(1).toLowerCase();
      Option option = programOptions.get(name);

      if (option == null) {
        throw new SyntaxException("unknown option: %s", argument);
      }

      String[] operandDescriptions = option.getOperands();
      int operandCount = operandDescriptions.length;

      {
        int index = argumentCount - argumentIndex - 1;

        if (index < operandCount) {
          throw new SyntaxException(
            "missing %s: %c%s",
            operandDescriptions[index],
            Option.PREFIX_CHARACTER,
            option.getName()
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
      exitCode = EXIT_CODE_SYNTAX;
    } else if (exception instanceof SemanticException) {
      exitCode = EXIT_CODE_SEMANTIC;
    } else if (exception instanceof ExternalException) {
      exitCode = EXIT_CODE_EXTERNAL;
    } else {
      exitCode = EXIT_CODE_INTERNAL;
    }

    throw new ExitException(exitCode);
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
