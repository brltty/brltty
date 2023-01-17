/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

package org.a11y.brltty.android;
import org.a11y.brltty.core.Braille;

import java.util.Locale;
import java.util.Map;
import java.util.HashMap;

import android.view.accessibility.AccessibilityNodeInfo;
import android.view.KeyEvent;

import android.os.Bundle;
import android.util.Log;

public class StructuralMotion {
  private final static String LOG_TAG = StructuralMotion.class.getName();

  public abstract static class Type {
    private final String typeName;

    public final String getTypeName () {
      return typeName;
    }

    protected Type (String name) {
      typeName = name;
    }

    protected abstract void setTypeArgument (Bundle arguments);
    protected abstract int getActionForNext ();
    protected abstract int getActionForPrevious ();
  }

  public static class Text extends Type {
    private final int movementGranularity;

    public final int getMovementGranularity () {
      return movementGranularity;
    }

    @Override
    protected final void setTypeArgument (Bundle arguments) {
      arguments.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_MOVEMENT_GRANULARITY_INT, movementGranularity);
    }

    @Override
    protected final int getActionForNext () {
      return AccessibilityNodeInfo.ACTION_NEXT_AT_MOVEMENT_GRANULARITY;
    }

    @Override
    protected final int getActionForPrevious () {
      return AccessibilityNodeInfo.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY;
    }

    private Text (int granularity, String name) {
      super(name);
      movementGranularity = granularity;
    }

    public final static Type CHARACTER = new Text(
      AccessibilityNodeInfo.MOVEMENT_GRANULARITY_CHARACTER, "CHARACTER"
    );

    public final static Type WORD = new Text(
      AccessibilityNodeInfo.MOVEMENT_GRANULARITY_WORD, "WORD"
    );

    public final static Type LINE = new Text(
      AccessibilityNodeInfo.MOVEMENT_GRANULARITY_LINE, "LINE"
    );

    public final static Type PARAGRAPH = new Text(
      AccessibilityNodeInfo.MOVEMENT_GRANULARITY_PARAGRAPH, "PARAGRAPH"
    );

    public final static Type PAGE = new Text(
      AccessibilityNodeInfo.MOVEMENT_GRANULARITY_PAGE, "PAGE"
    );
  }

  public static class Element extends Type {
    private final String roleName;

    public final String getRoleName () {
      return roleName;
    }

    @Override
    protected final void setTypeArgument (Bundle arguments) {
      arguments.putString(AccessibilityNodeInfo.ACTION_ARGUMENT_HTML_ELEMENT_STRING, roleName);
    }

    @Override
    protected final int getActionForNext () {
      return AccessibilityNodeInfo.ACTION_NEXT_HTML_ELEMENT;
    }

    @Override
    protected final int getActionForPrevious () {
      return AccessibilityNodeInfo.ACTION_PREVIOUS_HTML_ELEMENT;
    }

    private Element (String role, String name) {
      super(name);
      roleName = role;
    }

    private Element (String role) {
      this(role, role);
    }

    public final static Type MAIN = new Element("MAIN");
    public final static Type FOCUSABLE = new Element("FOCUSABLE");
    public final static Type CONTROL = new Element("CONTROL");
    public final static Type LANDMARK = new Element("LANDMARK");

    public final static Type ARTICLE = new Element("ARTICLE");
    public final static Type FRAME = new Element("FRAME");
    public final static Type SECTION = new Element("SECTION");

    public final static Type GRAPHIC = new Element("GRAPHIC");
    public final static Type MEDIA = new Element("MEDIA");

    public final static Type TABLE = new Element("TABLE");
    public final static Type LIST = new Element("LIST");
    public final static Type LIST_ITEM = new Element("LIST_ITEM");

    public final static Type HEADING = new Element("HEADING");
    public final static Type HEADING_1 = new Element("H1");
    public final static Type HEADING_2 = new Element("H2");
    public final static Type HEADING_3 = new Element("H3");
    public final static Type HEADING_4 = new Element("H4");
    public final static Type HEADING_5 = new Element("H5");
    public final static Type HEADING_6 = new Element("H6");

    public final static Type LINK = new Element("LINK");
    public final static Type VISITED_LINK = new Element("VISITED_LINK");
    public final static Type UNVISITED_LINK = new Element("UNVISITED_LINK");

    public final static Type EDITABLE_TEXT = new Element("TEXT_FIELD", "EDITABLE_TEXT");
    public final static Type CHECKBOX = new Element("CHECKBOX");
    public final static Type COMBOBOX = new Element("COMBOBOX");
    public final static Type BUTTON = new Element("BUTTON");
    public final static Type RADIO_BUTTON = new Element("RADIO", "RADIO_BUTTON");
  }

  public enum Direction {
    NEXT(
      new ActionGetter() {
        @Override
        public int getAction (Type type) {
          return type.getActionForNext();
        }
      }
    ),

    PREVIOUS(
      new ActionGetter() {
        @Override
        public int getAction (Type type) {
          return type.getActionForPrevious();
        }
      }
    ),

    ; // end of enumeration

    public interface ActionGetter {
      public int getAction (Type type);
    }

    public final ActionGetter actionGetter;

    Direction (ActionGetter getter) {
      actionGetter = getter;
    }

    public final int getAction (Type type) {
      return actionGetter.getAction(type);
    }
  }

  private final static Map<Character, StructuralMotion> structuralMotions =
               new HashMap<Character, StructuralMotion>();

  public static StructuralMotion get (char character) {
    return structuralMotions.get(character);
  }

  private final Type motionType;
  private final Direction motionDirection;

  public final Type getType () {
    return motionType;
  }

  public final Direction getDirection () {
    return motionDirection;
  }

  private final int actionIdentifier;
  private final Bundle actionArguments;

  @Override
  public String toString () {
    return new StringBuilder()
          .append(getClass().getSimpleName()).append('{')
          .append("Dir:").append(motionDirection.name().toLowerCase(Locale.ROOT))
          .append(" Type:").append(motionType.getTypeName().toLowerCase(Locale.ROOT))
          .append(" Act:").append(actionIdentifier)
          .append(" Args:").append(actionArguments.toString())
          .append('}')
          .toString();
  }

  public static String toString (StructuralMotion motion) {
    return motion.toString();
  }

  public final boolean apply (AccessibilityNodeInfo node) {
    if (ApplicationParameters.ENABLE_MOTION_LOGGING) {
      Log.d(LOG_TAG,
        String.format(
          "applying: %s From:%s Afd:%b", toString(),
          ScreenUtilities.getText(node), node.isAccessibilityFocused()
        )
      );
    }

    return ScreenUtilities.performAction(node, actionIdentifier, actionArguments);
  }

  private StructuralMotion (Type type, Direction direction) {
    motionType = type;
    motionDirection = direction;

    actionIdentifier = direction.getAction(type);
    actionArguments = new Bundle();
    type.setTypeArgument(actionArguments);
  }

  private static void addMotion (char character, Type type, Direction direction) {
    structuralMotions.put(character, new StructuralMotion(type, direction));
  }

  private static void addMotion (char previous, char next, Type type) {
    addMotion(previous, type, Direction.PREVIOUS);
    addMotion(next, type, Direction.NEXT);
  }

  public final static char DOT_PREVIOUS   = Braille.DOT7;
  public final static char DOT_NEXT       = Braille.DOT8;
  public final static char DOTS_DIRECTION = DOT_PREVIOUS | DOT_NEXT;

  private static void addMotion (char character, Type type, Integer key) {
    character |= Braille.ROW;

    char previous = character;
    previous |= DOT_PREVIOUS;

    char next = character;
    next |= DOT_NEXT;

    addMotion(previous, next, type);
  }

  private static void addMotion (char character, Type type) {
    addMotion(character, type, KeyEvent.KEYCODE_UNKNOWN);
  }

  static {
    addMotion('[', ']', Text.PARAGRAPH);
    addMotion('{', '}', Text.PAGE);

    addMotion('<', '>', Element.MAIN);
    addMotion('(', ')', Element.FRAME);

    addMotion(Braille.DOTS_A, Element.ARTICLE);
    addMotion(Braille.DOTS_B, Element.BUTTON, KeyEvent.KEYCODE_B);
    addMotion(Braille.DOTS_C, Element.CONTROL, KeyEvent.KEYCODE_C);
    addMotion(Braille.DOTS_D, Element.LANDMARK, KeyEvent.KEYCODE_D);
    addMotion(Braille.DOTS_E, Element.EDITABLE_TEXT, KeyEvent.KEYCODE_E);
    addMotion(Braille.DOTS_F, Element.FOCUSABLE, KeyEvent.KEYCODE_F);
    addMotion(Braille.DOTS_G, Element.GRAPHIC, KeyEvent.KEYCODE_G);
    addMotion(Braille.DOTS_H, Element.HEADING, KeyEvent.KEYCODE_H);
    addMotion(Braille.DOTS_I, Element.LIST_ITEM, KeyEvent.KEYCODE_I);
    addMotion(Braille.DOTS_L, Element.LINK, KeyEvent.KEYCODE_L);
    addMotion(Braille.DOTS_M, Element.MEDIA);
    addMotion(Braille.DOTS_O, Element.LIST, KeyEvent.KEYCODE_O);
    addMotion(Braille.DOTS_R, Element.RADIO_BUTTON);
    addMotion(Braille.DOTS_S, Element.SECTION);
    addMotion(Braille.DOTS_T, Element.TABLE, KeyEvent.KEYCODE_T);
    addMotion(Braille.DOTS_U, Element.UNVISITED_LINK);
    addMotion(Braille.DOTS_V, Element.VISITED_LINK);
    addMotion(Braille.DOTS_X, Element.CHECKBOX, KeyEvent.KEYCODE_X);
    addMotion(Braille.DOTS_Z, Element.COMBOBOX, KeyEvent.KEYCODE_Z);

    addMotion(Braille.DOTS_1, Element.HEADING_1, KeyEvent.KEYCODE_1);
    addMotion(Braille.DOTS_2, Element.HEADING_2, KeyEvent.KEYCODE_2);
    addMotion(Braille.DOTS_3, Element.HEADING_3, KeyEvent.KEYCODE_3);
    addMotion(Braille.DOTS_4, Element.HEADING_4, KeyEvent.KEYCODE_4);
    addMotion(Braille.DOTS_5, Element.HEADING_5, KeyEvent.KEYCODE_5);
    addMotion(Braille.DOTS_6, Element.HEADING_6, KeyEvent.KEYCODE_6);
  }
}
