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

#include "piste.hpp"

#include "hall.hpp"

// --------------------------------------------------------------------------------
Hall::Hall ()
  : CanvasModule ("hall.glade")
{
  _piste_list = NULL;

  _new_x_location = 0.0;
  _new_y_location = 0.0;

  _dragging = FALSE;

  SetZoomer (GTK_RANGE (_glade->GetWidget ("zoom_scale")),
             2.0);
}

// --------------------------------------------------------------------------------
Hall::~Hall ()
{
  g_list_free_full (_piste_list,
                    (GDestroyNotify) Object::TryToRelease);
}

// --------------------------------------------------------------------------------
void Hall::OnPlugged ()
{
  CanvasModule::OnPlugged ();

  _root = goo_canvas_group_new (GetRootItem (),
                                NULL);

  goo_canvas_rect_new (_root,
                       0.0, 0.0, 300,  150,
                       "line-width",   2.0,
                       "stroke-color", "darkgrey",
                       NULL);

  g_signal_connect (GetRootItem (),
                    "button_press_event",
                    G_CALLBACK (OnSelected),
                    this);

  AddPiste ();
  RestoreZoomFactor ();
}

// --------------------------------------------------------------------------------
void Hall::AddPiste ()
{
  Piste *piste  = new Piste (_root, this);
  GList *before = _piste_list;

  for (guint i = 1; before != NULL; i++)
  {
    Piste *current_piste = (Piste *) before->data;

    if (current_piste->GetId () != i)
    {
      break;
    }
    piste->SetId (i+1);

    before = g_list_next (before);
  }

  if (before)
  {
    _piste_list = g_list_insert_before (_piste_list,
                                        before,
                                        piste);
  }
  else
  {
    _piste_list = g_list_append (_piste_list,
                                 piste);
  }

  piste->Translate (_new_x_location,
                    _new_y_location);

  _new_x_location += 5.0;
  _new_y_location += 5.0;
}

// --------------------------------------------------------------------------------
void Hall::RemoveSelected ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    RemovePiste (piste);

    current = g_list_next (current);
  }

  g_list_free (_selected_list);
  _selected_list = NULL;
}

// --------------------------------------------------------------------------------
void Hall::TranslateSelected (gdouble tx,
                              gdouble ty)
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    piste->Translate (tx,
                      ty);

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Hall::RotateSelected ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    piste->Rotate ();

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Hall::RemovePiste (Piste *piste)
{
  if (piste)
  {
    GList *current = _piste_list;

    while (current)
    {
      if (current->data == piste)
      {
        _piste_list = g_list_remove_link (_piste_list,
                                          current);
        piste->Release ();
        break;
      }

      current = g_list_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Hall::OnSelected (GooCanvasItem  *item,
                           GooCanvasItem  *target,
                           GdkEventButton *event,
                           Hall           *hall)
{
  hall->CancelSeletion ();

  return TRUE;
}

// --------------------------------------------------------------------------------
void Hall::CancelSeletion ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *current_piste = (Piste *) current->data;

    current_piste->UnSelect ();

    current = g_list_next (current);
  }

  g_list_free (_selected_list);
  _selected_list = NULL;
}

// --------------------------------------------------------------------------------
void Hall::OnPisteButtonEvent (Piste          *piste,
                               GdkEventButton *event)
{
  if (event->type == GDK_BUTTON_PRESS)
  {
    GList *selected_node = g_list_find (_selected_list, piste);

    if (selected_node)
    {
      if (event->state & GDK_CONTROL_MASK)
      {
        _selected_list = g_list_remove_link (_selected_list,
                                             selected_node);
        piste->UnSelect ();
      }
    }
    else
    {
      if ((event->state & GDK_CONTROL_MASK) == 0)
      {
        CancelSeletion ();
      }

      {
        _selected_list = g_list_prepend (_selected_list,
                                         piste);

        piste->Select ();
      }
    }

    {
      _dragging = TRUE;
      _drag_x = event->x;
      _drag_y = event->y;
    }
  }
  else
  {
    _dragging = FALSE;
  }
}

// --------------------------------------------------------------------------------
void Hall::OnPisteMotionEvent (Piste          *piste,
                               GdkEventMotion *event)
{
  if (_dragging && (event->state & GDK_BUTTON1_MASK))
  {
    double new_x = event->x;
    double new_y = event->y;

    TranslateSelected (new_x - _drag_x,
                       new_y - _drag_y);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_piste_button_clicked (GtkWidget *widget,
                                                             Object    *owner)
{
  Hall *h = dynamic_cast <Hall *> (owner);

  h->AddPiste ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_rotate_piste_button_clicked (GtkWidget *widget,
                                                                Object    *owner)
{
  Hall *h = dynamic_cast <Hall *> (owner);

  h->RotateSelected ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_piste_button_clicked (GtkWidget *widget,
                                                                Object    *owner)
{
  Hall *h = dynamic_cast <Hall *> (owner);

  h->RemoveSelected ();
}
