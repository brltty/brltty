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

public abstract class ProgramComponent extends Component {
  protected ProgramComponent () {
    super();
  }

  public static String getProgramName (Class<? extends Program> type) {
    return type.getSimpleName();
  }

  public static boolean isClient (Class <? extends Program> type) {
    return Client.class.isAssignableFrom(type);
  }

  public static boolean isClient (Program program) {
    return isClient(program.getClass());
  }

  public final static int EXIT_CODE_SUCCESS  = 0;
  public final static int EXIT_CODE_SYNTAX   = 2;
  public final static int EXIT_CODE_SEMANTIC = 3;
  public final static int EXIT_CODE_EXTERNAL = 8;
  public final static int EXIT_CODE_INTERNAL = 9;
}
