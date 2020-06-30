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
import org.a11y.brlapi.programs.*;
import org.a11y.brlapi.clients.*;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

public abstract class Programs extends ProgramComponent {
  private Programs () {
  }

  private final static KeywordMap<Class<? extends Program>> programs = new KeywordMap<>();

  private static void addProgram (Class<? extends Program> type) {
    String name = type.getSimpleName();

    if (Client.class.isAssignableFrom(type)) {
      name = name.replaceAll("Client$", "");
    } else {
      name = name.replaceAll("Program$", "");
    }

    programs.put(toName(wordify(name)), type);
  }

  static {
    addProgram(BoundCommandsClient.class);
    addProgram(ComputerBrailleClient.class);
    addProgram(DriverKeysClient.class);
    addProgram(EchoClient.class);
    addProgram(ListParametersClient.class);
    addProgram(PauseClient.class);
    addProgram(SetParameterClient.class);
    addProgram(VersionProgram.class);
  }

  private static class MainProgram extends Program {
    public MainProgram (String... arguments) {
      super(arguments);
      addRequiredParameters("program/client");
      addOptionalParameters("arguments");
    }

    @Override
    protected void extendUsageSummary (StringBuilder usage) {
      super.extendUsageSummary(usage);
      usage.append("\n\n");

      if (programs.isEmpty()) {
        usage.append("No programs or clients have been defined.");
      } else {
        usage.append("These programs and clients have been defined:");

        for (String name : programs.getKeywords()) {
          usage.append("\n  ");
          usage.append(name);
        }
      }
    }

    private Program programObject = null;

    @Override
    protected final void processParameters (String[] parameters) {
      int count = parameters.length;
      if (count == 0) onSyntaxError("missing program/client name");

      String name = parameters[0];
      Class<? extends Program> type = programs.get(name);
      if (type == null) onSyntaxError("unknown program/client: %s", name);
      String term = Client.class.isAssignableFrom(type)? "client": "program";

      count -= 1;
      String[] arguments = new String[count];
      System.arraycopy(parameters, 1, arguments, 0, count);

      try {
        Constructor<? extends Program> constructor = type.getConstructor(
          arguments.getClass()
        );

        programObject = (Program)constructor.newInstance((Object)arguments);
      } catch (NoSuchMethodException exception) {
        onInternalError("%s constructor not found: %s", term, exception.getMessage());
      } catch (InstantiationException exception) {
        onInternalError("%s instantiation failed: %s", term, exception.getMessage());
      } catch (IllegalAccessException exception) {
        onInternalError("%s object access denied: %s", term, exception.getMessage());
      } catch (InvocationTargetException exception) {
        onInternalError("%s construction failed: %s", term, exception.getCause().getMessage());
      }
    }

    @Override
    protected final void runProgram () {
      programObject.run();
    }
  }

  public static void main (String arguments[]) {
    new MainProgram(arguments).run();
  }
}
