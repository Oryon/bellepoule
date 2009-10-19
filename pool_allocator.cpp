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

#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "pool_allocator.hpp"

#define VALUE_INIT {0,{{0}}}

enum
{
  POOL_SIZE_COL,
  NB_POOLS_COL,
  BEST_PIXMAP_COL
} ComboboxColumn;

extern "C" G_MODULE_EXPORT void on_nb_pools_combobox_changed (GtkWidget *widget,
                                                              Object_c  *owner);
extern "C" G_MODULE_EXPORT void on_pool_size_combobox_changed (GtkWidget *widget,
                                                               Object_c  *owner);

const gchar *PoolAllocator_c::_class_name = "pool_allocation_stage";

// --------------------------------------------------------------------------------
PoolAllocator_c::PoolAllocator_c (gchar *name)
: Stage_c (name),
  CanvasModule_c ("pool_allocator.glade")
{
  _pools_list      = NULL;
  _config_list     = NULL;
  _nb_pools        = 0;
  _pool_size       = 0;
  _dragging        = false;
  _target_pool     = NULL;
  _floating_player = NULL;
  _drag_text       = NULL;
  _main_table      = NULL;
  _attendees       = NULL;

  _combobox_store = GTK_LIST_STORE (_glade->GetObject ("combo_liststore"));

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("nb_pools_combobox"));
    AddSensitiveWidget (_glade->GetWidget ("pool_size_combobox"));
  }

  {
    ShowAttribute ("name");
    ShowAttribute ("first_name");
    ShowAttribute ("rating");
    ShowAttribute ("rank");
    ShowAttribute ("club");
    ShowAttribute ("ref");
  }
}

// --------------------------------------------------------------------------------
PoolAllocator_c::~PoolAllocator_c ()
{
  Wipe ();
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::Init ()
{
  RegisterStageClass (_class_name,
                      CreateInstance);
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::OnPlugged ()
{
  CanvasModule_c::OnPlugged ();
  Display ();
}

// --------------------------------------------------------------------------------
Stage_c *PoolAllocator_c::CreateInstance ()
{
  return new PoolAllocator_c ("Pool allocation");
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::Load (xmlNode *xml_node)
{
  static Pool_c *current_pool   = NULL;
  Stage_c       *previous_stage = GetPreviousStage ();

  if (_attendees == NULL)
  {
    _attendees = g_slist_copy (previous_stage->GetResult ());
  }

  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, "player_list") == 0)
      {
        current_pool = new Pool_c (_nb_pools+1);

        _pools_list = g_slist_append (_pools_list,
                                      current_pool);
        _nb_pools++;
      }
      else if (strcmp ((char *) n->name, "player") == 0)
      {
        gchar *attr;

        attr = (gchar *) xmlGetProp (n, BAD_CAST "ref");
        if (attr)
        {
          Player_c *player = previous_stage->GetPlayerFromRef (atoi (attr));

          if (player)
          {
            current_pool->AddPlayer (player);
          }
        }
      }
      else if (strcmp ((char *) n->name, _class_name) != 0)
      {
        FillCombobox ();
        SetUpCombobox ();
        return;
      }
    }

    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _class_name);
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "name",
                                     "%s", GetName ());
  if (_pools_list)
  {
    for (guint i = 0; i < g_slist_length (_pools_list); i++)
    {
      Pool_c *pool;

      pool = (Pool_c *) g_slist_nth_data (_pools_list, i);
      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "player_list");
      for (guint j = 0; j < pool->GetNbPlayers (); j++)
      {
        Player_c *player;

        player = pool->GetPlayer (j);

        xmlTextWriterStartElement (xml_writer,
                                   BAD_CAST "player");
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "ref",
                                           "%d", player->GetRef ());
        xmlTextWriterEndElement (xml_writer);
      }
      xmlTextWriterEndElement (xml_writer);
    }
  }
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::FillCombobox ()
{
  guint nb_players = g_slist_length (_attendees);

  _best_nb_pools = 1;

  for (guint nb_pool = 1; nb_pool <= nb_players / 3; nb_pool++)
  {
    for (guint i = 0; i < nb_pool; i++)
    {
      guint size;

      size = (nb_players + nb_pool - i) / nb_pool;

      if (   (size >= 3)
          && (nb_pool * (size - 1) + i == nb_players))
      {
        {
          Configuration *config = (Configuration *) g_malloc (sizeof (Configuration));
          gchar         *combo_text;
          GtkTreeIter    iter;

          config->size         = size;
          config->nb_pool      = nb_pool;
          config->has_two_size = false;

          _config_list = g_slist_append (_config_list,
                                         config);
          gtk_list_store_append (_combobox_store, &iter);
          {
            if (i != 0)
            {
              config->has_two_size = true;
              combo_text = g_strdup_printf ("%d ou %d", size, size - 1);
            }
            else
            {
              combo_text = g_strdup_printf ("%d", size - 1);
            }

            gtk_list_store_set (_combobox_store, &iter,
                                POOL_SIZE_COL, combo_text,
                                -1);
            g_free (combo_text);
          }

          {
            combo_text = g_strdup_printf ("%d", nb_pool);
            gtk_list_store_set (_combobox_store, &iter,
                                NB_POOLS_COL, combo_text,
                                -1);
            g_free (combo_text);
          }

          if (size >= 7)
          {
            _best_nb_pools = nb_pool;
          }
        }

        // skip the other configuartions with the same size
        nb_pool = (nb_players / (size - 1));
        break;
      }
    }
  }

  {
    GtkTreeIter  iter;

    if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_combobox_store),
                                       &iter,
                                       NULL,
                                       _best_nb_pools - 1))
    {
      gtk_list_store_set (_combobox_store, &iter,
                          BEST_PIXMAP_COL, GTK_STOCK_ABOUT,
                          -1);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::CreatePools ()
{
  for (guint i = 0; i < _nb_pools; i++)
  {
    Pool_c *pool;

    pool = new Pool_c (i);
    _pools_list = g_slist_append (_pools_list,
                                  pool);

    for (guint j = 0; j < _pool_size; j++)
    {
      Player_c *player;

      player = (Player_c *) g_slist_nth_data (_attendees,
                                              i + _nb_pools * j);
      if (player)
      {
        pool->AddPlayer (player);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::SetUpCombobox ()
{
  GtkWidget *w;

  w = _glade->GetWidget ("nb_pools_combobox");
  g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                        (void *) on_nb_pools_combobox_changed,
                                        (Object_c *) this);
  gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                            _nb_pools - 1);
  g_signal_connect (G_OBJECT (w), "changed",
                    G_CALLBACK (on_nb_pools_combobox_changed),
                    (Object_c *) this);

  w = _glade->GetWidget ("pool_size_combobox");
  g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                        (void *) on_pool_size_combobox_changed,
                                        (Object_c *) this);
  gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                            _nb_pools - 1);
  g_signal_connect (G_OBJECT (w), "changed",
                    G_CALLBACK (on_pool_size_combobox_changed),
                    (Object_c *) this);
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::on_button_press (GooCanvasItem  *item,
                                           GooCanvasItem  *target,
                                           GdkEventButton *event,
                                           Pool_c         *pool)
{
  PoolAllocator_c *pl = (PoolAllocator_c*) pool->GetData ("PoolAllocator_c::pool_allocator");

  if (pl)
  {
    return pl->OnButtonPress (item,
                              target,
                              event,
                              pool);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::OnButtonPress (GooCanvasItem  *item,
                                         GooCanvasItem  *target,
                                         GdkEventButton *event,
                                         Pool_c         *pool)
{
  if (Locked ())
  {
    return FALSE;
  }

  if (   (event->button == 1)
         && (event->type == GDK_BUTTON_PRESS))
  {
    _floating_player = (Player_c *) g_object_get_data (G_OBJECT (item),
                                                       "PoolAllocator_c::player");

    _drag_x = event->x;
    _drag_y = event->y;

    goo_canvas_convert_from_item_space  (GetCanvas (),
                                         item,
                                         &_drag_x,
                                         &_drag_y);

    {
      GooCanvasBounds  bounds;
      GString         *string = g_string_new ("");

      if (_floating_player && _attr_list)
      {
        for (guint i = 0; i < g_slist_length (_attr_list); i++)
        {
          gchar       *attr_name;
          Attribute_c *attr;

          attr_name = (gchar *) g_slist_nth_data (_attr_list,
                                                  i);
          attr = _floating_player->GetAttribute (attr_name);

          if (attr)
          {
            string = g_string_append (string,
                                      attr->GetStringImage ());
            string = g_string_append (string,
                                      "  ");
          }
        }
      }

      goo_canvas_item_get_bounds (item,
                                  &bounds);

      _drag_text = goo_canvas_text_new (GetRootItem (),
                                        string->str,
                                        //bounds.x1,
                                        //bounds.y1,
                                        _drag_x,
                                        _drag_y,
                                        -1,
                                        GTK_ANCHOR_NW,
                                        "font", "Sans Bold 14px", NULL);
      g_string_free (string,
                     TRUE);

    }

    {
      GdkCursor *fleur;

      fleur = gdk_cursor_new (GDK_FLEUR);
      gdk_window_set_cursor (gtk_widget_get_window (GetRootWidget ()),
                             fleur);
      gdk_cursor_unref (fleur);
    }

    _dragging = true;

    _source_pool = pool;

    {
      pool->RemovePlayer (_floating_player);
      FillPoolTable (pool);
      FixUpTablesBounds ();
    }

    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::on_button_release (GooCanvasItem  *item,
                                             GooCanvasItem  *target,
                                             GdkEventButton *event,
                                             PoolAllocator_c    *pl)
{
  if (pl)
  {
    return pl->OnButtonRelease (item,
                                target,
                                event);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::OnButtonRelease (GooCanvasItem  *item,
                                           GooCanvasItem  *target,
                                           GdkEventButton *event)
{
  if (_dragging)
  {
    _dragging = false;

    goo_canvas_item_remove (_drag_text);
    _drag_text = NULL;

    if (_target_pool)
    {
      _target_pool->AddPlayer (_floating_player);
      FillPoolTable (_target_pool);
      _target_pool = NULL;
    }
    else
    {
      _source_pool->AddPlayer (_floating_player);
      FillPoolTable (_source_pool);
    }

    FixUpTablesBounds ();

    _floating_player = NULL;
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::on_motion_notify (GooCanvasItem  *item,
                                            GooCanvasItem  *target,
                                            GdkEventButton *event,
                                            PoolAllocator_c    *pl)
{
  if (pl)
  {
    pl->OnMotionNotify (item,
                        target,
                        event);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::OnMotionNotify (GooCanvasItem  *item,
                                          GooCanvasItem  *target,
                                          GdkEventButton *event)
{
  if (_dragging && (event->state & GDK_BUTTON1_MASK))
  {
    gdouble new_x = event->x_root;
    gdouble new_y = event->y_root;

    goo_canvas_item_translate (_drag_text,
                               new_x - _drag_x,
                               new_y - _drag_y);

    _drag_x = new_x;
    _drag_y = new_y;
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::on_enter_notify (GooCanvasItem  *item,
                                           GooCanvasItem  *target,
                                           GdkEventButton *event,
                                           Pool_c         *pool)
{
  PoolAllocator_c *pl = (PoolAllocator_c*) pool->GetData ("PoolAllocator_c::pool_allocator");

  if (pl)
  {
    return pl->OnEnterNotify (item,
                              target,
                              event,
                              pool);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::OnEnterNotify (GooCanvasItem  *item,
                                         GooCanvasItem  *target,
                                         GdkEventButton *event,
                                         Pool_c         *pool)
{
  if (_dragging)
  {
    GooCanvasItem *rect = (GooCanvasItem*) pool->GetData ("PoolAllocator_c::focus_rectangle");

    g_object_set (G_OBJECT (rect),
                  "stroke-color", "black",
                  NULL);

    _target_pool = pool;

    return TRUE;
  }
  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::on_leave_notify (GooCanvasItem  *item,
                                           GooCanvasItem  *target,
                                           GdkEventButton *event,
                                           Pool_c         *pool)
{
  PoolAllocator_c *pl = (PoolAllocator_c*) pool->GetData ("PoolAllocator_c::pool_allocator");

  if (pl)
  {
    return pl->OnLeaveNotify (item,
                              target,
                              event,
                              pool);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator_c::OnLeaveNotify (GooCanvasItem  *item,
                                         GooCanvasItem  *target,
                                         GdkEventButton *event,
                                         Pool_c         *pool)
{
  if (_dragging)
  {
    GooCanvasItem *rect = (GooCanvasItem*) pool->GetData ("PoolAllocator_c::focus_rectangle");

    g_object_set (G_OBJECT (rect),
                  "stroke-pattern", NULL,
                  NULL);

    _target_pool = NULL;

    return TRUE;
  }
  return FALSE;
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::FixUpTablesBounds ()
{
  for (guint p = 0; p < g_slist_length (_pools_list); p++)
  {
    Pool_c          *pool;
    GooCanvasItem   *focus_rect;
    GooCanvasBounds  bounds;
    GooCanvasItem   *table;

    pool = (Pool_c *) g_slist_nth_data (_pools_list, p);
    table = (GooCanvasItem *) pool->GetData ("PoolAllocator_c::table");

    focus_rect = (GooCanvasItem *) pool->GetData ("PoolAllocator_c::focus_rectangle");

    goo_canvas_item_get_bounds (table,
                                &bounds);

    g_object_set (G_OBJECT (focus_rect),
                  "x",      bounds.x1,
                  "y",      bounds.y1,
                  "width",  bounds.x2 - bounds.x1,
                  "height", bounds.y2 - bounds.y1,
                  NULL);

    if (_dragging)
    {
      goo_canvas_item_raise (focus_rect,
                             NULL);
    }
    else
    {
      goo_canvas_item_lower (focus_rect,
                             NULL);

      g_object_set (G_OBJECT (focus_rect),
                    "stroke-pattern", NULL,
                    NULL);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::FillPoolTable (Pool_c *pool)
{
  if (_main_table == NULL)
  {
    return;
  }

  GooCanvasItem *table      = (GooCanvasItem *) pool->GetData ("PoolAllocator_c::table");
  GooCanvasItem *focus_rect = (GooCanvasItem *) pool->GetData ("PoolAllocator_c::focus_rectangle");
  GooCanvasItem *item;

  if (table)
  {
    goo_canvas_item_remove (table);
  }

  {
    guint nb_columns;
    guint nb_rows;

    nb_columns = 2;
    nb_rows = _nb_pools / nb_columns;
    if (_nb_pools % nb_columns != 0)
    {
      nb_rows++;
    }

    {
      guint row    = pool->GetNumber () % nb_rows;
      guint column = pool->GetNumber () / nb_rows;

      table = goo_canvas_table_new (_main_table,
                                    "row-spacing",    2.0,
                                    "column-spacing", 4.0,
                                    NULL);
      PutInTable (_main_table, table,
                  row,
                  column);
      pool->SetData ("PoolAllocator_c::table",
                     table);
    }
  }

  {
    guint          pool_size   = pool->GetNbPlayers ();
    Configuration *best_config = NULL;

    for (guint i = 0; i < g_slist_length (_config_list); i ++)
    {
      best_config = (Configuration *) g_slist_nth_data (_config_list,
                                                        i);
      if (best_config->nb_pool == _nb_pools)
      {
        break;
      }
    }

    item = PutInTable (table,
                       0, 0,
                       pool->GetName ());
    g_object_set (G_OBJECT (item),
                  "font", "Sans bold 18px",
                  NULL);

    if (   (pool_size == best_config->size - 1)
        || (   best_config->has_two_size
            && (pool_size == best_config->size) || (pool_size == best_config->size - 1)))
    {
      g_object_set (G_OBJECT (item),
                    "fill_color", "medium blue",
                    NULL);
    }
    else
    {
      g_object_set (G_OBJECT (item),
                    "fill_color", "dark red",
                    NULL);
    }
  }

  for (guint p = 0; p < pool->GetNbPlayers (); p++)
  {
    Player_c *player = pool->GetPlayer (p);

    if (player && _attr_list)
    {
      for (guint i = 0; i < g_slist_length (_attr_list); i++)
      {
        GooCanvasItem *item;
        gchar         *attr_name;
        Attribute_c   *attr;

        attr_name = (gchar *) g_slist_nth_data (_attr_list,
                                                i);
        attr = player->GetAttribute (attr_name);

        if (attr)
        {
          item = PutInTable (table,
                             p+1, i,
                             attr->GetStringImage ());
        }
        else
        {
          item = PutInTable (table,
                             p+1, i,
                             "");
        }
        g_object_set (G_OBJECT (item),
                      "font", "Sans 14px",
                      "fill_color", "black",
                      NULL);
        g_object_set_data (G_OBJECT (item),
                           "PoolAllocator_c::player",
                           player);

        g_signal_connect (item, "button_press_event",
                          G_CALLBACK (on_button_press), pool);
      }
    }
  }

  if (focus_rect == NULL)
  {
    focus_rect = goo_canvas_rect_new (GetRootItem (),
                                      0, 0,
                                      0, 0,
                                      "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                      "stroke-pattern", NULL,
                                      NULL);

    goo_canvas_item_lower (focus_rect,
                           NULL);

    g_signal_connect (focus_rect, "enter_notify_event",
                      G_CALLBACK (on_enter_notify), pool);
    g_signal_connect (focus_rect, "leave_notify_event",
                      G_CALLBACK (on_leave_notify), pool);

    pool->SetData ("PoolAllocator_c::pool_allocator",
                   this);
    pool->SetData ("PoolAllocator_c::focus_rectangle",
                   focus_rect);
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::Display ()
{
  if (_main_table)
  {
    goo_canvas_item_remove (_main_table);
  }

  {
    GooCanvasItem *root = GetRootItem ();

    _main_table = goo_canvas_table_new (root,
                                        "row-spacing",    20.0,
                                        "column-spacing", 40.0,
                                        NULL);

    g_object_set (G_OBJECT (root),
                  "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                  NULL);
    g_signal_connect (root, "motion_notify_event",
                      G_CALLBACK (on_motion_notify), this);
    g_signal_connect (root, "button_release_event",
                      G_CALLBACK (on_button_release), this);

    for (guint i = 0; i < g_slist_length (_pools_list); i++)
    {
      Pool_c *pool;

      pool = (Pool_c *) g_slist_nth_data (_pools_list, i);
      FillPoolTable (pool);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::DeletePools ()
{
  if (_pools_list)
  {
    for (guint i = 0; i < g_slist_length (_pools_list); i++)
    {
      Pool_c *pool;

      pool = (Pool_c *) g_slist_nth_data (_pools_list, i);
      Object_c::Release (pool);
    }

    g_slist_free (_pools_list);
    _pools_list = NULL;
  }

  CanvasModule_c::Wipe ();
  _main_table = NULL;
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::Wipe ()
{
  DeletePools ();

  {
    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("pool_size_combobox"),
                                          (void *) on_pool_size_combobox_changed,
                                          (Object_c *) this);
    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("nb_pools_combobox"),
                                          (void *) on_nb_pools_combobox_changed,
                                          (Object_c *) this);

    gtk_list_store_clear (_combobox_store);

    g_signal_connect (_glade->GetWidget ("pool_size_combobox"), "changed",
                      G_CALLBACK (on_pool_size_combobox_changed),
                      (Object_c *) this);
    g_signal_connect (_glade->GetWidget ("nb_pools_combobox"), "changed",
                      G_CALLBACK (on_nb_pools_combobox_changed),
                      (Object_c *) this);
  }

  if (_config_list)
  {
    for (guint i = 0; i < g_slist_length (_config_list); i++)
    {
      Configuration *config;

      config = (Configuration *) g_slist_nth_data (_config_list, i);
      g_free (config);
    }
    g_slist_free (_config_list);
    _config_list = NULL;
  }

  g_slist_free (_attendees);
  _attendees = NULL;
}

// --------------------------------------------------------------------------------
guint PoolAllocator_c::GetNbPools ()
{
  return g_slist_length (_pools_list);
}

// --------------------------------------------------------------------------------
Pool_c *PoolAllocator_c::GetPool (guint index)
{
  return (Pool_c *) g_slist_nth_data (_pools_list,
                                      index);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pool_size_combobox_changed (GtkWidget *widget,
                                                               Object_c  *owner)
{
  PoolAllocator_c *pl = dynamic_cast <PoolAllocator_c *> (owner);

  pl->OnComboboxChanged (GTK_COMBO_BOX (widget));
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::OnComboboxChanged (GtkComboBox *cb)
{
  gint config_index     = gtk_combo_box_get_active (cb);
  Configuration *config = (Configuration *) g_slist_nth_data (_config_list,
                                                              config_index);

  if (config)
  {
    {
      GtkWidget *w;

      w = _glade->GetWidget ("nb_pools_combobox");
      g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                            (void *) on_nb_pools_combobox_changed,
                                            (Object_c *) this);
      gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                config_index);
      g_signal_connect (G_OBJECT (w), "changed",
                        G_CALLBACK (on_nb_pools_combobox_changed),
                        (Object_c *) this);

      w = _glade->GetWidget ("pool_size_combobox");
      g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                            (void *) on_pool_size_combobox_changed,
                                            (Object_c *) this);
      gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                config_index);
      g_signal_connect (G_OBJECT (w), "changed",
                        G_CALLBACK (on_pool_size_combobox_changed),
                        (Object_c *) this);
    }

    _pool_size = config->size;
    _nb_pools  = config->nb_pool;

    DeletePools ();
    CreatePools ();
    Display ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_nb_pools_combobox_changed (GtkWidget *widget,
                                                              Object_c  *owner)
{
  PoolAllocator_c *c = dynamic_cast <PoolAllocator_c *> (owner);

  c->OnComboboxChanged (GTK_COMBO_BOX (widget));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_print_toolbutton_clicked (GtkWidget *widget,
                                                             Object_c  *owner)
{
  PoolAllocator_c *c = dynamic_cast <PoolAllocator_c *> (owner);

  c->Print ();
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::Enter ()
{
  EnableSensitiveWidgets ();

  if (_pools_list == NULL)
  {
    Stage_c *previous_stage = GetPreviousStage ();

    _attendees = g_slist_copy (previous_stage->GetResult ());

    FillCombobox ();

    _nb_pools = _best_nb_pools;

    if (_config_list)
    {
      Configuration *best_config;

      best_config = (Configuration *) g_slist_nth_data (_config_list,
                                                        _nb_pools - 1);
      _pool_size = best_config->size;

      CreatePools ();
      SetUpCombobox ();
    }
  }

  Display ();
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::OnLocked ()
{
  DisableSensitiveWidgets ();

  _result = g_slist_copy (_attendees);
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::OnUnLocked ()
{
  EnableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void PoolAllocator_c::OnAttrListUpdated ()
{
  for (guint i = 0; i < g_slist_length (_pools_list); i++)
  {
    Pool_c *pool;

    pool = (Pool_c *) g_slist_nth_data (_pools_list, i);
    FillPoolTable (pool);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_filter_button_clicked (GtkWidget *widget,
                                                          Object_c  *owner)
{
  PoolAllocator_c *p = dynamic_cast <PoolAllocator_c *> (owner);

  p->SelectAttributes ();
}
