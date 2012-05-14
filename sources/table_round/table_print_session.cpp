// Copyright (C) 2009 Yannick Le Roux.
// This file is part of BellePoule.
//
//   BellePoule is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   BellePoule is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with BellePoule.  If not, see <http://www.gnu.org/licenses/>.

#include "table_print_session.hpp"

// --------------------------------------------------------------------------------
TablePrintSession::TablePrintSession ()
{
  _target_resolution = 1.0;
  _source_resolution = 1.0;
  SetScale (1.0);
}

// --------------------------------------------------------------------------------
TablePrintSession::~TablePrintSession ()
{
}

// --------------------------------------------------------------------------------
void TablePrintSession::SetResolutions (gdouble source_resolution,
                                        gdouble target_resolution)
{
  _source_resolution = source_resolution;
  _target_resolution = target_resolution;

  SetScale (_user_scale);
}

// --------------------------------------------------------------------------------
void TablePrintSession::SetScale (gdouble scale)
{
  gdouble dpi_adaptation = _target_resolution / _source_resolution;

  _user_scale   = scale;
  _global_scale = dpi_adaptation * _user_scale;
}

// --------------------------------------------------------------------------------
void TablePrintSession::SetPaperSize (gdouble paper_w,
                                      gdouble paper_h,
                                      gdouble header_h_on_paper)
{
  _page_w  = paper_w / _global_scale;
  _page_h  = paper_h / _global_scale;

  _header_h_on_canvas = header_h_on_paper / _global_scale;

  {
    _nb_x_pages = 1;
    if (_cutting_w > _page_w)
    {
      _nb_x_pages += (guint) (_cutting_w / _page_w);
    }

    _nb_y_pages = 1;
    if (_cutting_h > _page_h - _header_h_on_canvas)
    {
      _nb_y_pages += (guint) ((_cutting_h + _header_h_on_canvas) / _page_h);
    }
  }
}

// --------------------------------------------------------------------------------
gdouble TablePrintSession::GetGlobalScale ()
{
  return _global_scale;
}

// --------------------------------------------------------------------------------
gboolean TablePrintSession::CurrentPageHasHeader ()
{
  return _current_page_has_header;
}

// --------------------------------------------------------------------------------
GooCanvasBounds *TablePrintSession::GetCanvasBoundsForCurrentPage ()
{
  return &_canvas_bounds;
}

// --------------------------------------------------------------------------------
gdouble TablePrintSession::GetPaperXShiftForCurrentPage ()
{
  return -_canvas_bounds.x1;
}

// --------------------------------------------------------------------------------
gdouble TablePrintSession::GetPaperYShiftForCurrentPage ()
{
  gdouble shift_y = 0.0;

  shift_y -= _current_cutting * _cutting_h;
  shift_y += _header_h_on_canvas;
  shift_y -= _cutting_y_page * _page_h;

  return shift_y;
}

// --------------------------------------------------------------------------------
void TablePrintSession::ProcessCurrentPage (guint page)
{
  guint x_page = page % _nb_x_pages;
  guint y_page = page / _nb_x_pages;

  _current_cutting = y_page / _nb_y_pages;

  _cutting_x_page = x_page;
  _cutting_y_page = y_page - (_current_cutting * _nb_y_pages);

  _current_page_has_header = (_cutting_y_page % _nb_y_pages == 0);

  // Horizontal adjustement
  {
    _canvas_bounds.x1 = _page_w * (_cutting_x_page);
    _canvas_bounds.x2 = _page_w * (_cutting_x_page+1);
  }

  // Vertical adjustement
  {
    // Move to the current cutting area
    _canvas_bounds.y1 = _cutting_h * _current_cutting;

    // Move to the current position in the current cutting
    if (_cutting_h + _header_h_on_canvas <= _page_h)
    {
      _canvas_bounds.y2 = _canvas_bounds.y1 + _cutting_h;
    }
    else
    {
      _canvas_bounds.y1 += _page_h * _cutting_y_page;
      _canvas_bounds.y2  = _canvas_bounds.y1 + _page_h;

      if (_current_page_has_header)
      {
        _canvas_bounds.y2 -= _header_h_on_canvas;
      }
      else
      {
        _canvas_bounds.y1 -= _header_h_on_canvas;

        {
          gdouble total_page_h = (_cutting_y_page+1) * _page_h;

          if (total_page_h > _cutting_h)
          {
            _canvas_bounds.y2 -= total_page_h - _cutting_h;
          }
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
void TablePrintSession::Dump ()
{
}
