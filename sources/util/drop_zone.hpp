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

#ifndef drop_zone_hpp
#define drop_zone_hpp

#include <gtk/gtk.h>

#include "module.hpp"

#include "object.hpp"

class DropZone : public Object
{
  public:
    void Wipe ();

    void GetBounds (GooCanvasBounds *bounds,
                    gdouble          zoom_factor = 1.0);

    void Focus ();

    void Unfocus ();

    void Redraw (gdouble x,
                 gdouble y,
                 gdouble w,
                 gdouble h);

    virtual void AddObject (Object *object);

    virtual void RemoveObject (Object *object);

  public:
    virtual void Draw (GooCanvasItem *root_item);

  protected:
    Module        *_container;
    GooCanvasItem *_back_rect;

    DropZone (Module *container);

    virtual ~DropZone ();
};

#endif
