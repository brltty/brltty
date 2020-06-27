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
    String name = toName(wordify(type.getSimpleName()));
    programs.put(name, type);
  }

  static {
    addProgram(VersionProgram.class);

    addProgram(BoundCommandsClient.class);
    addProgram(ComputerBrailleClient.class);
    addProgram(DriverKeysClient.class);
    addProgram(EchoClient.class);
    addProgram(ListParametersClient.class);
    addProgram(SetParameterClient.class);
  }

  private static class MainProgram extends Program {
    public MainProgram (String[] arguments) {
      super(arguments);
    }

    private Program programObject = null;

    @Override
    protected final void processParameters (String[] parameters) {
      int count = parameters.length;
      if (count == 0) syntaxError("missing program name");

      String name = parameters[0];
      Class<? extends Program> type = programs.get(name);
      if (type == null) syntaxError("unknown program: %s", name);

      count -= 1;
      String[] arguments = new String[count];
      System.arraycopy(parameters, 1, arguments, 0, count);

      try {
        Constructor constructor = type.getConstructor(arguments.getClass());
        programObject = (Program)constructor.newInstance((Object)arguments);
      } catch (NoSuchMethodException exception) {
        internalError("program constructor not found: %s", exception.getMessage());
      } catch (InstantiationException exception) {
        internalError("program instantiation failed: %s", exception.getMessage());
      } catch (IllegalAccessException exception) {
        internalError("program object access denied: %s", exception.getMessage());
      } catch (InvocationTargetException exception) {
        internalError("program construction failed: %s", exception.getCause().getMessage());
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
