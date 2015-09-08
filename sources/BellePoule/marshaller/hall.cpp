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
#include "batch.hpp"
#include "job.hpp"

#include "hall.hpp"

// --------------------------------------------------------------------------------
Hall::Hall ()
  : CanvasModule ("hall.glade")
{
  _piste_list    = NULL;
  _selected_list = NULL;

  _dragging = FALSE;

  SetZoomer (GTK_RANGE (_glade->GetWidget ("zoom_scale")),
             2.0);

  _batch_list = NULL;
}

// --------------------------------------------------------------------------------
Hall::~Hall ()
{
  g_list_free_full (_piste_list,
                    (GDestroyNotify) Object::TryToRelease);

  g_list_free_full (_batch_list,
                    (GDestroyNotify) Object::TryToRelease);
}

// --------------------------------------------------------------------------------
void Hall::OnPlugged ()
{
  CanvasModule::OnPlugged ();

  _root = goo_canvas_group_new (GetRootItem (),
                                NULL);

  g_signal_connect (_root,
                    "motion_notify_event",
                    G_CALLBACK (OnMotionNotify),
                    this);

#if 1
  {
    GooCanvasItem *rect = goo_canvas_rect_new (_root,
                                               0.0, 0.0, 300, 150,
                                               "line-width", 0.0,
                                               "fill-color", "White",
                                               NULL);
    g_signal_connect (rect,
                      "motion_notify_event",
                      G_CALLBACK (OnMotionNotify),
                      this);
  }
#endif

  g_signal_connect (GetRootItem (),
                    "button_press_event",
                    G_CALLBACK (OnSelected),
                    this);


  {
    _dnd_config->AddTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);
    _dnd_config->AddTarget ("bellepoule/job",     GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

    _dnd_config->SetOnAWidgetDest (GTK_WIDGET (GetCanvas ()),
                                   GDK_ACTION_COPY);

    ConnectDndDest (GTK_WIDGET (GetCanvas ()));
    EnableDndOnCanvas ();
  }

  AddPiste ();
  RestoreZoomFactor ();
}

// --------------------------------------------------------------------------------
Object *Hall::GetDropObjectFromRef (guint32 ref,
                                    guint   key)
{
  if (strcmp (g_quark_to_string (key), "bellepoule/referee") == 0)
  {
    printf ("====> %d\n", ref);
    return this;
  }
  else if (strcmp (g_quark_to_string (key), "bellepoule/job") == 0)
  {
    printf ("====> %d\n", ref);
    return GetBatch (ref);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Hall::DropObject (Object   *object,
                       DropZone *source_zone,
                       DropZone *target_zone)
{
  Piste *piste = dynamic_cast <Piste *> (target_zone);

  {
    Batch *batch = dynamic_cast <Batch *> (object);

    if (batch)
    {
      GSList *selection = batch->GetCurrentSelection ();
      GSList *current   = selection;

      while (current)
      {
        Job *job = (Job *) current->data;

        piste->AddJob (job);

        current = g_slist_next (current);
      }

      g_slist_free_full (selection,
                         (GDestroyNotify) g_free);
      return;
    }
  }

  {
    piste->AddReferee (NULL);
  }
}

// --------------------------------------------------------------------------------
void Hall::ManageContest (Net::Message *message,
                          GtkNotebook  *notebook)
{
  gchar *id = message->GetString ("id");

  if (id)
  {
    Batch *batch = GetBatch (id);

    if (batch == NULL)
    {
      batch = new Batch (id);

      _batch_list = g_list_prepend (_batch_list,
                                    batch);

      batch->AttachTo (notebook);
    }

    batch->SetProperties (message);

    g_free (id);
  }
}

// --------------------------------------------------------------------------------
void Hall::DropContest (Net::Message *message)
{
  gchar *id = message->GetString ("id");

  if (id)
  {
    Batch *batch = GetBatch (id);

    if (batch)
    {
      GList *node = g_list_find (_batch_list,
                                 batch);

      _batch_list = g_list_delete_link (_batch_list,
                                        node);

      batch->Release ();
    }

    g_free (id);
  }
}

// --------------------------------------------------------------------------------
void Hall::ManageJob (Net::Message *message)
{
  gchar *id    = message->GetString ("contest");
  Batch *batch = GetBatch (id);

  if (batch)
  {
    batch->LoadJob (message);
  }

  g_free (id);
}

// --------------------------------------------------------------------------------
void Hall::AddPiste ()
{
  GList *anchor = g_list_last (_piste_list);
  Piste *piste  = new Piste (_root, this);

  piste->SetListener (this);

  {
    _drop_zones = g_slist_append (_drop_zones,
                                  piste);

    piste->Draw (GetRootItem ());
  }

  {
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
  }

  if (anchor)
  {
    piste->AnchorTo ((Piste *) anchor->data);
  }
  else
  {
    piste->Translate (0.5,
                      0.5);
  }
}

// --------------------------------------------------------------------------------
Batch *Hall::GetBatch (const gchar *id)
{
  guint32 dnd_id = (guint32) g_ascii_strtoull (id,
                                               NULL,
                                               16);
  return GetBatch (dnd_id);
}

// --------------------------------------------------------------------------------
Batch *Hall::GetBatch (guint32 id)
{
  GList *current = _batch_list;

  while (current)
  {
    Batch *batch = (Batch *) current->data;

    if (batch->GetId () == id)
    {
      return batch;
    }

    current = g_list_next (current);
  }

  return NULL;
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
void Hall::AlignSelectedOnGrid ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    piste->AlignOnGrid ();

    current = g_list_next (current);
  }
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
        SetCursor (GDK_FLEUR);
      }
    }

    {
      _dragging = TRUE;
      _drag_x = event->x;
      _drag_y = event->y;

      piste->GetHorizontalCoord (&_drag_x,
                                 &_drag_y);
    }
  }
  else
  {
    _dragging = FALSE;

    AlignSelectedOnGrid ();
  }
}

// --------------------------------------------------------------------------------
void Hall::OnPisteMotionEvent (Piste          *piste,
                               GdkEventMotion *event)
{
  SetCursor (GDK_FLEUR);

  if (_dragging)
  {
    if (   (event->state & GDK_BUTTON1_MASK)
        && ((event->state & GDK_CONTROL_MASK) == 0))
    {
      double new_x = event->x;
      double new_y = event->y;

      piste->GetHorizontalCoord (&new_x,
                                 &new_y);

      TranslateSelected (new_x - _drag_x,
                         new_y - _drag_y);
    }
  }
  else if (g_list_find (_selected_list, piste) == NULL)
  {
    ResetCursor ();
  }
}

// --------------------------------------------------------------------------------
gboolean Hall::OnMotionNotify (GooCanvasItem  *item,
                               GooCanvasItem  *target,
                               GdkEventMotion *event,
                               Hall           *hall)
{
  hall->ResetCursor ();

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
