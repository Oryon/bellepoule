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

  goo_canvas_set_scale (GetCanvas (),
                        3.0);

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
}

// --------------------------------------------------------------------------------
void Hall::AddPiste ()
{
  Piste *piste = new Piste (_root,
                            g_list_length (_piste_list));

  _piste_list = g_list_prepend (_piste_list,
                                piste);

  piste->Translate (_new_x_location,
                    _new_y_location);

  _new_x_location += 5.0;
  _new_y_location += 5.0;
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
  goo_canvas_grab_focus (goo_canvas_item_get_canvas (item),
                         item);
  return TRUE;
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
  Piste *piste = Piste::GetSelected ();

  if (piste)
  {
    piste->Rotate ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_piste_button_clicked (GtkWidget *widget,
                                                                Object    *owner)
{
  Hall *h = dynamic_cast <Hall *> (owner);

  h->RemovePiste (Piste::GetSelected ());
}
