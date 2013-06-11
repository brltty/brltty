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
    public class Coordinate {
      private final int coordinateValue;

      private final List<Cell> cells = new ArrayList<Cell>();

      public final int getValue () {
        return coordinateValue;
      }

      public void addCell (Cell cell) {
        cells.add(cell);
      }

      public void sortCells (Comparator<Cell> comparator) {
        Collections.sort(cells, comparator);
      }

      public Coordinate (int value) {
        coordinateValue = value;
      }
    }

    public class Cell {
      private final Coordinate cellColumn;
      private final Coordinate cellRow;

      private final String[] cellText;
      private final int cellWidth;
      private final int cellHeight;

      public final Coordinate getColumn () {
        return cellColumn;
      }

      public final Coordinate getRow () {
        return cellRow;
      }

      public final String[] getText () {
        return cellText;
      }

      public final int getWidth () {
        return cellWidth;
      }

      public final int getHeight () {
        return cellHeight;
      }

      public Cell (Coordinate column, Coordinate row, String[] text) {
        cellColumn = column;
        cellRow = row;

        cellText = text;
        cellWidth = getTextWidth(cellText);
        cellHeight = cellText.length;
      }
    }

    private final List<Coordinate> columns = new ArrayList<Coordinate>();
    private final List<Coordinate> rows = new ArrayList<Coordinate>();

    private final Coordinate getCoordinate (List<Coordinate> coordinates, int value) {
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
        Coordinate coordinate = new Coordinate(value);
        iterator.add(coordinate);
        return coordinate;
      }
    }

    private final Coordinate getColumn (int value) {
      return getCoordinate(columns, value);
    }

    private final Coordinate getRow (int value) {
      return getCoordinate(rows, value);
    }

    public final Cell addCell (ScreenElement element) {
      String[] text = element.getBrailleText();
      if (text == null) return null;

      Rect location = element.getVisualLocation();
      if (location == null) return null;

      Coordinate column = getColumn(location.left);
      Coordinate row = getRow(location.top);
      Cell cell = new Cell(column, row, text);

      column.addCell(cell);
      row.addCell(cell);

      return cell;
    }

    private void sortCoordinatesByCell (List<Coordinate> coordinates, Comparator<Cell> comparator) {
      for (Coordinate coordinate : coordinates) {
        coordinate.sortCells(comparator);
      }
    }

    private void sortColumnsByRow () {
      Comparator<Cell> comparator = new Comparator<Cell>() {
        @Override
        public int compare (Cell cell1, Cell cell2) {
          return LanguageUtilities.compare(
            cell1.getRow().getValue(),
            cell2.getRow().getValue()
          );
        }
      };

      sortCoordinatesByCell(columns, comparator);
    }

    private void sortRowsByColumn () {
      Comparator<Cell> comparator = new Comparator<Cell>() {
        @Override
        public int compare (Cell cell1, Cell cell2) {
          return LanguageUtilities.compare(
            cell1.getColumn().getValue(),
            cell2.getColumn().getValue()
          );
        }
      };

      sortCoordinatesByCell(rows, comparator);
    }

    public final void finish () {
      sortColumnsByRow();
      sortRowsByColumn();
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
  }

  public GridBrailleRenderer () {
    super();
  }
}
