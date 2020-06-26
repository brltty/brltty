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
import org.a11y.brlapi.commands.*;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

public abstract class Commands extends CommandHelper {
  private Commands () {
  }

  private final static KeywordMap<Class<? extends Command>> commands = new KeywordMap<>();
  static {
    commands.put("bound-commands", BoundCommandsCommand.class);
    commands.put("defined-keys", DefinedKeysCommand.class);
    commands.put("list-parameters", ListParametersCommand.class);
    commands.put("set-parameter", SetParameterCommand.class);
  }

  public static void main (String arguments[]) {
    int count = arguments.length;
    if (count == 0) syntaxError("missing command name");

    String name = arguments[0];
    Class<? extends Command> type = commands.get(name);
    if (type == null) syntaxError("unknown command: %s", name);

    count -= 1;
    String[] operands = new String[count];
    System.arraycopy(arguments, 1, operands, 0, count);

    Command command = null;
    try {
      Constructor constructor = type.getConstructor(operands.getClass());
      command = (Command)constructor.newInstance((Object)operands);
    } catch (NoSuchMethodException exception) {
      internalError("command constructor not found: %s", exception.getMessage());
    } catch (InstantiationException exception) {
      internalError("command instantiation failed: %s", exception.getMessage());
    } catch (IllegalAccessException exception) {
      internalError("command object access denied: %s", exception.getMessage());
    } catch (InvocationTargetException exception) {
      internalError("command construction failed: %s", exception.getCause().getMessage());
    }

    command.run();
  }
}
