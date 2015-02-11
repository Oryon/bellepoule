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

#ifndef piste_hpp
#define piste_hpp

#include <goocanvas.h>

#include "util/object.hpp"

class Piste : public Object
{
  public:
    Piste (GooCanvasItem *parent,
           guint          id);

    void Translate (gdouble tx,
                    gdouble ty);

  private:
    GooCanvasItem *_root;
    GooCanvasItem *_rect;
    guint          _id;

    ~Piste ();

    static gboolean OnPisteSelected (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Piste          *piste);

    static gboolean OnFocusIn (GooCanvasItem *goo_rect,
                               GooCanvasItem *target,
                               GdkEventFocus *event,
                               Piste          *piste);

    static gboolean OnFocusOut (GooCanvasItem *goo_rect,
                                GooCanvasItem *target,
                                GdkEventFocus *event,
                                Piste          *piste);
};

#endif
