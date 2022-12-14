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

#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <goocanvas.h>

#include "util/canvas.hpp"
#include "util/attribute.hpp"
#include "util/glade.hpp"

#include "players_store.hpp"
#include "players_list.hpp"

namespace People
{
  GList *PlayersList::_clipboard = nullptr;

  // --------------------------------------------------------------------------------
  PlayersList::PlayersList (const gchar    *glade_file,
                            AntiCheatBlock *anti_cheat_block,
                            guint           rights)
    : Object ("PlayersList"),
    Module (glade_file)
  {
    _rights             = rights;
    _player_list        = nullptr;
    _store              = nullptr;
    _column_width       = nullptr;
    _selector_column    = -1;
    _parcel_name        = nullptr;
    _list_changes_muted = FALSE;
    _anti_cheat_block   = anti_cheat_block;

    {
      _tree_view = GTK_TREE_VIEW (_glade->GetWidget ("players_list"));

      if (_tree_view)
      {
        {
          g_object_set (_tree_view,
                        "rules-hint",  TRUE,
                        "reorderable", FALSE,
                        NULL);

          gtk_tree_selection_set_mode (gtk_tree_view_get_selection (_tree_view),
                                       GTK_SELECTION_MULTIPLE);
        }

        // Popup menu
        {
          _ui_manager = gtk_ui_manager_new ();

          {
            GError *error = nullptr;
            static const gchar xml[] =
              "<ui>\n"
              "  <popup name='PopupMenu'>\n"
              "    <menuitem action='CopyAction'/>\n"
              "    <menuitem action='PasteCloneAction'/>\n"
              "    <menuitem action='PasteLinkAction'/>\n"
              "    <menuitem action='RemoveAction'/>\n"
              "  </popup>\n"
              "</ui>";

            if (gtk_ui_manager_add_ui_from_string (_ui_manager,
                                                   xml,
                                                   -1,
                                                   &error) == FALSE)
            {
              g_message ("building menus failed: %s", error->message);
              g_error_free (error);
              error = nullptr;
            }
          }

          // CopyAction
          {
            static GtkActionEntry entries[] =
            {
              {"CopyAction", GTK_STOCK_COPY, gettext ("Copy"), nullptr, nullptr, G_CALLBACK (OnCopySelection)}
            };

            AddPopupEntries ("PlayersList::CopyAction",
                             G_N_ELEMENTS (entries),
                             entries);
          }

          // PasteCloneAction
          {
            static GtkActionEntry entries[] =
            {
              {"PasteCloneAction", GTK_STOCK_PASTE, gettext ("Paste"), nullptr, nullptr, G_CALLBACK (OnPasteCloneSelection)}
            };

            AddPopupEntries ("PlayersList::PasteCloneAction",
                             G_N_ELEMENTS (entries),
                             entries);
          }

          // PasteLinkAction
          {
            static GtkActionEntry entries[] =
            {
              {"PasteLinkAction", GTK_STOCK_PASTE, gettext ("Paste (link)"), nullptr, nullptr, G_CALLBACK (OnPasteLinkSelection)}
            };

            AddPopupEntries ("PlayersList::PasteLinkAction",
                             G_N_ELEMENTS (entries),
                             entries);
          }

          // RemoveAction
          {
            static GtkActionEntry entries[] =
            {
              {"RemoveAction", GTK_STOCK_REMOVE, gettext ("Remove"), nullptr, nullptr, G_CALLBACK (OnRemoveSelection)}
            };

            AddPopupEntries ("PlayersList::RemoveAction",
                             G_N_ELEMENTS (entries),
                             entries);
          }

          {
            GtkWidget *menu = gtk_ui_manager_get_widget (_ui_manager, "/PopupMenu");

            gtk_widget_show_all (menu);

            g_signal_connect_swapped (_tree_view, "button_press_event",
                                      G_CALLBACK (OnButtonPress), menu);
          }

          SetPopupVisibility ("PlayersList::PasteCloneAction",
                              FALSE);
          SetPopupVisibility ("PlayersList::PasteLinkAction",
                              FALSE);
          SetPopupVisibility ("PlayersList::RemoveAction",
                              FALSE);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::AddPopupEntries (const gchar    *group,
                                     guint           n,
                                     GtkActionEntry *entries)
  {
    GtkActionGroup *action_group = gtk_action_group_new (group);

    gtk_action_group_add_actions (action_group,
                                  entries,
                                  n,
                                  this);
    gtk_ui_manager_insert_action_group (_ui_manager,
                                        action_group,
                                        0);

    g_object_unref (G_OBJECT (action_group));
  }


  // --------------------------------------------------------------------------------
  void PlayersList::SetPopupVisibility (const gchar *group,
                                        gboolean     visibility)
  {
    GtkActionGroup *action_group = GetActionGroup (group);

    if (action_group)
    {
      gtk_action_group_set_visible (action_group,
                                    visibility);
    }
  }

  // --------------------------------------------------------------------------------
  PlayersList::~PlayersList ()
  {
    Wipe ();
    FreeFullGList (Player,
                   _clipboard);
    Object::TryToRelease (_store);
    //g_object_unref (G_OBJECT (_ui_manager));
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SetFilter (Filter *filter)
  {
    Module::SetFilter (filter);

    if (_filter && (_store == nullptr))
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
        _store->SetSearchColumn (_filter->GetAttributeId ("name"));
        _store->SelectTreeMode (_tree_view);
      }

      g_free (types);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SetVisibileFunc (GtkTreeModelFilterVisibleFunc func,
                                     gpointer                      data)
  {
    _store->SetVisibileFunc (func,
                             data);
  }

  // --------------------------------------------------------------------------------
  GtkUIManager *PlayersList::GetUiManager ()
  {
    return _ui_manager;
  }

  // --------------------------------------------------------------------------------
  GtkActionGroup *PlayersList::GetActionGroup (const gchar *group_name)
  {
    GList *groups = gtk_ui_manager_get_action_groups (_ui_manager);

    while (groups)
    {
      GtkActionGroup *group = (GtkActionGroup *) groups->data;

      if (g_strcmp0 (gtk_action_group_get_name (group), group_name) == 0)
      {
        return group;
      }

      groups = g_list_next (groups);
    }

    return nullptr;
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

    MuteListChanges (TRUE);
    {
      GList *current = _player_list;

      while (current)
      {
        Player *player = (Player *) current->data;

        Update (player);
        current = g_list_next (current);
      }
    }
    MuteListChanges (FALSE);
    NotifyListChanged ();
  }

  // --------------------------------------------------------------------------------
  void PlayersList::UpdateHierarchy (Player *player)
  {
    _store->Update (player);

    {
      GtkTreeRowReference *ref   = _store->GetTreeRowRef (_tree_view, player);
      GtkTreePath         *path  = gtk_tree_row_reference_get_path (ref);

      gtk_tree_view_expand_to_path (_tree_view,
                                    path);
      gtk_tree_path_free (path);
    }
  }

  // --------------------------------------------------------------------------------
  Player *PlayersList::GetPlayerFromRef (guint ref)
  {
    if (ref)
    {
      GList *current = _player_list;

      while (current)
      {
        Player *player = (Player *) current->data;

        if (player->GetRef () == ref)
        {
          return player;
        }
        current = g_list_next (current);
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Update (Player *player)
  {
    GtkTreeRowReference *ref = _store->GetTreeRowRef (_tree_view,
                                                      player);

    if (ref)
    {
      GtkTreePath  *path;
      GtkTreeIter   iter;
      GtkTreeStore *store = _store->GetTreeStore (_tree_view);

      path = gtk_tree_row_reference_get_path (ref);
      gtk_tree_model_get_iter (GTK_TREE_MODEL (store),
                               &iter,
                               path);
      gtk_tree_path_free (path);

      if (_filter)
      {
        GSList *attr_list = _filter->GetAttrList ();

        for (guint i = 0; attr_list != nullptr; i++)
        {
          AttributeDesc       *desc    = (AttributeDesc *) attr_list->data;
          Player::AttributeId *attr_id = Player::AttributeId::Create (desc, GetDataOwner ());
          Attribute           *attr    = player->GetAttribute (attr_id);

          attr_id->Release ();
          if (attr)
          {
            attr->TreeStoreSet (store, &iter,
                                i*AttributeDesc::NB_LOOK + AttributeDesc::LONG_TEXT,  AttributeDesc::LONG_TEXT);
            attr->TreeStoreSet (store, &iter,
                                i*AttributeDesc::NB_LOOK + AttributeDesc::SHORT_TEXT, AttributeDesc::SHORT_TEXT);
            attr->TreeStoreSet (store, &iter,
                                i*AttributeDesc::NB_LOOK + AttributeDesc::GRAPHICAL,  AttributeDesc::GRAPHICAL);
          }
          else
          {
            desc->TreeStoreSetDefault (store, &iter,
                                       i*AttributeDesc::NB_LOOK + AttributeDesc::LONG_TEXT,  AttributeDesc::LONG_TEXT);
            desc->TreeStoreSetDefault (store, &iter,
                                       i*AttributeDesc::NB_LOOK + AttributeDesc::SHORT_TEXT, AttributeDesc::SHORT_TEXT);
            desc->TreeStoreSetDefault (store, &iter,
                                       i*AttributeDesc::NB_LOOK + AttributeDesc::GRAPHICAL,  AttributeDesc::GRAPHICAL);
          }
          attr_list = g_slist_next (attr_list);
        }

        gtk_tree_store_set (store, &iter,
                            gtk_tree_model_get_n_columns (GTK_TREE_MODEL (store)) - 1,
                            player, -1);
      }

      NotifyListChanged ();
      player->Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  GList *PlayersList::GetList ()
  {
    return _player_list;
  }

  // --------------------------------------------------------------------------------
  GSList *PlayersList::CreateCustomList (CustomFilter filter,
                                         PlayersList *owner)
  {
    GSList *custom_list = nullptr;
    GList  *current     = _player_list;

    while (current)
    {
      Player *p = (Player *) current->data;

      if (filter (p, owner) == TRUE)
      {
        custom_list = g_slist_append (custom_list,
                                      p);
      }
      current = g_list_next (current);
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
  GList *PlayersList::RetreiveSelectedPlayers ()
  {
    GList            *result         = nullptr;
    GtkTreeSelection *selection      = gtk_tree_view_get_selection (_tree_view);
    GList            *selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                                             nullptr);

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
        result = g_list_append (result,
                                p);
      }
    }

    g_list_foreach (selection_list, (GFunc) gtk_tree_path_free, nullptr);
    g_list_free    (selection_list);

    return result;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::TogglePlayerAttr (Player              *player,
                                      Player::AttributeId *attr_id,
                                      gboolean             new_value,
                                      gboolean             popup_on_error)
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
      GList *selected_players = RetreiveSelectedPlayers ();

      if (selected_players)
      {
        GList *current_player = selected_players;

        while (current_player)
        {
          Player *p = (Player *) current_player->data;

          TogglePlayerAttr (p,
                            attr_id,
                            !is_active);
          current_player = g_list_next (current_player);
        }

        g_list_free (selected_players);
      }
    }
    else
    {
      Player *p = GetPlayer (path_string);

      TogglePlayerAttr (p,
                        attr_id,
                        !is_active,
                        TRUE);
    }
    attr_id->Release ();
    gtk_tree_path_free (toggeled_path);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnAttrListUpdated ()
  {
    if (_tree_view)
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
        GList *layout_list = _filter->GetLayoutList ();

        while (layout_list)
        {
          Filter::Layout *attr_layout = (Filter::Layout *) layout_list->data;

          SetColumn (_filter->GetAttributeId (attr_layout->_desc->_code_name),
                     attr_layout,
                     -1);
          layout_list = g_list_next (layout_list);
        }
      }

      _store->Refilter ();
    }
  }

  // --------------------------------------------------------------------------------
  gint PlayersList::CompareIterator (GtkTreeModel        *model,
                                     GtkTreeIter         *a,
                                     GtkTreeIter         *b,
                                     Player::AttributeId *attr_id)
  {
    guint32 *anti_cheat_token = (guint32 *) attr_id->GetPtrData (nullptr,
                                                                 "CompareIteratorAntiCheatToken");

    if (anti_cheat_token)
    {
      attr_id->MakeRandomReady (*anti_cheat_token);
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
    GList *current = _player_list;

    while (current)
    {
      Player    *player       = (Player *) current->data;
      Attribute *current_attr = player->GetAttribute (attr_id);

      if (Attribute::Compare (attr, current_attr) == 0)
      {
        return player;
      }
      current = g_list_next (current);
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnCellDataFunc (GtkTreeViewColumn *tree_column,
                                    GtkCellRenderer   *cell,
                                    GtkTreeModel      *tree_model,
                                    GtkTreeIter       *iter,
                                    PlayersList       *players_list)
  {
    players_list->CellDataFunc (tree_column,
                                cell,
                                tree_model,
                                iter);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::CellDataFunc (GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer   *cell,
                                  GtkTreeModel      *tree_model,
                                  GtkTreeIter       *iter)
  {
  }

  // --------------------------------------------------------------------------------
  void PlayersList::SetColumn (guint           id,
                               Filter::Layout *attr_layout,
                               gint            at)
  {
    AttributeDesc       *desc = attr_layout->_desc;
    AttributeDesc::Look  look = attr_layout->_look;
    GtkTreeViewColumn   *column   = nullptr;
    GtkCellRenderer     *renderer = nullptr;
    gboolean             attr_modifiable = desc->GetUIntData (this,
                                                              "modifiable");

    if (   (desc->_type == G_TYPE_UINT)
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

      if (   (desc->_rights == AttributeDesc::Rights::PUBLIC)
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

    g_object_set_data (G_OBJECT (column),
                       "PlayersList::AttributeDesc", desc);
    gtk_tree_view_column_set_cell_data_func (column,
                                             renderer,
                                             (GtkTreeCellDataFunc) OnCellDataFunc,
                                             this,
                                             nullptr);

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
           guint32 token = 0;

          if (_anti_cheat_block)
          {
            token = _anti_cheat_block->GetAntiCheatToken ();
          }
          attr_id->SetData (nullptr,
                            "CompareIteratorAntiCheatToken",
                            &token);
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
    GList *columns = gtk_tree_view_get_columns (_tree_view);

    for (guint c = 0; c < g_list_length (columns) ; c++)
    {
      GList             *renderers;
      GtkTreeViewColumn *column;

      column = GTK_TREE_VIEW_COLUMN (g_list_nth_data (columns, c));
      renderers = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));

      for (guint r = 0; r < g_list_length (renderers) ; r++)
      {
        GtkCellRenderer *renderer;
        gchar           *sensitive_attribute;

        renderer = GTK_CELL_RENDERER (g_list_nth_data (renderers, r));

        sensitive_attribute = (gchar *) g_object_get_data (G_OBJECT (renderer),
                                                           "PlayersList::SensitiveAttribute");
        if (sensitive_attribute)
        {
          g_object_set (renderer,
                        sensitive_attribute, sensitive_value,
                        NULL);
        }
      }

      g_list_free (renderers);
    }

    g_list_free (columns);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Add (Player *player)
  {
    player->Retain ();

    _store->Append (player);

    _player_list = g_list_append (_player_list,
                                  player);

    if (_parcel_name)
    {
      player->Disclose (_parcel_name);
    }

    Update (player);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Wipe ()
  {
    {
      GList *current = _player_list;

      while (current)
      {
        Player *p = (Player *) current->data;

        p->RemoveCbkOwner (this);
        p->Release ();
        current = g_list_next (current);
      }
    }

    g_list_free (_player_list);
    _player_list = nullptr;

    if (_store)
    {
      _store->Wipe ();
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::RemoveSelection ()
  {
    GList        *ref_list = nullptr;
    GtkTreeModel *model    = GTK_TREE_MODEL (_store->GetTreeStore (_tree_view));

    {
      GtkTreeSelection *selection            = gtk_tree_view_get_selection (_tree_view);
      GList            *full_selection_list  = nullptr;
      GList            *short_selection_list = nullptr;

      full_selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                                  nullptr);

      // Forget children when their ancestor is in the selection
      {
        GList *current = full_selection_list;

        while (current)
        {
          GtkTreePath *current_path = (GtkTreePath *) current->data;

          if (gtk_tree_path_get_depth (current_path) == 1)
          {
            short_selection_list = g_list_prepend (short_selection_list,
                                                   current->data);
          }
          else
          {
            GtkTreePath *ancestor_path = gtk_tree_path_copy (current_path);

            gtk_tree_path_up (ancestor_path);
            if (g_list_find_custom (full_selection_list,
                                    ancestor_path,
                                    (GCompareFunc) gtk_tree_path_compare) == nullptr)
            {
              short_selection_list = g_list_prepend (short_selection_list,
                                                     current->data);
            }
            gtk_tree_path_free (ancestor_path);
          }

          current = g_list_next (current);
        }
      }

      // Build a reference list to remove
      {
        GList *current = short_selection_list;

        while (current)
        {
          GtkTreeRowReference *ref;

          {
            GtkTreePath *path;
            GtkTreePath *view_path = (GtkTreePath *) current->data;

            path = _store->GetStorePathFromViewPath (_tree_view,
                                                     view_path);
            ref = gtk_tree_row_reference_new (model,
                                              path);
            gtk_tree_path_free (path);
          }

          ref_list = g_list_append (ref_list,
                                    ref);

          current = g_list_next (current);
        }
      }

      g_list_foreach (full_selection_list, (GFunc) gtk_tree_path_free, nullptr);
      g_list_free    (full_selection_list);
      g_list_free    (short_selection_list);
    }

    // Perform the removing
    MuteListChanges (TRUE);
    {
      GList *current_ref = ref_list;

      while (current_ref)
      {
        GtkTreeRowReference *ref            = (GtkTreeRowReference *) current_ref->data; ;
        GtkTreePath         *selected_path  = gtk_tree_row_reference_get_path (ref);
        GList               *current_player = _player_list;

        gtk_tree_row_reference_free (ref);

        while (current_player)
        {
          Player              *player     = (Player *) current_player->data;
          GtkTreeRowReference *player_ref = _store->GetTreeRowRef (_tree_view, player);

          if (player_ref)
          {
            GtkTreePath *current_path;

            current_path = gtk_tree_row_reference_get_path (player_ref);
            if (gtk_tree_path_compare (selected_path,
                                       current_path) == 0)
            {
              Remove (player);
              gtk_tree_path_free (current_path);
              break;
            }

            gtk_tree_path_free (current_path);
          }

          current_player = g_list_next (current_player);
        }

        gtk_tree_path_free (selected_path);
        current_ref = g_list_next (current_ref);
      }
    }
    MuteListChanges (FALSE);
    NotifyListChanged ();

    g_list_free (ref_list);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Remove (Player *player)
  {
    if (player->IsSticky () == FALSE)
    {
      OnPlayerRemoved (player);

      _store->Remove (player);

      _player_list = g_list_remove (_player_list,
                                    player);

      player->Release ();

      NotifyListChanged ();
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::ExpandAll ()
  {
    gtk_tree_view_expand_all (_tree_view);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::CollapseAll ()
  {
    gtk_tree_view_collapse_all (_tree_view);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::MuteListChanges (gboolean mute)
  {
    _list_changes_muted = mute;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::NotifyListChanged ()
  {
    if (_list_changes_muted == FALSE)
    {
      OnListChanged ();
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnListChanged ()
  {
    MakeDirty ();
  }

  // --------------------------------------------------------------------------------
  Player *PlayersList::GetPlayer (const gchar *path_string)
  {
    Player      *result  = nullptr;
    GtkTreePath *path;
    GList       *current = _player_list;

    {
      GtkTreePath *view_path = gtk_tree_path_new_from_string (path_string);

      path = _store->GetStorePathFromViewPath (_tree_view,
                                               view_path);
      gtk_tree_path_free (view_path);
    }

    while (current)
    {
      GtkTreeRowReference *current_ref;
      Player              *p = (Player *) current->data;

      current_ref = _store->GetTreeRowRef (_tree_view,
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

      current = g_list_next (current);
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
      guint   players_count;
      gdouble canvas_w;
      gdouble canvas_h;

      _column_width = (gdouble *) g_malloc0 (g_list_length (_filter->GetLayoutList ()) * sizeof (gdouble));

      _flat_print = TRUE;
      GetPrintScale (operation,
                     context,
                     &players_count,
                     &_print_scale,
                     &canvas_w,
                     &canvas_h);

      {
        gdouble free_area = GetPrintBodySize (context, SizeReferential::NORMALIZED);

        _nb_player_per_page = guint (players_count * free_area/_print_scale / canvas_h) - 1;
      }

      {
        _nb_pages = players_count / _nb_player_per_page;

        if (players_count % _nb_player_per_page != 0)
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
    GList   *layout_list = _filter->GetLayoutList ();
    gdouble  x           = 0.0;

    for (guint i = 0; layout_list != nullptr; i++)
    {
      Filter::Layout *attr_layout = (Filter::Layout *) layout_list->data;

      if (attr_layout->_desc)
      {
        gchar         *font = g_strdup_printf (BP_FONT "Bold %dpx", guint (PRINT_FONT_HEIGHT));
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
      layout_list = g_list_next (layout_list);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::PrintPlayer (GooCanvasItem   *root_item,
                                 GtkPrintContext *context,
                                 GtkTreePath     *path,
                                 Player          *player,
                                 guint            row,
                                 gboolean         update_column_width)
  {
    GList         *layout_list = _filter->GetLayoutList ();
    gdouble        x           = 0.0;
    GooCanvasItem *bar         = nullptr;

    if (   ((_flat_print == FALSE) && (gtk_tree_path_get_depth (path) == 1))
        || ((_flat_print == TRUE)  && (row % 2)))
    {
      bar = goo_canvas_rect_new (root_item,
                                 0.0, row * (PRINT_FONT_HEIGHT + PRINT_FONT_HEIGHT/3.0),
                                 15.0, PRINT_FONT_HEIGHT,
                                 "stroke-pattern", NULL,
                                 "fill-color", "Grey85",
                                 NULL);
    }

    for (guint j = 0; layout_list != nullptr; j++)
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
            gdouble h;
            gdouble scale;
            gdouble paper_w = gtk_print_context_get_width (context);

            item = Canvas::CreateIcon (root_item,
                                       GTK_STOCK_APPLY,
                                       0.0,
                                       0.0);
            goo_canvas_item_get_bounds (item,
                                        &bounds);
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
            gchar *font;
            gchar *image = attr->GetUserImage (attr_layout->_look);

            if ((_flat_print == FALSE) && (bar && g_strcmp0 (attr->GetCodeName (), "name") == 0))
            {
              font = g_strdup_printf (BP_FONT "Bold %dpx", guint (PRINT_FONT_HEIGHT));
            }
            else
            {
              font = g_strdup_printf (BP_FONT "%dpx", guint (PRINT_FONT_HEIGHT));
            }
            item = goo_canvas_text_new (root_item,
                                        image,
                                        x,
                                        row * (PRINT_FONT_HEIGHT + PRINT_FONT_HEIGHT/3.0),
                                        -1.0,
                                        GTK_ANCHOR_NW,
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

      if (bar)
      {
        g_object_set (G_OBJECT (bar),
                      "width", x,
                      NULL);
      }

      layout_list = g_list_next (layout_list);
    }
  }

  // --------------------------------------------------------------------------------
  guint PlayersList::PrintAllPlayers (GtkPrintOperation *operation,
                                      GooCanvas         *canvas,
                                      GtkPrintContext   *context,
                                      gint               page_nr)
  {
    guint       players_count   = 0;
    gboolean    print_full_list = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "PRINT_STAGE_VIEW"));
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_first (gtk_tree_view_get_model (_tree_view),
                                       &iter))
    {
      Player *player = GetPlayer (gtk_tree_view_get_model (_tree_view), &iter);

      if ((print_full_list == FALSE) && (PlayerIsPrintable (player) == FALSE))
      {
        player = GetNextPrintablePlayer (gtk_tree_view_get_model (_tree_view),
                                         &iter,
                                         print_full_list);
      }

      if (page_nr != -1)
      {
        for (guint i = 0; i < page_nr*_nb_player_per_page; i++)
        {
          player = GetNextPrintablePlayer (gtk_tree_view_get_model (_tree_view),
                                           &iter,
                                           print_full_list);
        }
      }

      while (player)
      {
        GtkTreePath *path = gtk_tree_model_get_path (gtk_tree_view_get_model (_tree_view),
                                                     &iter);

        PrintPlayer (goo_canvas_get_root_item (canvas),
                     context,
                     path,
                     player,
                     players_count+1,
                     (page_nr == -1));
        gtk_tree_path_free (path);
        players_count++;

        if ((page_nr != -1) && (players_count >= _nb_player_per_page))
        {
          break;
        }

        player = GetNextPrintablePlayer (gtk_tree_view_get_model (_tree_view),
                                         &iter,
                                         print_full_list);
      }
    }

    return players_count;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::GetPrintScale (GtkPrintOperation *operation,
                                   GtkPrintContext   *context,
                                   guint             *players_count,
                                   gdouble           *scale,
                                   gdouble           *canvas_w,
                                   gdouble           *canvas_h)
  {
    GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

    PrintHeader (goo_canvas_get_root_item (canvas),
                 TRUE);

    *players_count = PrintAllPlayers (operation,
                                      canvas,
                                      context,
                                      -1);

    {
      {
        *canvas_w = 0;

        for (guint i = 0; i < g_list_length (_filter->GetLayoutList ()); i++)
        {
          *canvas_w += _column_width[i];
        }

        *canvas_h = *players_count * (PRINT_FONT_HEIGHT + PRINT_FONT_HEIGHT/3.0);
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
    if (_column_width == nullptr)
    {
      return;
    }

    if (_filter)
    {
      cairo_t *cr = gtk_print_context_get_cairo_context (context);

      {
        gdouble    paper_w = gtk_print_context_get_width (context);
        gdouble    paper_h = gtk_print_context_get_height (context);
        GooCanvas *canvas  = Canvas::CreatePrinterCanvas (context);
        char      *text    = g_strdup_printf ("%s %d/%d", gettext ("Page"), page_nr+1, _nb_pages);

        goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                             text,
                             99.0, 94.0*(paper_h/paper_w),
                             -1.0,
                             GTK_ANCHOR_SE,
                             "fill-color", "black",
                             "font", BP_FONT "Bold 2px", NULL);
        g_free (text);

        goo_canvas_render (canvas,
                           cr,
                           nullptr,
                           1.0);

        gtk_widget_destroy (GTK_WIDGET (canvas));
      }

      cairo_save (cr);
      {
        GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

        PrintHeader (goo_canvas_get_root_item (canvas),
                     FALSE);

        PrintAllPlayers (operation,
                         canvas,
                         context,
                         page_nr);

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
                           nullptr,
                           1.0);

        gtk_widget_destroy (GTK_WIDGET (canvas));
      }
      cairo_restore (cr);
    }
  }

  // --------------------------------------------------------------------------------
  Player *PlayersList::GetNextPrintablePlayer (GtkTreeModel *model,
                                               GtkTreeIter  *iter,
                                               gboolean      print_full_list)
  {
    while (1)
    {
      if (IterNextNode (model,
                        iter) == FALSE)
      {
        return nullptr;
      }

      {
        Player *player = GetPlayer (gtk_tree_view_get_model (_tree_view), iter);

        if (print_full_list || PlayerIsPrintable (player))
        {
          return player;
        }
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersList::IterNextNode (GtkTreeModel *model,
                                      GtkTreeIter  *iter)
  {
    {
      GtkTreeIter next_node;

      if (gtk_tree_model_iter_children (model,
                                        &next_node,
                                        iter))
      {
        _flat_print = FALSE;
        *iter = next_node;
        return TRUE;
      }
    }

    {
      GtkTreeIter tmp_node = *iter;

      if (gtk_tree_model_iter_next (model,
                                    iter))
      {
        return TRUE;
      }
      *iter = tmp_node;
    }

    {
      GtkTreeIter next_node;

      if (gtk_tree_model_iter_parent (model,
                                      &next_node,
                                      iter))
      {
        *iter = next_node;
        return (gtk_tree_model_iter_next (model,
                                          iter));
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnEndPrint (GtkPrintOperation *operation,
                                GtkPrintContext   *context)
  {
    g_free (_column_width);
    _column_width = nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersList::PlayerIsPrintable (Player *player)
  {
    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnCopySelection (GtkWidget   *w,
                                     PlayersList *players_list)
  {
    FreeFullGList (Player,
                   _clipboard);

    _clipboard = players_list->RetreiveSelectedPlayers ();

    for (GList *current = _clipboard; current; current = g_list_next (current))
    {
      Player *player = (Player *) current->data;

      player->Retain ();
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnPasteCloneSelection (GtkWidget   *w,
                                           PlayersList *players_list)
  {
    for (GList *current = _clipboard; current; current = g_list_next (current))
    {
      Player *player     = (Player *) current->data;
      Player *duplicated = (Player *) player->Duplicate ();

      players_list->Add (duplicated);
      duplicated->Release ();
    }

    FreeFullGList (Player,
                   _clipboard);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnPasteLinkSelection (GtkWidget   *w,
                                          PlayersList *players_list)
  {
    for (GList *current = _clipboard; current; current = g_list_next (current))
    {
      Player *player = (Player *) current->data;

      if (g_list_find (players_list->_player_list, current->data) == nullptr)
      {
        players_list->Add (player);
      }
    }

    FreeFullGList (Player,
                   _clipboard);
  }

  // --------------------------------------------------------------------------------
  void PlayersList::OnRemoveSelection (GtkWidget   *w,
                                       PlayersList *players_list)
  {
    players_list->RemoveSelection ();
  }

  // --------------------------------------------------------------------------------
  gint PlayersList::OnButtonPress (GtkWidget *widget,
                                   GdkEvent  *event)
  {
    if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *event_button = (GdkEventButton *) event;

      if (event_button->button == 3)
      {
        gtk_menu_popup (GTK_MENU (widget),
                        nullptr, nullptr, nullptr, nullptr,
                        event_button->button, event_button->time);
        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersList::IsTableBorder (guint place)
  {
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Spread ()
  {
    GList *current = _player_list;

    while (current)
    {
      Player *p = (Player *) current->data;

      p->Spread ();

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Recall ()
  {
    GList *current = _player_list;

    while (current)
    {
      Player *p = (Player *) current->data;

      p->Recall ();

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Disclose (const gchar *as,
                              Object      *extra_feeder)
  {
    GList *current = _player_list;

    while (current)
    {
      Player       *p      = (Player *) current->data;
      Net::Message *parcel = p->Disclose (as);

      if (extra_feeder)
      {
        extra_feeder->FeedParcel (parcel);
      }

      current = g_list_next (current);
    }

    _parcel_name = as;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::Conceal ()
  {
    GList *current = _player_list;

    while (current)
    {
      Player *p = (Player *) current->data;

      p->Conceal ();

      current = g_list_next (current);
    }

    _parcel_name = nullptr;
  }

  // --------------------------------------------------------------------------------
  void PlayersList::DumpToHTML (FILE *file)
  {
    if (_filter && file)
    {
      fprintf (file,
               "        <table class=\"List\">\n");

      // Header
      {
        GList *current_attr_desc = _filter->GetLayoutList ();

        fprintf (file, "          <tr class=\"ListHeader\">\n");
        while (current_attr_desc)
        {
          Filter::Layout *layout    = (Filter::Layout *) current_attr_desc->data;
          AttributeDesc  *attr_desc = layout->_desc;

          fprintf (file,
                   "            <th>%s</th>\n",
                   attr_desc->_user_name);

          current_attr_desc = g_list_next (current_attr_desc);
        }
        fprintf (file, "          </tr>\n");
      }

      // Fencers
      {
        GList *current_player = _player_list;

        for (guint i = 0; current_player; i++)
        {
          Player *player = (Player *) current_player->data;

          if (IsTableBorder (i))
          {
            fprintf (file, "          <tr class=\"Separator\">\n");
          }

          if (i%2)
          {
            fprintf (file, "          <tr class=\"OddRow\">\n");
          }
          else
          {
            fprintf (file, "          <tr class=\"EvenRow\">\n");
          }

          {
            GList *current_attr_desc = _filter->GetLayoutList ();

            while (current_attr_desc)
            {
              Filter::Layout      *layout    = (Filter::Layout *) current_attr_desc->data;
              AttributeDesc       *attr_desc = layout->_desc;
              Player::AttributeId *attr_id;
              Attribute           *attr;

              if (attr_desc->_scope == AttributeDesc::Scope::GLOBAL)
              {
                attr_id = new Player::AttributeId  (attr_desc->_code_name);
              }
              else
              {
                attr_id = new Player::AttributeId  (attr_desc->_code_name,
                                                    GetDataOwner ());
              }

              attr = player->GetAttribute (attr_id);
              attr_id->Release ();
              if (attr)
              {
                gchar *image = attr->GetUserImage (AttributeDesc::LONG_TEXT);

                fprintf (file, "            <td>%s</td>\n", image);
                g_free (image);
              }
              else
              {
                fprintf (file, "            <td></td>\n");
              }

              current_attr_desc = g_list_next (current_attr_desc);
            }
          }
          fprintf (file, "          </tr>\n");
          current_player = g_list_next (current_player);
        }
      }

      fprintf (file,
               "        </table>\n");
    }
  }
}
