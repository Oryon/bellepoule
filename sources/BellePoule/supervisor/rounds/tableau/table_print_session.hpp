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

#ifndef table_print_session_hpp
#define table_print_session_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>

namespace Table
{
  class PrintSession
  {
    public:
      PrintSession ();

      virtual ~PrintSession ();

      void Dump ();

      void SetScale (gdouble scale);

      void SetResolutions (gdouble source_resolution,
                           gdouble target_resolution);

      void SetPaperSize (gdouble paper_w,
                         gdouble paper_h,
                         gdouble header_h_on_paper);

      gdouble GetGlobalScale ();

      void SetCuttingBounds (guint            cutting,
                             GooCanvasBounds *bounds);

      void Begin (guint cutting_count);

    public:
      void ProcessCurrentPage (guint page);

      gboolean CurrentPageHasHeader ();

      GooCanvasBounds *GetMiniHeaderBoundsForCurrentPage ();

      GooCanvasBounds *GetCanvasBoundsForCurrentPage ();

      gdouble GetPaperXShiftForCurrentPage ();

      gdouble GetPaperYShiftForCurrentPage ();

    public:
      gboolean _full_table;
      guint    _cutting_count;
      guint    _nb_x_pages;
      guint    _nb_y_pages;

    private:
      GooCanvasBounds *_bounds_table;

      gdouble  _user_scale;
      gdouble  _global_scale;
      gdouble  _source_resolution;
      gdouble  _target_resolution;

      gdouble  _header_h_on_canvas;
      gdouble  _page_w;
      gdouble  _page_h;

      gdouble  _cutting_w;
      gdouble  _cutting_h;

      gboolean        _current_page_has_header;
      GooCanvasBounds _mini_header_bounds;
      GooCanvasBounds _canvas_bounds;
      guint           _cutting_x_page;
      guint           _cutting_y_page;
      guint           _current_cutting;
  };
}

#endif
