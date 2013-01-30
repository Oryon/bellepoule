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

namespace Table
{
  // --------------------------------------------------------------------------------
  PrintSession::PrintSession ()
  {
    _bounds_table = NULL;

    _target_resolution = 1.0;
    _source_resolution = 1.0;
    SetScale (1.0);
  }

  // --------------------------------------------------------------------------------
  PrintSession::~PrintSession ()
  {
    g_free (_bounds_table);
  }

  // --------------------------------------------------------------------------------
  void PrintSession::Begin (guint cutting_count)
  {
    _cutting_count = cutting_count;

    g_free (_bounds_table);
    _bounds_table = g_new (GooCanvasBounds,
                           _cutting_count);
  }

  // --------------------------------------------------------------------------------
  void PrintSession::SetResolutions (gdouble source_resolution,
                                     gdouble target_resolution)
  {
    _source_resolution = source_resolution;
    _target_resolution = target_resolution;

    SetScale (_user_scale);
  }

  // --------------------------------------------------------------------------------
  void PrintSession::SetScale (gdouble scale)
  {
    gdouble dpi_adaptation = _target_resolution / _source_resolution;

    _user_scale   = scale;
    _global_scale = dpi_adaptation * _user_scale;
  }

  // --------------------------------------------------------------------------------
  void PrintSession::SetCuttingBounds (guint            cutting,
                                       GooCanvasBounds *bounds)
  {
    if (cutting < _cutting_count)
    {
      _bounds_table[cutting] = *bounds;
    }

    //if (cutting == 0)
    {
      _cutting_w = bounds->x2 - bounds->x1;
      _cutting_h = bounds->y2 - bounds->y1;
    }
  }

  // --------------------------------------------------------------------------------
  void PrintSession::SetPaperSize (gdouble paper_w,
                                   gdouble paper_h,
                                   gdouble header_h_on_paper)
  {
    gdouble mini_header_h = _bounds_table[0].y1;

    _page_w = paper_w / _global_scale;
    _page_h = paper_h / _global_scale;

    _header_h_on_canvas = header_h_on_paper / _global_scale;

    {
      _nb_x_pages = 1;
      if (_cutting_w > _page_w)
      {
        _nb_x_pages += (guint) (_cutting_w / _page_w);
      }

      _nb_y_pages = 1;
      if (_cutting_h > _page_h - _header_h_on_canvas - mini_header_h)
      {
        _nb_y_pages += (guint) ((_cutting_h + _header_h_on_canvas + mini_header_h) / _page_h);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gdouble PrintSession::GetGlobalScale ()
  {
    return _global_scale;
  }

  // --------------------------------------------------------------------------------
  gboolean PrintSession::CurrentPageHasHeader ()
  {
    return _current_page_has_header;
  }

  // --------------------------------------------------------------------------------
  GooCanvasBounds *PrintSession::GetMiniHeaderBoundsForCurrentPage ()
  {
    return &_mini_header_bounds;
  }

  // --------------------------------------------------------------------------------
  GooCanvasBounds *PrintSession::GetCanvasBoundsForCurrentPage ()
  {
    return &_canvas_bounds;
  }

  // --------------------------------------------------------------------------------
  gdouble PrintSession::GetPaperXShiftForCurrentPage ()
  {
    return -_canvas_bounds.x1;
  }

  // --------------------------------------------------------------------------------
  gdouble PrintSession::GetPaperYShiftForCurrentPage ()
  {
    gdouble shift_y = 0.0;

    shift_y -= _bounds_table[_current_cutting].y1;
    shift_y -= _cutting_y_page * _page_h;

    return shift_y;
  }

  // --------------------------------------------------------------------------------
  void PrintSession::ProcessCurrentPage (guint page)
  {
    guint x_page = page % _nb_x_pages;
    guint y_page = page / _nb_x_pages;

    _current_cutting = y_page / _nb_y_pages;

    _cutting_x_page = x_page;
    _cutting_y_page = y_page - (_current_cutting * _nb_y_pages);

    _current_page_has_header = (_cutting_y_page % _nb_y_pages == 0);

    // Mini header
    {
      _mini_header_bounds.x1 = x_page * _page_w;
      _mini_header_bounds.x2 = _mini_header_bounds.x1 + _page_w;
      _mini_header_bounds.y1 = 0.0;
      _mini_header_bounds.y2 = _bounds_table[0].y1;
    }

    // Horizontal adjustement
    {
      _canvas_bounds.x1 = _page_w * (_cutting_x_page);
      _canvas_bounds.x2 = _page_w * (_cutting_x_page+1);

      // Clipping
      {
        gdouble total_page_w = (_cutting_x_page+1) * _page_w;

        if (total_page_w > _cutting_w)
        {
          _canvas_bounds.x2      -= total_page_w - _cutting_w;
          _mini_header_bounds.x2 -= total_page_w - _cutting_w;
        }
      }
    }

    // Vertical adjustement
    {
      gdouble mini_header_h     = _mini_header_bounds.y2 - _mini_header_bounds.y1;
      gdouble current_cutting_h = _bounds_table[_current_cutting].y2 - _bounds_table[_current_cutting].y1;

      // Move to the current cutting area
      _canvas_bounds.y1 = _bounds_table[_current_cutting].y1;

      // Move to the current position in the current cutting
      if (current_cutting_h + _header_h_on_canvas + mini_header_h <= _page_h)
      {
        _canvas_bounds.y2 = _bounds_table[_current_cutting].y2;
      }
      else
      {
        _canvas_bounds.y1 += _page_h * _cutting_y_page;
        _canvas_bounds.y2  = _canvas_bounds.y1 + _page_h;

        if (_current_page_has_header)
        {
          _canvas_bounds.y2 -= (_header_h_on_canvas + mini_header_h);
        }
        else
        {
          _canvas_bounds.y1 -= (_header_h_on_canvas + mini_header_h);

          // Clipping
          {
            gdouble total_page_h = (_cutting_y_page+1) * _page_h;

            if (total_page_h > current_cutting_h)
            {
              _canvas_bounds.y2 -= total_page_h - current_cutting_h;
            }
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void PrintSession::Dump ()
  {
  }
}
