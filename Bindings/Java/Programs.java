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

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

public abstract class Programs extends ProgramHelper {
  private Programs () {
  }

  private final static KeywordMap<Class<? extends Program>> programs = new KeywordMap<>();
  static {
    programs.put("bound-commands", BoundCommandsProgram.class);
    programs.put("computer-braille", ComputerBrailleProgram.class);
    programs.put("driver-keys", DriverKeysProgram.class);
    programs.put("echo", EchoProgram.class);
    programs.put("list-parameters", ListParametersProgram.class);
    programs.put("set-parameter", SetParameterProgram.class);
  }

  public static void main (String arguments[]) {
    int count = arguments.length;
    if (count == 0) syntaxError("missing program name");

    String name = arguments[0];
    Class<? extends Program> type = programs.get(name);
    if (type == null) syntaxError("unknown program: %s", name);

    count -= 1;
    String[] operands = new String[count];
    System.arraycopy(arguments, 1, operands, 0, count);

    Program program = null;
    try {
      Constructor constructor = type.getConstructor(operands.getClass());
      program = (Program)constructor.newInstance((Object)operands);
    } catch (NoSuchMethodException exception) {
      internalError("program constructor not found: %s", exception.getMessage());
    } catch (InstantiationException exception) {
      internalError("program instantiation failed: %s", exception.getMessage());
    } catch (IllegalAccessException exception) {
      internalError("program object access denied: %s", exception.getMessage());
    } catch (InvocationTargetException exception) {
      internalError("program construction failed: %s", exception.getCause().getMessage());
    }

    program.run();
  }
}
