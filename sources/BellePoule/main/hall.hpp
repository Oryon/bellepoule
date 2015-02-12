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

#ifndef hall_hpp
#define hall_hpp

#include "util/canvas_module.hpp"
#include "piste.hpp"

class Hall : public CanvasModule, Piste::Listener
{
  public:
    Hall ();

    void AddPiste ();

    void RemovePiste (Piste *piste);

    void TranslateSelected (gdouble tx,
                            gdouble ty);

    void RotateSelected ();

    void RemoveSelected ();

    void AlignSelectedOnGrid ();

  private:
    GooCanvasItem *_root;
    GList         *_piste_list;
    GList         *_selected_list;
    gdouble        _new_x_location;
    gdouble        _new_y_location;
    gboolean       _dragging;
    gdouble        _drag_x;
    gdouble        _drag_y;

    ~Hall ();

    void OnPlugged ();

    void CancelSeletion ();

    static gboolean OnSelected (GooCanvasItem  *goo_rect,
                                GooCanvasItem  *target,
                                GdkEventButton *event,
                                Hall           *hall);

    void OnPisteButtonEvent (Piste          *piste,
                             GdkEventButton *event);

    void OnPisteMotionEvent (Piste          *piste,
                             GdkEventMotion *event);
};

#endif
