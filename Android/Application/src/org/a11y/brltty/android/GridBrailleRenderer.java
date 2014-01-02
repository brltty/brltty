/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.ListIterator;
import java.util.ArrayList;

import android.graphics.Rect;
import android.graphics.Point;

import android.view.accessibility.AccessibilityNodeInfo;

public class GridBrailleRenderer extends BrailleRenderer {
  public class Grid {
    protected abstract class Coordinate implements Comparator<Cell> {
      private final int coordinateValue;

      private final List<Cell> cells = new ArrayList<Cell>();

      private int brailleOffset = 0;

      public abstract int compare (Cell cell1, Cell cell2);
      protected abstract void setPreviousCell (Cell cell, Cell previous);
      protected abstract void setNextCell (Cell cell, Cell Next);
      protected abstract Cell getAdjacentCell (Cell cell);
      protected abstract Coordinate getCoordinate (Cell cell);
      protected abstract int getSize (Cell cell);
      protected abstract int getSpacing ();

      public final int getValue () {
        return coordinateValue;
      }

      public List<Cell> getCells () {
        return cells;
      }

      public int getBrailleOffset () {
        return brailleOffset;
      }

      public void setBrailleOffset (int offset) {
        if (offset > brailleOffset) brailleOffset = offset;
      }

      public void addCell (Cell cell) {
        cells.add(cell);
      }

      public void linkCells () {
        Collections.sort(cells, this);
        Cell previous = null;

        for (Cell next : cells) {
          if (previous != null) {
            setPreviousCell(next, previous);
            setNextCell(previous, next);
          }

          previous = next;
        }
      }

      public void setOffsets () {
        for (Cell cell : cells) {
          Cell next = getAdjacentCell(cell);

          if (next != null) {
            getCoordinate(next).setBrailleOffset(getBrailleOffset() + getSize(cell) + getSpacing());
          }
        }
      }

      public Coordinate (int value) {
        coordinateValue = value;
      }
    }

    public class Column extends Coordinate {
      @Override
      public final int compare (Cell cell1, Cell cell2) {
        return LanguageUtilities.compare(
          cell1.getGridRow().getValue(),
          cell2.getGridRow().getValue()
        );
      }

      @Override
      protected final void setPreviousCell (Cell cell, Cell previous) {
        cell.setNorthCell(previous);
      }

      @Override
      protected final void setNextCell (Cell cell, Cell next) {
        cell.setSouthCell(next);
      }

      @Override
      protected final Cell getAdjacentCell (Cell cell) {
        return cell.getEastCell();
      }

      @Override
      protected final Coordinate getCoordinate (Cell cell) {
        return cell.getGridColumn();
      }

      @Override
      protected final int getSize (Cell cell) {
        return cell.getWidth();
      }

      @Override
      protected final int getSpacing () {
        return ApplicationParameters.BRAILLE_COLUMN_SPACING;
      }

      public Column (int value) {
        super(value);
      }
    }

    public class Row extends Coordinate {
      @Override
      public final int compare (Cell cell1, Cell cell2) {
        return LanguageUtilities.compare(
          cell1.getGridColumn().getValue(),
          cell2.getGridColumn().getValue()
        );
      }

      @Override
      protected final void setPreviousCell (Cell cell, Cell previous) {
        cell.setWestCell(previous);
      }

      @Override
      protected final void setNextCell (Cell cell, Cell next) {
        cell.setEastCell(next);
      }

      @Override
      protected final Cell getAdjacentCell (Cell cell) {
        return cell.getSouthCell();
      }

      @Override
      protected final Coordinate getCoordinate (Cell cell) {
        return cell.getGridRow();
      }

      @Override
      protected final int getSize (Cell cell) {
        return cell.getHeight();
      }

      @Override
      protected final int getSpacing () {
        return ApplicationParameters.BRAILLE_ROW_SPACING;
      }

      public Row (int value) {
        super(value);
      }
    }

    protected abstract class Coordinates {
      private final int coordinateSpacing;

      private final List<Coordinate> coordinates = new ArrayList<Coordinate>();

      protected abstract Coordinate newCoordinate (int value);

      public final List<Coordinate> getCoordinates () {
        return coordinates;
      }

      public final Coordinate getCoordinate (int value) {
        ListIterator<Coordinate> iterator = coordinates.listIterator();

        while (iterator.hasNext()) {
          Coordinate coordinate = iterator.next();
          int current = coordinate.getValue();
          if (current == value) return coordinate;

          if (current > value) {
            iterator.previous();
            break;
          }
        }

        {
          Coordinate coordinate = newCoordinate(value);
          iterator.add(coordinate);
          return coordinate;
        }
      }

      public void linkCells () {
        for (Coordinate coordinate : coordinates) {
          coordinate.linkCells();
        }
      }

      public void setOffsets () {
        int offset = 0;
        int increment = coordinateSpacing;
        if (increment == 0) increment = 1;

        for (Coordinate coordinate : coordinates) {
          coordinate.setBrailleOffset(offset);
          offset += increment;
          coordinate.setOffsets();
        }
      }

      public Coordinates (int spacing) {
        coordinateSpacing = spacing;
      }
    }

    public class Columns extends Coordinates {
      @Override
      protected final Coordinate newCoordinate (int value) {
        return new Column(value);
      }

      public Columns () {
        super(ApplicationParameters.BRAILLE_COLUMN_SPACING);
      }
    }

    public class Rows extends Coordinates {
      @Override
      protected final Coordinate newCoordinate (int value) {
        return new Row(value);
      }

      public Rows () {
        super(ApplicationParameters.BRAILLE_ROW_SPACING);
      }
    }

    public class Cell {
      private final Coordinate gridColumn;
      private final Coordinate gridRow;
      private final ScreenElement screenElement;

      private final int cellWidth;
      private final int cellHeight;

      private Cell northCell = null;
      private Cell eastCell = null;
      private Cell southCell = null;
      private Cell westCell = null;

      public final Coordinate getGridColumn () {
        return gridColumn;
      }

      public final Coordinate getGridRow () {
        return gridRow;
      }

      public final ScreenElement getScreenElement () {
        return screenElement;
      }

      public final int getWidth () {
        return cellWidth;
      }

      public final int getHeight () {
        return cellHeight;
      }

      public Cell getNorthCell () {
        return northCell;
      }

      public Cell getEastCell () {
        return eastCell;
      }

      public Cell getSouthCell () {
        return southCell;
      }

      public Cell getWestCell () {
        return westCell;
      }

      public void setNorthCell (Cell cell) {
        northCell = cell;
      }

      public void setEastCell (Cell cell) {
        eastCell = cell;
      }

      public void setSouthCell (Cell cell) {
        southCell = cell;
      }

      public void setWestCell (Cell cell) {
        westCell = cell;
      }

      public Cell (Coordinate column, Coordinate row, ScreenElement element) {
        gridColumn = column;
        gridRow = row;
        screenElement = element;

        String[] text = element.getBrailleText();
        cellWidth = getTextWidth(text);
        cellHeight = text.length;
      }
    }

    private final Coordinates columns = new Columns();
    private final Coordinates rows = new Rows();

    public final List<Coordinate> getColumns () {
      return columns.getCoordinates();
    }

    public final List<Coordinate> getRows () {
      return rows.getCoordinates();
    }

    private final Coordinate getColumn (int value) {
      return columns.getCoordinate(value);
    }

    private final Coordinate getRow (int value) {
      return rows.getCoordinate(value);
    }

    public Point getPoint (ScreenElement element) {
      AccessibilityNodeInfo node = element.getAccessibilityNode();
      if (node == null) return null;

      Rect location = new Rect();
      boolean leaf = node.getChildCount() == 0;

      do {
        node.getBoundsInScreen(location);

        AccessibilityNodeInfo parent = node.getParent();
        if (parent == null) break;

        node.recycle();
        node = parent;
      } while (node.getChildCount() == 1);

      node.recycle();
      node = null;

      int x = location.left;
      int y = leaf? ((location.top + location.bottom) / 2): location.top;
      return new Point(x, y);
    }

    public final Cell addCell (ScreenElement element) {
      String[] text = element.getBrailleText();
      if (text == null) return null;

      Point point = getPoint(element);
      if (point == null) return null;

      Coordinate column = getColumn(point.x);
      Coordinate row = getRow(point.y);
      Cell cell = new Cell(column, row, element);

      column.addCell(cell);
      row.addCell(cell);

      return cell;
    }

    public final void finish () {
      columns.linkCells();
      rows.linkCells();

      columns.setOffsets();
      rows.setOffsets();
    }

    public Grid () {
    }
  }

  @Override
  protected final void setBrailleLocations (ScreenElementList elements) {
    Grid grid = new Grid();

    for (ScreenElement element : elements) {
      grid.addCell(element);
    }
    grid.finish();

    for (Grid.Coordinate row : grid.getRows()) {
      int top = row.getBrailleOffset();

      for (Grid.Cell cell : row.getCells()) {
        Grid.Coordinate column = cell.getGridColumn();
        int left = column.getBrailleOffset();

        int right = left + cell.getWidth() - 1;
        int bottom = top + cell.getHeight() - 1;

        ScreenElement element = cell.getScreenElement();
        element.setBrailleLocation(left, top, right, bottom);
      }
    }
  }

  public GridBrailleRenderer () {
    super();
  }
}
