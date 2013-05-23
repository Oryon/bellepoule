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
#include <goocanvas.h>

#include "util/canvas.hpp"
#include "util/attribute.hpp"
#include "common/schedule.hpp"
#include "common/player.hpp"

#include "players_list.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  PlayersList::PlayersList (const gchar *glade_file,
                            guint        rights)
    : Module (glade_file)
  {
    _rights          = rights;
    _player_list     = NULL;
    _store           = NULL;
    _column_width    = NULL;
    _selector_column = -1;

    {
      _tree_view = GTK_TREE_VIEW (_glade->GetWidget ("players_list"));

      if (_tree_view)
      {
        g_object_set (_tree_view,
                      "rules-hint",  TRUE,
                      "reorderable", FALSE,
                      NULL);

        gtk_tree_selection_set_mode (gtk_tree_view_get_selection (_tree_view),
                                     GTK_SELECTION_MULTIPLE);
      }
    }
  }

  // --------------------------------------------------------------------------------
  PlayersList::~PlayersList ()
  {
    Wipe ();

    Object::TryToRelease (_store);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SetFilter (Filter *filter)
  {
    Module::SetFilter (filter);

    if (_filter)
    {
      GSList *current = _filter->GetAttrList ();
      guint   nb_attr = g_slist_length (current);
      GType  *types   = g_new (GType, nb_attr*AttributeDesc::NB_LOOK + 1);

      for (guint i = 0; current; i++)
      {
        AttributeDesc *desc = (AttributeDesc *) current->data;

        types[i*AttributeDesc::NB_LOOK + AttributeDesc::LONG_TEXT]  = desc->GetGType (AttributeDesc::LONG_TEXT);
        types[i*AttributeDesc::NB_LOOK + AttributeDesc::SHORT_TEXT] = desc->GetGType (AttributeDesc::SHORT_TEXT);
        types[i*AttributeDesc::NB_LOOK + AttributeDesc::GRAPHICAL]  = desc->GetGType (AttributeDesc::GRAPHICAL);

        if (desc->_is_selector)
        {
          _selector_column = i;
        }

        current = g_slist_next (current);
      }

      // The player reference used in comparison
      types[nb_attr*AttributeDesc::NB_LOOK] = G_TYPE_POINTER;

      _store = new PlayersStore (nb_attr*AttributeDesc::NB_LOOK + 1,
                                 types);

      if (_tree_view)
      {
        _store->SelectFlatMode (_tree_view);

        gtk_tree_view_set_search_column (_tree_view,
                                         _filter->GetAttributeId ("name"));
      }

      g_free (types);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SelectTreeMode ()
  {
    if (_store->SelectTreeMode (_tree_view))
    {
      RefreshDisplay ();
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SelectFlatMode ()
  {
    if (_store->SelectFlatMode (_tree_view))
    {
      RefreshDisplay ();
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::RefreshDisplay ()
  {
    OnAttrListUpdated ();

    {
      GSList *current = _player_list;

      while (current)
      {
        Player *player = (Player *) current->data;

        Update (player);
        current = g_slist_next (current);
      }
    }

    OnListChanged ();
  }

  // --------------------------------------------------------------------------------
  void PlayersList::UpdateHierarchy (Player *player)
  {
    _store->Update (player);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Update (Player *player)
  {
    GtkTreeStore        *model = GTK_TREE_STORE (gtk_tree_view_get_model (_tree_view));
    GtkTreeRowReference *ref   = _store->GetTreeRowRef (GTK_TREE_MODEL (model),
                                                        player);

    if (ref)
    {
      GtkTreePath         *path;
      GtkTreeIter          iter;

      path = gtk_tree_row_reference_get_path (ref);
      gtk_tree_model_get_iter (GTK_TREE_MODEL (model),
                               &iter,
                               path);
      gtk_tree_path_free (path);

      if (_filter)
      {
        GSList *attr_list = _filter->GetAttrList ();

        for (guint i = 0; attr_list != NULL; i++)
        {
          AttributeDesc       *desc    = (AttributeDesc *) attr_list->data;
          Player::AttributeId *attr_id = Player::AttributeId::Create (desc, GetDataOwner ());
          Attribute           *attr    = player->GetAttribute (attr_id);

          attr_id->Release ();
          if (attr)
          {
            attr->TreeStoreSet (model, &iter,
                                i*AttributeDesc::NB_LOOK + AttributeDesc::LONG_TEXT,  AttributeDesc::LONG_TEXT);
            attr->TreeStoreSet (model, &iter,
                                i*AttributeDesc::NB_LOOK + AttributeDesc::SHORT_TEXT, AttributeDesc::SHORT_TEXT);
            attr->TreeStoreSet (model, &iter,
                                i*AttributeDesc::NB_LOOK + AttributeDesc::GRAPHICAL,  AttributeDesc::GRAPHICAL);
          }
          else
          {
            desc->TreeStoreSetDefault (model, &iter,
                                       i*AttributeDesc::NB_LOOK + AttributeDesc::LONG_TEXT,  AttributeDesc::LONG_TEXT);
            desc->TreeStoreSetDefault (model, &iter,
                                       i*AttributeDesc::NB_LOOK + AttributeDesc::SHORT_TEXT, AttributeDesc::SHORT_TEXT);
            desc->TreeStoreSetDefault (model, &iter,
                                       i*AttributeDesc::NB_LOOK + AttributeDesc::GRAPHICAL,  AttributeDesc::GRAPHICAL);
          }
          attr_list = g_slist_next (attr_list);
        }

        gtk_tree_store_set (model, &iter,
                            gtk_tree_model_get_n_columns (GTK_TREE_MODEL (model)) - 1,
                            player, -1);
      }
    }
  }

  // --------------------------------------------------------------------------------
  GSList *PlayersList::GetList ()
  {
    return _player_list;
  }

  // --------------------------------------------------------------------------------
  GSList *PlayersList::CreateCustomList (CustomFilter filter)
  {
    GSList *custom_list = NULL;
    GSList *current     = _player_list;

    while (current)
    {
      Player *p = (Player *) current->data;

      if (filter (p) == TRUE)
      {
        custom_list = g_slist_append (custom_list,
                                      p);
      }
      current = g_slist_next (current);
    }

    return custom_list;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::on_cell_toggled (GtkCellRendererToggle *cell,
                                     gchar                 *path_string,
                                     gpointer               user_data)
  {
    PlayersList   *p    = (PlayersList *) user_data;
    AttributeDesc *desc = (AttributeDesc *) g_object_get_data (G_OBJECT (cell),
                                                               "PlayersList::attribute_desc");

    p->OnCellToggled (path_string,
                      gtk_cell_renderer_toggle_get_active (cell),
                      desc);
  }

  // --------------------------------------------------------------------------------
  GSList *PlayersList::GetSelectedPlayers ()
  {
    GSList           *result         = NULL;
    GtkTreeSelection *selection      = gtk_tree_view_get_selection (_tree_view);
    GList            *selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                                             NULL);

    for (guint i = 0; i < g_list_length (selection_list); i++)
    {
      Player *p;
      gchar  *path;

      path = gtk_tree_path_to_string ((GtkTreePath *) g_list_nth_data (selection_list,
                                                                       i));
      p = GetPlayer (path);
      g_free (path);

      if (p)
      {
        result = g_slist_append (result,
                                 p);
      }
    }

    g_list_foreach (selection_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free    (selection_list);

    return result;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::TogglePlayerAttr (Player              *player,
                                      Player::AttributeId *attr_id,
                                      gboolean             new_value)
  {
    if (player)
    {
      player->SetAttributeValue (attr_id,
                                 new_value);

      Update (player);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnCellToggled (gchar         *path_string,
                                   gboolean       is_active,
                                   AttributeDesc *desc)
  {
    GtkTreePath         *toggeled_path = gtk_tree_path_new_from_string (path_string);
    Player::AttributeId *attr_id       = Player::AttributeId::Create (desc,
                                                                      GetDataOwner ());

    if (gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (_tree_view),
                                             toggeled_path))
    {
      GSList *selected_players = GetSelectedPlayers ();

      if (selected_players)
      {
        GSList *current_player = selected_players;

        while (current_player)
        {
          Player *p = (Player *) current_player->data;

          TogglePlayerAttr (p,
                            attr_id,
                            !is_active);
          current_player = g_slist_next (current_player);
        }

        g_slist_free (selected_players);
      }
    }
    else
    {
      Player *p = GetPlayer (path_string);

      TogglePlayerAttr (p,
                        attr_id,
                        !is_active);
    }
    attr_id->Release ();
    gtk_tree_path_free (toggeled_path);

    OnListChanged ();
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnAttrListUpdated ()
  {
    {
      GList *column_list = gtk_tree_view_get_columns (_tree_view);
      guint  nb_col      = g_list_length (column_list);

      for (guint i = 0; i < nb_col; i++)
      {
        GtkTreeViewColumn *column;

        column = gtk_tree_view_get_column (_tree_view,
                                           0);
        if (column)
        {
          gtk_tree_view_remove_column (_tree_view,
                                       column);
        }
      }
    }

    if (_filter)
    {
      GSList *layout_list = _filter->GetLayoutList ();

      while (layout_list)
      {
        Filter::Layout *attr_layout = (Filter::Layout *) layout_list->data;

        SetColumn (_filter->GetAttributeId (attr_layout->_desc->_code_name),
                   attr_layout,
                   -1);
        layout_list = g_slist_next (layout_list);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gint PlayersList::CompareIterator (GtkTreeModel        *model,
                                     GtkTreeIter         *a,
                                     GtkTreeIter         *b,
                                     Player::AttributeId *attr_id)
  {
    guint32 *rand_seed = (guint32 *) attr_id->GetPtrData (NULL,
                                                          "CompareIteratorRandSeed");

    if (rand_seed)
    {
      attr_id->MakeRandomReady (*rand_seed);
    }
    return Player::Compare (GetPlayer (model, a),
                            GetPlayer (model, b),
                            attr_id);
  }

  // --------------------------------------------------------------------------------
  Player *PlayersList::GetPlayer (GtkTreeModel *model,
                                  GtkTreeIter  *iter)
  {
    Player *player;

    gtk_tree_model_get (model, iter,
                        gtk_tree_model_get_n_columns (model) - 1,
                        &player, -1);

    return player;
  }

  // --------------------------------------------------------------------------------
  Player *PlayersList::GetPlayerWithAttribute (Player::AttributeId *attr_id,
                                               Attribute           *attr)
  {
    GSList *current = _player_list;

    while (current)
    {
      Player    *player       = (Player *) current->data;
      Attribute *current_attr = player->GetAttribute (attr_id);

      if (Attribute::Compare (attr, current_attr) == 0)
      {
        return player;
      }
      current = g_slist_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SetColumn (guint           id,
                               Filter::Layout *attr_layout,
                               gint            at)
  {
    AttributeDesc       *desc = attr_layout->_desc;
    AttributeDesc::Look  look = attr_layout->_look;
    GtkTreeViewColumn   *column   = NULL;
    GtkCellRenderer     *renderer = NULL;
    gboolean             attr_modifiable = desc->GetUIntData (this,
                                                              "modifiable");

    if (   (desc->_type == G_TYPE_INT)
        && (look & AttributeDesc::GRAPHICAL))
    {
      renderer = gtk_cell_renderer_progress_new ();
      column = gtk_tree_view_column_new_with_attributes (desc->_user_name,
                                                         renderer,
                                                         "value", id*AttributeDesc::NB_LOOK + look,
                                                         NULL);
    }
    else if (desc->GetGType (look) == G_TYPE_OBJECT)
    {
      renderer = gtk_cell_renderer_pixbuf_new ();

      column = gtk_tree_view_column_new_with_attributes (desc->_user_name,
                                                         renderer,
                                                         "pixbuf", id*AttributeDesc::NB_LOOK + look,
                                                         NULL);
    }
    else if (   (desc->_type == G_TYPE_BOOLEAN)
             && (look == AttributeDesc::GRAPHICAL))
    {
      renderer = gtk_cell_renderer_toggle_new ();

      if (   (desc->_rights == AttributeDesc::PUBLIC)
          && ((_rights & MODIFIABLE) || attr_modifiable))
      {
        g_object_set (renderer,
                      "activatable", TRUE,
                      NULL);
        g_signal_connect (renderer,
                          "toggled", G_CALLBACK (on_cell_toggled), this);
        g_object_set_data (G_OBJECT (renderer),
                           "PlayersList::SensitiveAttribute",
                           (void *) "activatable");
      }

      column = gtk_tree_view_column_new_with_attributes (desc->_user_name,
                                                         renderer,
                                                         "active", id*AttributeDesc::NB_LOOK + look,
                                                         NULL);
    }
    else
    {
      if (desc->HasDiscreteValue ())
      {
        renderer = gtk_cell_renderer_combo_new ();
      }
      else
      {
        renderer = gtk_cell_renderer_text_new ();
      }

      column = gtk_tree_view_column_new_with_attributes (desc->_user_name,
                                                         renderer,
                                                         "text", id*AttributeDesc::NB_LOOK + look,
                                                         NULL);
    }

    if (renderer && column)
    {
      g_object_set_data (G_OBJECT (renderer),
                         "PlayersList::attribute_desc", desc);

      if (_rights & SORTABLE)
      {
        Player::AttributeId *attr_id = Player::AttributeId::Create (desc, GetDataOwner ());

        gtk_tree_view_column_set_sort_column_id (column,
                                                 id*AttributeDesc::NB_LOOK + AttributeDesc::LONG_TEXT);

        if (desc->_type != G_TYPE_STRING)
        {
          attr_id->SetData (NULL,
                            "CompareIteratorRandSeed",
                            &_rand_seed);
        }
        _store->SetSortFunction (id*AttributeDesc::NB_LOOK + AttributeDesc::LONG_TEXT,
                                 (GtkTreeIterCompareFunc) CompareIterator,
                                 attr_id);

        _store->SetSortFunction (id*AttributeDesc::NB_LOOK + AttributeDesc::SHORT_TEXT,
                                 (GtkTreeIterCompareFunc) CompareIterator,
                                 attr_id);

        _store->SetSortFunction (id*AttributeDesc::NB_LOOK + AttributeDesc::GRAPHICAL,
                                 (GtkTreeIterCompareFunc) CompareIterator,
                                 attr_id);

        attr_id->Release ();
      }

      gtk_tree_view_insert_column (_tree_view,
                                   column,
                                   at);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SetAttributeRight (const gchar *name,
                                       gboolean     modifiable)
  {
    AttributeDesc *desc = AttributeDesc::GetDescFromCodeName (name);

    if (desc)
    {
      desc->SetData (this,
                     "modifiable",
                     (void *) modifiable);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SetSensitiveState (gboolean sensitive_value)
  {
    {
      GList *columns = gtk_tree_view_get_columns (_tree_view);

      for (guint c = 0; c < g_list_length (columns) ; c++)
      {
        GList             *renderers;
        GtkTreeViewColumn *column;

        column = GTK_TREE_VIEW_COLUMN (g_list_nth_data (columns, c));
        renderers = gtk_tree_view_column_get_cell_renderers (column);

        for (guint r = 0; r < g_list_length (renderers) ; r++)
        {
          GtkCellRenderer *renderer;
          gchar           *sensitive_attribute;

          renderer = GTK_CELL_RENDERER (g_list_nth_data (renderers, r));

          sensitive_attribute = (gchar *) g_object_get_data (G_OBJECT (renderer),
                                                             "PlayersList::SensitiveAttribute");
          g_object_set (renderer,
                        sensitive_attribute, sensitive_value,
                        NULL);
        }

        g_list_free (renderers);
      }

      g_list_free (columns);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Add (Player *player)
  {
    player->Retain ();

    _store->Append (player);

    _player_list = g_slist_append (_player_list,
                                   player);

    Update (player);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Wipe ()
  {
    {
      GSList *current = _player_list;

      while (current)
      {
        Player *p = (Player *) current->data;

        p->RemoveCbkOwner (this);
        p->Release ();
        current = g_slist_next (current);
      }
    }

    g_slist_free (_player_list);
    _player_list = NULL;

    if (_store)
    {
      _store->Wipe ();
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::RemoveSelection ()
  {
    GList        *ref_list = NULL;
    GtkTreeModel *model    = gtk_tree_view_get_model (_tree_view);

    {
      GtkTreeSelection *selection      = gtk_tree_view_get_selection (_tree_view);
      GList            *selection_list = NULL;

      selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                             NULL);

      for (guint i = 0; i < g_list_length (selection_list); i++)
      {
        GtkTreeRowReference *ref;

        ref = gtk_tree_row_reference_new (model,
                                          (GtkTreePath *) g_list_nth_data (selection_list, i));
        ref_list = g_list_append (ref_list,
                                  ref);
      }

      g_list_foreach (selection_list, (GFunc) gtk_tree_path_free, NULL);
      g_list_free    (selection_list);
    }


    for (guint i = 0; i < g_list_length (ref_list); i++)
    {
      GtkTreeRowReference *ref;
      GtkTreePath         *selected_path;
      GSList              *current_player;

      ref = (GtkTreeRowReference *) g_list_nth_data (ref_list, i);
      selected_path = gtk_tree_row_reference_get_path (ref);
      gtk_tree_row_reference_free (ref);

      current_player = _player_list;
      while (current_player)
      {
        Player      *p = (Player *) current_player->data;
        GtkTreePath *current_path;

        current_path = gtk_tree_row_reference_get_path (_store->GetTreeRowRef (model,
                                                                               p));
        if (gtk_tree_path_compare (selected_path,
                                   current_path) == 0)
        {
          _store->Remove (p);

          _player_list = g_slist_remove (_player_list,
                                         p);
          OnPlayerRemoved (p);
          p->Release ();
          gtk_tree_path_free (current_path);
          break;
        }

        gtk_tree_path_free (current_path);
        current_player = g_slist_next (current_player);
      }
      gtk_tree_path_free (selected_path);
    }

    g_list_free (ref_list);

    OnListChanged ();
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Remove (Player *player)
  {
    _store->Remove (player);

    _player_list = g_slist_remove (_player_list,
                                   player);

    OnPlayerRemoved (player);
    player->Release ();
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnListChanged ()
  {
    MakeDirty ();
  }

  // --------------------------------------------------------------------------------
  Player *PlayersList::GetPlayer (const gchar *path_string)
  {
    Player       *result  = NULL;
    GtkTreePath  *path    = gtk_tree_path_new_from_string (path_string);
    GSList       *current = _player_list;
    GtkTreeModel *model   = gtk_tree_view_get_model (_tree_view);

    while (current)
    {
      GtkTreeRowReference *current_ref;
      Player              *p = (Player *) current->data;

      current_ref = _store->GetTreeRowRef (model,
                                            p);
      if (current_ref)
      {
        GtkTreePath *current_path = gtk_tree_row_reference_get_path (current_ref);

        if (gtk_tree_path_compare (path,
                                   current_path) == 0)
        {
          result = p;
          gtk_tree_path_free (current_path);
          break;
        }
        gtk_tree_path_free (current_path);
      }

      current = g_slist_next (current);
    }
    gtk_tree_path_free (path);

    return result;
  }

  // --------------------------------------------------------------------------------
  guint PlayersList::PreparePrint (GtkPrintOperation *operation,
                                   GtkPrintContext   *context)
  {
    _nb_pages = 0;

    if (_filter)
    {
      guint   nb_players  = g_slist_length (_player_list);
      gdouble canvas_w;
      gdouble canvas_h;

      _column_width = (gdouble *) g_malloc0 (g_slist_length (_filter->GetLayoutList ()) * sizeof (gdouble));

      GetPrintScale (operation,
                     context,
                     &_print_scale,
                     &canvas_w,
                     &canvas_h);

      {
        gdouble paper_w   = gtk_print_context_get_width (context);
        gdouble paper_h   = gtk_print_context_get_height (context);
        gdouble free_area = (paper_h*100.0/paper_w - PRINT_HEADER_HEIGHT+2.0);

        _nb_player_per_page = guint (nb_players * free_area/_print_scale / canvas_h) - 1;
      }

      {
        _nb_pages = nb_players / _nb_player_per_page;

        if (nb_players % _nb_player_per_page != 0)
        {
          _nb_pages++;
        }
      }
    }

    return _nb_pages;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::PrintHeader (GooCanvasItem *root_item,
                                 gboolean       update_column_width)
  {
    GSList *layout_list = _filter->GetLayoutList ();
    gdouble x           = 0.0;

    for (guint i = 0; layout_list != NULL; i++)
    {
      Filter::Layout *attr_layout = (Filter::Layout *) layout_list->data;

      if (attr_layout->_desc)
      {
        gchar         *font = g_strdup_printf ("Sans Bold %dpx", guint (PRINT_FONT_HEIGHT));
        GooCanvasItem *item;

        item = goo_canvas_text_new (root_item,
                                    attr_layout->_desc->_user_name,
                                    x,
                                    0.0,
                                    -1.0,
                                    GTK_ANCHOR_W,
                                    "font", font,
                                    NULL);
        g_free (font);

        {
          GooCanvasBounds bounds;
          gdouble         width;

          goo_canvas_item_get_bounds (item,
                                      &bounds);
          goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (root_item),
                                            item,
                                            &bounds.x1,
                                            &bounds.y1);
          goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (root_item),
                                            item,
                                            &bounds.x2,
                                            &bounds.y2);
          width = PRINT_FONT_HEIGHT + (bounds.x2 - bounds.x1);
          if (_column_width[i] < width)
          {
            _column_width[i] = width;
          }
        }
      }
      x += _column_width[i];
      layout_list = g_slist_next (layout_list);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::PrintPlayer (GooCanvasItem   *root_item,
                                 GtkPrintContext *context,
                                 Player          *player,
                                 guint            row,
                                 gboolean         update_column_width)
  {
    GSList  *layout_list = _filter->GetLayoutList ();
    gdouble  x           = 0;

    for (guint j = 0; layout_list != NULL; j++)
    {
      Filter::Layout      *attr_layout = (Filter::Layout *) layout_list->data;
      Player::AttributeId *attr_id     = Player::AttributeId::Create (attr_layout->_desc, GetDataOwner ());
      Attribute           *attr        = player->GetAttribute (attr_id);

      attr_id->Release ();
      if (attr)
      {
        GooCanvasItem   *item;
        GooCanvasBounds  bounds;

        if (attr_layout->_desc->_type == G_TYPE_BOOLEAN)
        {
          if (attr->GetUIntValue () != 0)
          {
            gdouble w;
            gdouble h;
            gdouble scale;
            gdouble paper_w = gtk_print_context_get_width (context);

            item = Canvas::CreateIcon (root_item,
                                       GTK_STOCK_APPLY,
                                       0.0,
                                       0.0);
            goo_canvas_item_get_bounds (item,
                                        &bounds);
            w = bounds.x2-bounds.x1;
            h = bounds.y2-bounds.y1;

            scale = paper_w*(PRINT_FONT_HEIGHT/100.0)/h;

            goo_canvas_item_set_simple_transform (item,
                                                  x,
                                                  row * (PRINT_FONT_HEIGHT + PRINT_FONT_HEIGHT/3.0),
                                                  scale,
                                                  0.0);
            goo_canvas_item_get_bounds (item,
                                        &bounds);
          }
        }
        else
        {
          GdkPixbuf *pixbuf = attr->GetPixbuf ();

          if (pixbuf && (attr_layout->_look == AttributeDesc::GRAPHICAL))
          {
            gdouble height = PRINT_FONT_HEIGHT;
            gdouble width  = height *
              (gdouble) gdk_pixbuf_get_width (pixbuf) / (gdouble) gdk_pixbuf_get_height (pixbuf);

            item = goo_canvas_image_new (root_item,
                                         pixbuf,
                                         x,
                                         row * (PRINT_FONT_HEIGHT + PRINT_FONT_HEIGHT/3.0),
                                         NULL);

            g_object_set (G_OBJECT (item),
                          "width",         width,
                          "height",        height,
                          "scale-to-fit",  TRUE,
                          NULL);
          }
          else
          {
            gchar *font  = g_strdup_printf ("Sans %dpx", guint (PRINT_FONT_HEIGHT));
            gchar *image = attr->GetUserImage (attr_layout->_look);

            item = goo_canvas_text_new (root_item,
                                        image,
                                        x,
                                        row * (PRINT_FONT_HEIGHT + PRINT_FONT_HEIGHT/3.0),
                                        -1.0,
                                        GTK_ANCHOR_W,
                                        "font", font,
                                        NULL);
            g_free (image);
            g_free (font);
          }

          goo_canvas_item_get_bounds (item,
                                      &bounds);

          if (update_column_width)
          {
            gdouble width;

            goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (root_item),
                                              item,
                                              &bounds.x1,
                                              &bounds.y1);
            goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (root_item),
                                              item,
                                              &bounds.x2,
                                              &bounds.y2);
            width = PRINT_FONT_HEIGHT + (bounds.x2 - bounds.x1);
            if (_column_width[j] < width)
            {
              _column_width[j] = width;
            }
          }
        }
      }
      x += _column_width[j];
      layout_list = g_slist_next (layout_list);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::GetPrintScale (GtkPrintOperation *operation,
                                   GtkPrintContext   *context,
                                   gdouble           *scale,
                                   gdouble           *canvas_w,
                                   gdouble           *canvas_h)
  {
    GSList    *current_player;
    guint      nb_players = 0;
    GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

    PrintHeader (goo_canvas_get_root_item (canvas),
                 TRUE);

    current_player = _player_list;
    for (guint i = 0; current_player != NULL; i++)
    {
      Player *player = (Player *) current_player->data;

      if (   (g_object_get_data (G_OBJECT (operation), "PRINT_STAGE_VIEW"))
          || PlayerIsPrintable (player))
      {
        PrintPlayer (goo_canvas_get_root_item (canvas),
                     context,
                     player,
                     nb_players+1,
                     TRUE);
        nb_players++;
      }
      current_player = g_slist_next (current_player);
    }

    {
      {
        *canvas_w = 0;

        for (guint i = 0; i < g_slist_length (_filter->GetLayoutList ()); i++)
        {
          *canvas_w += _column_width[i];
        }

        *canvas_h = nb_players * (PRINT_FONT_HEIGHT + PRINT_FONT_HEIGHT/3.0);
      }

      {
        gdouble canvas_dpi;
        gdouble printer_dpi;

        g_object_get (canvas,
                      "resolution-x", &canvas_dpi,
                      NULL);
        printer_dpi = gtk_print_context_get_dpi_x (context);

        *scale = printer_dpi/canvas_dpi;
      }

      if ((*canvas_w) * (*scale) > 100.0)
      {
        gdouble x_scale = *scale * 100.0/(*canvas_w);

        *scale = x_scale;
      }
      else
      {
        *scale = 1.0;
      }
    }

    gtk_widget_destroy (GTK_WIDGET (canvas));
  }

  // --------------------------------------------------------------------------------
  void PlayersList::DrawPage (GtkPrintOperation *operation,
                              GtkPrintContext   *context,
                              gint               page_nr)
  {
    DrawContainerPage (operation,
                       context,
                       page_nr);

    DrawBarePage (operation,
                  context,
                  page_nr);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::DrawBarePage (GtkPrintOperation *operation,
                                  GtkPrintContext   *context,
                                  gint               page_nr)
  {
    if (_column_width == NULL)
    {
      return;
    }

    if (_filter)
    {
      cairo_t *cr = gtk_print_context_get_cairo_context (context);

      {
        GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);
        char      *text   = g_strdup_printf ("Page %d/%d", page_nr+1, _nb_pages);

        goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                             text,
                             98.0, -2.0,
                             -1.0,
                             GTK_ANCHOR_SE,
                             "fill-color", "black",
                             "font", "Sans Bold 2px", NULL);
        g_free (text);

        goo_canvas_render (canvas,
                           gtk_print_context_get_cairo_context (context),
                           NULL,
                           1.0);

        gtk_widget_destroy (GTK_WIDGET (canvas));
      }

      cairo_save (cr);
      {
        GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

        PrintHeader (goo_canvas_get_root_item (canvas),
                     FALSE);

        {
          GtkTreeIter  iter;
          gboolean     iter_is_valid;
          guint        nb_players = 0;

          iter_is_valid = gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_store),
                                                         &iter,
                                                         NULL,
                                                         page_nr*_nb_player_per_page);

          for (guint i = 0; iter_is_valid && (nb_players < _nb_player_per_page); i++)
          {
            Player *current_player = GetPlayer (GTK_TREE_MODEL (_store), &iter);

            if (   (g_object_get_data (G_OBJECT (operation), "PRINT_STAGE_VIEW"))
                || PlayerIsPrintable (current_player))
            {
              PrintPlayer (goo_canvas_get_root_item (canvas),
                           context,
                           current_player,
                           nb_players+1,
                           FALSE);
              nb_players++;
            }

            iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_store),
                                                      &iter);
          }
        }

        if (_print_scale != 1.0)
        {
          cairo_matrix_t matrix;

          goo_canvas_item_get_transform (goo_canvas_get_root_item (canvas),
                                         &matrix);
          cairo_matrix_scale (&matrix,
                              _print_scale,
                              _print_scale);

          goo_canvas_item_set_transform (goo_canvas_get_root_item (canvas),
                                         &matrix);
        }

        goo_canvas_render (canvas,
                           cr,
                           NULL,
                           1.0);

        gtk_widget_destroy (GTK_WIDGET (canvas));
      }
      cairo_restore (cr);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnEndPrint (GtkPrintOperation *operation,
                                GtkPrintContext   *context)
  {
    g_free (_column_width);
    _column_width = NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersList::PlayerIsPrintable (Player *player)
  {
    return TRUE;
  }
}
