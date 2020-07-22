/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
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

import java.util.Map;
import java.util.HashMap;

import android.view.accessibility.AccessibilityNodeInfo;
import android.view.KeyEvent;

import android.os.Bundle;
import android.util.Log;

public class Motion {
  private final static String LOG_TAG = Motion.class.getName();

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

  private final static Map<Character, Motion> motions =
               new HashMap<Character, Motion>();

  public static Motion get (char character) {
    return motions.get(character);
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
          .append("Dir:").append(motionDirection.name().toLowerCase())
          .append(" Type:").append(motionType.getTypeName().toLowerCase())
          .append(" Act:").append(actionIdentifier)
          .append(" Args:").append(actionArguments.toString())
          .append('}')
          .toString();
  }

  public static String toString (Motion motion) {
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

  private Motion (Type type, Direction direction) {
    motionType = type;
    motionDirection = direction;

    actionIdentifier = direction.getAction(type);
    actionArguments = new Bundle();
    type.setTypeArgument(actionArguments);
  }

  private static void addMotion (char character, Type type, Direction direction) {
    motions.put(character, new Motion(type, direction));
  }

  private static void addMotion (char previous, char next, Type type) {
    addMotion(previous, type, Direction.PREVIOUS);
    addMotion(next, type, Direction.NEXT);
  }

  private static void addMotion (char letter, Type type, Integer key) {
    addMotion(Character.toUpperCase(letter), Character.toLowerCase(letter), type);
  }

  private static void addMotion (char letter, Type type) {
    addMotion(letter, type, KeyEvent.KEYCODE_UNKNOWN);
  }

  static {
    addMotion('[', ']', Text.PARAGRAPH);
    addMotion('{', '}', Text.PAGE);

    addMotion('<', '>', Element.MAIN);
    addMotion('(', ')', Element.FRAME);

    addMotion('a', Element.ARTICLE);
    addMotion('b', Element.BUTTON, KeyEvent.KEYCODE_B);
    addMotion('c', Element.CONTROL, KeyEvent.KEYCODE_C);
    addMotion('d', Element.LANDMARK, KeyEvent.KEYCODE_D);
    addMotion('e', Element.EDITABLE_TEXT, KeyEvent.KEYCODE_E);
    addMotion('f', Element.FOCUSABLE, KeyEvent.KEYCODE_F);
    addMotion('g', Element.GRAPHIC, KeyEvent.KEYCODE_G);
    addMotion('h', Element.HEADING, KeyEvent.KEYCODE_H);
    addMotion('i', Element.LIST_ITEM, KeyEvent.KEYCODE_I);
    addMotion('l', Element.LINK, KeyEvent.KEYCODE_L);
    addMotion('m', Element.MEDIA);
    addMotion('o', Element.LIST, KeyEvent.KEYCODE_O);
    addMotion('r', Element.RADIO_BUTTON);
    addMotion('s', Element.SECTION);
    addMotion('t', Element.TABLE, KeyEvent.KEYCODE_T);
    addMotion('u', Element.UNVISITED_LINK);
    addMotion('v', Element.VISITED_LINK);
    addMotion('x', Element.CHECKBOX, KeyEvent.KEYCODE_X);
    addMotion('z', Element.COMBOBOX, KeyEvent.KEYCODE_Z);
  }
}
