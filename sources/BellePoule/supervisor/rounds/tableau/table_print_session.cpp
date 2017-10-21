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

    SetResolutions (1.0,
                    1.0);
  }

  // --------------------------------------------------------------------------------
  PrintSession::~PrintSession ()
  {
    g_free (_bounds_table);
  }

  // --------------------------------------------------------------------------------
  void PrintSession::Begin (guint nb_pages)
  {
    _cutting_w = 0.0;
    _cutting_h = 0.0;

    _nb_pages = nb_pages;

    g_free (_bounds_table);
    _bounds_table = g_new (GooCanvasBounds,
                           _nb_pages);
  }

  // --------------------------------------------------------------------------------
  guint PrintSession::GetNbPages ()
  {
    return _nb_pages;
  }

  // --------------------------------------------------------------------------------
  void PrintSession::SetResolutions (gdouble source_resolution,
                                     gdouble target_resolution)
  {
    _source_resolution = source_resolution;
    _target_resolution = target_resolution;

    _dpi_adaptation = _target_resolution / _source_resolution;
  }

  // --------------------------------------------------------------------------------
  void PrintSession::SetPageBounds (guint            page,
                                    GooCanvasBounds *bounds)
  {
    if (page < _nb_pages)
    {
      _bounds_table[page] = *bounds;

      {
        if (_cutting_w < bounds->x2 - bounds->x1)
        {
          _cutting_w = bounds->x2 - bounds->x1;
        }

        if (_cutting_h < bounds->y2 - bounds->y1)
        {
          _cutting_h = bounds->y2 - bounds->y1;
        }
      }
    }
    else
    {
      g_warning ("PrintSession::SetPageBounds ==> wrong page");
    }
  }

  // --------------------------------------------------------------------------------
  void PrintSession::SetPaperSize (gdouble paper_w,
                                   gdouble paper_h,
                                   gdouble header_h_on_paper)
  {
    gdouble mini_header_h = _bounds_table[0].y1;
    gdouble max_scale     = (3.0 * paper_h) / (100 * mini_header_h);
    gdouble x_scale       = paper_w*0.95 / _cutting_w;
    gdouble y_scale       = (paper_h - header_h_on_paper) / (mini_header_h + _cutting_h);

    _scale = MIN (x_scale, y_scale);
    _scale = MIN (_scale, max_scale);

    _page_w = paper_w / _scale;
    _page_h = paper_h / _scale;

    _mini_header_bounds.x1 = 0;
    _mini_header_bounds.x2 = _page_w;
    _mini_header_bounds.y1 = 0.0;
    _mini_header_bounds.y2 = _bounds_table[0].y1;
  }

  // --------------------------------------------------------------------------------
  gdouble PrintSession::GetScale ()
  {
    return _scale;
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
  gdouble PrintSession::GetPaperYShiftForCurrentPage ()
  {
    return -_bounds_table[_current_page].y1;
  }

  // --------------------------------------------------------------------------------
  void PrintSession::ProcessCurrentPage (guint page)
  {
    _current_page = page;

    // Canvas bounds adjustment
    {
      _canvas_bounds.x1 = 0;
      _canvas_bounds.x2 = _page_w;
      _canvas_bounds.y1 = _bounds_table[_current_page].y1;
      _canvas_bounds.y2 = _bounds_table[_current_page].y2;
    }
  }

  // --------------------------------------------------------------------------------
  void PrintSession::Dump ()
  {
  }
}
