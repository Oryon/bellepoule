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

#pragma once

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

      void SetResolutions (gdouble source_resolution,
                           gdouble target_resolution);

      void SetPaperSize (gdouble paper_w,
                         gdouble paper_h,
                         gdouble header_h_on_paper);

      gdouble GetScale ();

      void SetPageBounds (guint            page,
                          GooCanvasBounds *bounds);

      void Begin (guint nb_pages);

    public:
      void ProcessCurrentPage (guint page);

      GooCanvasBounds *GetMiniHeaderBoundsForCurrentPage ();

      GooCanvasBounds *GetCanvasBoundsForCurrentPage ();

      gdouble GetPaperYShiftForCurrentPage ();

      guint GetNbPages ();

    public:
      gboolean _full_table;

    private:
      GooCanvasBounds *_bounds_table;

      guint    _nb_pages;

      gdouble  _dpi_adaptation;
      gdouble  _scale;
      gdouble  _source_resolution;
      gdouble  _target_resolution;

      gdouble  _page_w;
      gdouble  _page_h;

      gdouble  _cutting_w;
      gdouble  _cutting_h;

      GooCanvasBounds _mini_header_bounds;
      GooCanvasBounds _canvas_bounds;
      guint           _current_page;
  };
}
