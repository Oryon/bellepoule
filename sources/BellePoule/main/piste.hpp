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

class Module;

class Piste : public Object
{
  public:
    class Listener
    {
      public:
        virtual void OnPisteButtonEvent (Piste          *piste,
                                         GdkEventButton *event) = 0;
        virtual void OnPisteMotionEvent (Piste          *piste,
                                         GdkEventMotion *event) = 0;
    };

  public:
    Piste (GooCanvasItem *parent,
           Listener      *listener);

    void Select ();

    void UnSelect ();

    void SetColor (const gchar *color);

    void Translate (const gchar *color);

    void Translate (gdouble tx,
                    gdouble ty);

    void Rotate ();

    guint GetId ();

    void SetId (guint id);

    void AlignOnGrid ();

    void GetHorizontalCoord (gdouble *x,
                             gdouble *y);

  private:
    static const gdouble _W          = 160.0;
    static const gdouble _H          = 20.0;
    static const gdouble _BORDER_W   = 0.5;
    static const gdouble _RESOLUTION = 10.0;

    Piste::Listener *_listener;
    GooCanvasItem   *_root_item;
    GooCanvasItem   *_rect_item;
    GooCanvasItem   *_progress_item;
    GooCanvasItem   *_id_item;
    GooCanvasItem   *_match_item;
    GooCanvasItem   *_title_item;
    GooCanvasItem   *_status_item;
    guint            _id;
    gboolean         _horizontal;

    ~Piste ();

    void MonitorEvent (GooCanvasItem *item);

    gdouble GetGridAdjustment (gdouble coordinate);

    static gboolean OnButtonPress (GooCanvasItem  *item,
                                   GooCanvasItem  *target,
                                   GdkEventButton *event,
                                   Piste          *piste);

    static gboolean OnButtonRelease (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Piste          *piste);

    static gboolean OnMotionNotify (GooCanvasItem  *item,
                                    GooCanvasItem  *target,
                                    GdkEventMotion *event,
                                    Piste          *piste);
};

#endif
