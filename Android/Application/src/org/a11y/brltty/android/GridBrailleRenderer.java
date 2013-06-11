/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

public class GridBrailleRenderer extends BrailleRenderer {
  public class Grid {
    public abstract class Coordinate implements Comparator<Cell> {
      private final int coordinateValue;

      private final List<Cell> cells = new ArrayList<Cell>();

      private int cellOffset = 0;
      private int cellSize = 1;

      public abstract int compare (Cell cell1, Cell cell2);
      protected abstract int getSize (Cell cell);

      public final int getValue () {
        return coordinateValue;
      }

      public List<Cell> getCells () {
        return cells;
      }

      public int getOffset () {
        return cellOffset;
      }

      public int getSize () {
        return cellSize;
      }

      public void addCell (Cell cell) {
        cells.add(cell);
      }

      public void finish (int offset) {
        Collections.sort(cells, this);
        cellOffset = offset;

        for (Cell cell : cells) {
          int size = getSize(cell);
          if (size > cellSize) cellSize = size;
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
      protected final int getSize (Cell cell) {
        return cell.getWidth();
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
      protected final int getSize (Cell cell) {
        return cell.getHeight();
      }

      public Row (int value) {
        super(value);
      }
    }

    public abstract class Coordinates {
      private final List<Coordinate> coordinates = new ArrayList<Coordinate>();

      protected abstract Coordinate newCoordinate (int value);
      protected abstract int getSpacing ();

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

      public void finish () {
        int offset = 0;

        for (Coordinate coordinate : coordinates) {
          coordinate.finish(offset);
          offset += coordinate.getSize() + getSpacing();
        }
      }

      public Coordinates () {
      }
    }

    public class Columns extends Coordinates {
      @Override
      protected final Coordinate newCoordinate (int value) {
        return new Column(value);
      }

      @Override
      protected final int getSpacing () {
        return ApplicationParameters.COLUMN_SPACING;
      }

      public Columns () {
        super();
      }
    }

    public class Rows extends Coordinates {
      @Override
      protected final Coordinate newCoordinate (int value) {
        return new Row(value);
      }

      @Override
      protected final int getSpacing () {
        return 0;
      }

      public Rows () {
        super();
      }
    }

    public class Cell {
      private final Coordinate gridColumn;
      private final Coordinate gridRow;

      private final ScreenElement screenElement;
      private final int cellWidth;
      private final int cellHeight;

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

    public final Cell addCell (ScreenElement element) {
      String[] text = element.getBrailleText();
      if (text == null) return null;

      Rect location = element.getVisualLocation();
      if (location == null) return null;

      Coordinate column = getColumn(location.left);
      Coordinate row = getRow((location.top + location.bottom) / 2);
      Cell cell = new Cell(column, row, element);

      column.addCell(cell);
      row.addCell(cell);

      return cell;
    }

    public final void finish () {
      columns.finish();
      rows.finish();
    }

    public Grid () {
    }
  }

  @Override
  public void renderScreenElements (List<CharSequence> rows, ScreenElementList elements) {
    Grid grid = new Grid();

    for (ScreenElement element : elements) {
      grid.addCell(element);
    }
    grid.finish();

    for (Grid.Coordinate row : grid.getRows()) {
      int top = row.getOffset();
      int bottom = top + row.getSize() - 1;

      for (Grid.Cell cell : row.getCells()) {
        Grid.Coordinate column = cell.getGridColumn();
        int left = column.getOffset();
        int right = left + column.getSize() - 1;

        ScreenElement element = cell.getScreenElement();
        String[] text = element.getBrailleText();

        int y = top;

        for (String line : text) {
          while (rows.size() <= y) rows.add("");

          StringBuilder sb = new StringBuilder(rows.get(y));
          while (sb.length() < left) sb.append(' ');
          sb.append(line);
          rows.set(y, sb.toString());

          element.setBrailleLocation(new Rect(left, top, right, bottom));
          y += 1;
        }
      }
    }
  }

  public GridBrailleRenderer () {
    super();
  }
}
