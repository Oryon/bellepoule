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

#include <libxml/encoding.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <gdk/gdkkeysyms.h>

#include "util/attribute.hpp"
#include "util/player.hpp"
#include "util/filter.hpp"
#include "util/glade.hpp"
#include "util/data.hpp"
#include "util/xml_scheme.hpp"
#include "network/advertiser.hpp"
#include "network/message.hpp"
#include "../common/point_system.hpp"
#include "../../book/section.hpp"
#include "../../classification.hpp"
#include "../../contest.hpp"
#include "../../error.hpp"
#include "table.hpp"

#include "table_supervisor.hpp"

namespace Table
{
  const gchar *Supervisor::_class_name     = N_("Table");
  const gchar *Supervisor::_xml_class_name = "PhaseDeTableaux";

  enum class DisplayColumnId
  {
    TABLE_SET_NAME_str,
    TABLE_SET_TABLE_ptr,
    TABLE_SET_STATUS_str,
    TABLE_SET_VISIBILITY_bool
  };

  // --------------------------------------------------------------------------------
  Supervisor::Supervisor (StageClass *stage_class)
    : Object ("Table::Supervisor"),
      Stage (stage_class),
      Module ("table_supervisor.glade")
  {
    Disclose ("Stage");

    _max_score = new Data ("ScoreMax",
                           10);

    _fenced_places = new Data ("PlacesTirees",
                               (guint) NONE);

    _displayed_table_set = nullptr;

    {
      AddSensitiveWidget (_glade->GetWidget ("input_toolbutton"));
      AddSensitiveWidget (_glade->GetWidget ("qualified_table"));
      AddSensitiveWidget (_glade->GetWidget ("classification_vbox"));
      AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
      AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

      LockOnClassification (_glade->GetWidget ("input_toolbutton"));
    }

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
#endif
                                          "incident",
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "score_quest",
                                          "tiebreaker_quest",
                                          "nb_matchs",
                                          "HS",
                                          "attending",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          "strip",
                                          "time",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("stage_start_rank");
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");

      SetFilter (filter);
      filter->Release ();
    }

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
#endif
                                          "incident",
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "HS",
                                          "attending",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "promoted",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          "strip",
                                          "time",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("rank");
      filter->ShowAttribute ("status");
#ifdef DEBUG
      filter->ShowAttribute ("stage_start_rank");
#endif
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");

      SetClassificationFilter (filter);
      filter->Release ();
    }

    _table_set_treestore = GTK_TREE_STORE (_glade->GetGObject ("table_set_treestore"));
    _table_set_filter    = GTK_TREE_MODEL_FILTER (_glade->GetGObject ("table_set_treemodelfilter"));

    gtk_tree_model_filter_set_visible_column (_table_set_filter,
                                              (gint) DisplayColumnId::TABLE_SET_VISIBILITY_bool);
  }

  // --------------------------------------------------------------------------------
  Supervisor::~Supervisor ()
  {
    _max_score->Release     ();
    _fenced_places->Release ();

    DeleteTableSets ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);
  }

  // --------------------------------------------------------------------------------
  const gchar *Supervisor::GetXmlClassName ()
  {
    return _xml_class_name;
  }

  // --------------------------------------------------------------------------------
  Stage *Supervisor::CreateInstance (StageClass *stage_class)
  {
    return new Supervisor (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Display ()
  {
    OnTableSetSelected (_displayed_table_set);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Recall ()
  {
    {
      OnTableSetSelected (_displayed_table_set);

      gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                              (GtkTreeModelForeachFunc) RecallTableSet,
                              this);
    }

    Stage::Recall ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnTableSetSelected (TableSet *table_set)
  {
    if (_displayed_table_set)
    {
      _displayed_table_set->UnPlug ();
      _displayed_table_set = nullptr;
    }

    if (table_set)
    {
      Plug (table_set,
            _glade->GetWidget ("table_set_hook"));
      table_set->Display ();

      _displayed_table_set = table_set;
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::IsOver ()
  {
    _is_over     = TRUE;
    _first_error = nullptr;

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                            (GtkTreeModelForeachFunc) TableSetIsOver,
                            this);

    return _is_over;
  }

  // --------------------------------------------------------------------------------
  Error *Supervisor::GetError ()
  {
    if (_first_error)
    {
      return _first_error->SpawnError ();
    }
    else
    {
      return nullptr;
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::TableSetIsOver (GtkTreeModel *model,
                                       GtkTreePath  *path,
                                       GtkTreeIter  *iter,
                                       Supervisor   *ts)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);
    if (table_set->GetFirstPlace () == 1)
    {
      ts->_is_over &= table_set->IsOver ();
    }

    if (ts->_first_error == nullptr)
    {
      ts->_first_error = table_set->GetFirstError ();
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::ProcessMessage (GtkTreeModel *model,
                                       GtkTreePath  *path,
                                       GtkTreeIter  *iter,
                                       Net::Message *message)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);

    return table_set->OnMessage (message);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::CreateTableSets ()
  {
    GSList *short_list = GetShortList ();

    if (short_list)
    {
      guint nb_players = g_slist_length (short_list);
      guint nb_tables  = 0;

      for (guint i = 0; i < 32; i++)
      {
        guint bit_cursor;

        bit_cursor = 1;
        bit_cursor = bit_cursor << i;
        if (bit_cursor >= nb_players)
        {
          nb_tables = i++;
          break;
        }
      }

      if (nb_tables > 0)
      {
        {
          gtk_tree_store_clear (_table_set_treestore);

          FeedTableSetStore (1,
                             nb_tables,
                             nullptr);
        }

        {
          GtkTreeIter  iter;
          TableSet    *first_table_set;

          gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                         &iter);
          gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                              DisplayColumnId::TABLE_SET_TABLE_ptr, &first_table_set,
                              -1);

          if (first_table_set)
          {
            Player::AttributeId  attr_id   = Player::AttributeId ("stage_start_rank", this);
            GSList              *attendees = g_slist_copy (short_list);

            attendees = SpreadAttendees (attendees);

            first_table_set->SetAttendees (attendees);
            _displayed_table_set = first_table_set;
          }
        }
      }
    }

    SetTableSetsState ();
  }

  // --------------------------------------------------------------------------------
  GSList *Supervisor::SpreadAttendees (GSList *attendees)
  {
    Player::AttributeId attr_id = Player::AttributeId ("stage_start_rank", this);

    return g_slist_sort_with_data (attendees,
                                   (GCompareDataFunc) Player::Compare,
                                   &attr_id);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::SetTableSetsState ()
  {
    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            (GtkTreeModelForeachFunc) ActivateTableSet,
                            this);
    gtk_tree_view_expand_all (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")));

    if (_displayed_table_set == nullptr)
    {
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                         &iter))
      {
        GtkTreePath *path;

        path = gtk_tree_model_get_path (GTK_TREE_MODEL (_table_set_treestore), &iter);

        gtk_tree_view_set_cursor (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")),
                                  path,
                                  nullptr,
                                  FALSE);

        gtk_tree_path_free (path);
      }
    }

    SignalStatusUpdate ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::ShowTableSet (TableSet    *table_set,
                                 GtkTreeIter *iter)
  {
    gtk_tree_store_set (_table_set_treestore, iter,
                        DisplayColumnId::TABLE_SET_VISIBILITY_bool, TRUE,
                        -1);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::HideTableSet (TableSet    *table_set,
                                 GtkTreeIter *iter)
  {
    gtk_tree_store_set (_table_set_treestore, iter,
                        DisplayColumnId::TABLE_SET_VISIBILITY_bool, FALSE,
                        -1);

    if (_displayed_table_set && (_displayed_table_set == table_set))
    {
      _displayed_table_set->UnPlug ();
      _displayed_table_set = nullptr;
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::TableSetIsFenced (TableSet *table_set)
  {
    if (table_set->GetFirstPlace () == 1)
    {
      return TRUE;
    }
    else if (_fenced_places->GetValue () == ALL_PLACES)
    {
      return TRUE;
    }
    else if (_fenced_places->GetValue () == THIRD_PLACES)
    {
      if (table_set->GetFirstPlace () <= 3)
      {
        return TRUE;
      }
    }
    else if (_fenced_places->GetValue () == QUOTA)
    {
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::ActivateTableSet (GtkTreeModel *model,
                                         GtkTreePath  *path,
                                         GtkTreeIter  *iter,
                                         Supervisor   *ts)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);

    if (ts->TableSetIsFenced (table_set))
    {
      table_set->Activate ();
      if (table_set->HasAttendees ())
      {
        ts->ShowTableSet (table_set,
                          iter);
      }
      else
      {
        ts->HideTableSet (table_set,
                          iter);
      }
    }
    else
    {
      table_set->DeActivate ();
      ts->HideTableSet (table_set,
                        iter);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::RecallTableSet (GtkTreeModel *model,
                                       GtkTreePath  *path,
                                       GtkTreeIter  *iter,
                                       Supervisor   *ts)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);

    table_set->Recall ();

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  TableSet *Supervisor::GetTableSet (gchar *id)
  {
    GtkTreeIter  iter;
    GtkTreePath *path = gtk_tree_path_new_from_string (id);
    TableSet    *table_set = nullptr;

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_treestore),
                                 &iter,
                                 path))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                          DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                          -1);
    }

    gtk_tree_path_free (path);

    return table_set;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::DeleteTableSet (GtkTreeModel *model,
                                       GtkTreePath  *path,
                                       GtkTreeIter  *iter,
                                       gpointer      data)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);
    if (table_set)
    {
      table_set->Release ();
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::DeleteTableSets ()
  {
    if (_displayed_table_set)
    {
      _displayed_table_set->Wipe ();
      _displayed_table_set->UnPlug ();
      _displayed_table_set = nullptr;
    }

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            DeleteTableSet,
                            nullptr);
    gtk_tree_store_clear (_table_set_treestore);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Garnish ()
  {
    DeleteTableSets ();
    CreateTableSets ();
  }

  // --------------------------------------------------------------------------------
  Generic::PointSystem *Supervisor::GetPointSystem (TableSet *table_set)
  {
    return new Generic::PointSystem (this,
                                     _contest->EloMatters (),
                                     TRUE);
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::ScoreOverflowAllowed ()
  {
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::OnMessage (Net::Message *message)
  {
    if (message->Is ("BellePoule2D::Roadmap"))
    {
      gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                              (GtkTreeModelForeachFunc) ProcessMessage,
                              message);
      return TRUE;
    }
    else if (   message->Is ("SmartPoule::ScoreSheetCall")
             || message->Is ("SmartPoule::Score")
             || message->Is ("BellePoule2D::EndOfBurst"))
    {
      gchar    *batch     = message->GetString ("batch");
      TableSet *table_set = GetTableSet (batch);

      if (table_set)
      {
        table_set->OnMessage (message);
      }
      g_free (batch);

      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::OnHttpPost (const gchar *command,
                                   const gchar **ressource,
                                   const gchar *data)
  {
    gboolean result = FALSE;
    gchar **tokens = g_strsplit_set (*ressource,
                                     ".",
                                     0);

    if (tokens)
    {
      GtkTreePath *path = gtk_tree_path_new_from_string (tokens[0]);

      if (path)
      {
        if (g_strcmp0 (command, "ScoreSheet") == 0)
        {
          gtk_tree_view_set_cursor (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")),
                                    path,
                                    nullptr,
                                    FALSE);

          if (_displayed_table_set)
          {
            gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                               TRUE);

            result = _displayed_table_set->OnHttpPost (command,
                                                       (const gchar**) &tokens[1],
                                                       data);
          }
        }

        gtk_tree_path_free (path);
      }

      g_strfreev (tokens);
    }

    return result;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::LoadConfiguration (xmlNode *xml_node)
  {
    if (xml_node)
    {
      Stage::LoadConfiguration (xml_node);

      if (_fenced_places)
      {
        _fenced_places->Load (xml_node);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Load (xmlNode *xml_node)
  {
    LoadConfiguration (xml_node);

    for (xmlNode *n = xml_node; n != nullptr; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (g_strcmp0 ((char *) n->name, GetXmlClassName ()) == 0)
        {
        }
        else if (g_strcmp0 ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          LoadAttendees (n);
        }
        else if (g_strcmp0 ((char *) n->name, "SuiteDeTableaux") == 0)
        {
          GtkTreeIter  iter;
          TableSet    *table_set;
          gchar       *prop = (gchar *) xmlGetProp (n, BAD_CAST "ID");

          if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                             &iter) == FALSE)
          {
            CreateTableSets ();
          }

          if (prop)
          {
            table_set = GetTableSet (prop);
            if (table_set)
            {
              table_set->Load (n);
            }
            xmlFree (prop);
          }
        }
        else
        {
          return;
        }
      }
      Load (n->children);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::SaveConfiguration (XmlScheme *xml_scheme)
  {
    Stage::SaveConfiguration (xml_scheme);

    if (_fenced_places)
    {
      _fenced_places->Save (xml_scheme);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::SaveHeader (XmlScheme *xml_scheme)
  {
    xml_scheme->StartElement (GetXmlClassName ());

    SaveConfiguration (xml_scheme);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Save (XmlScheme *xml_scheme)
  {
    SaveHeader (xml_scheme);

    SaveAttendees (xml_scheme);

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                            (GtkTreeModelForeachFunc) SaveTableSet,
                            xml_scheme);

    xml_scheme->EndElement ();
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::SaveTableSet (GtkTreeModel *model,
                                     GtkTreePath  *path,
                                     GtkTreeIter  *iter,
                                     XmlScheme    *xml_scheme)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);
    table_set->Save (xml_scheme);

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::FeedTableSetStore (guint        from_place,
                                      guint        nb_tables,
                                      GtkTreeIter *parent)
  {
    GtkTreeIter  iter;
    TableSet    *table_set;

    gtk_tree_store_append (_table_set_treestore,
                           &iter,
                           parent);

    {
      GtkTreePath *path  = gtk_tree_model_get_path (GTK_TREE_MODEL (_table_set_treestore), &iter);

      table_set = new TableSet (this,
                                gtk_tree_path_to_string (path),
                                from_place,
                                GTK_RANGE (_glade->GetWidget ("zoom_scale")),
                                this);

      {
        GtkTreeRowReference *ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (_table_set_treestore),
                                                               path);
        table_set->SetData (this, "tree_row_ref",
                            ref,
                            (GDestroyNotify) gtk_tree_row_reference_free);
      }

      gtk_tree_path_free (path);

      table_set->SetDataOwner (this);
      table_set->SetListener (this);

      gtk_tree_store_set (_table_set_treestore, &iter,
                          DisplayColumnId::TABLE_SET_NAME_str,        table_set->GetName (),
                          DisplayColumnId::TABLE_SET_TABLE_ptr,       table_set,
                          DisplayColumnId::TABLE_SET_VISIBILITY_bool, 0,
                          -1);
    }

    {
      guint nb_players   = g_slist_length (GetShortList ());
      guint place_offset = 1;

      for (guint i = 0; i < nb_tables-1; i++)
      {
        place_offset = place_offset << 1;

        if ((from_place+place_offset) < nb_players)
        {
          FeedTableSetStore (from_place + place_offset,
                             i+1,
                             &iter);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnTableSetStatusUpdated (TableSet *table_set)
  {
    GtkTreeIter iter;

    {
      GtkTreePath *path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) table_set->GetPtrData (this, "tree_row_ref"));

      gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_treestore),
                               &iter,
                               path);
      gtk_tree_path_free (path);
    }

    if (table_set->IsOver ())
    {
      gtk_tree_store_set (_table_set_treestore, &iter,
                          DisplayColumnId::TABLE_SET_STATUS_str, GTK_STOCK_APPLY,
                          -1);
    }
    else if (table_set->HasError ())
    {
      gtk_tree_store_set (_table_set_treestore, &iter,
                          DisplayColumnId::TABLE_SET_STATUS_str, GTK_STOCK_DIALOG_WARNING,
                          -1);
    }
    else if (table_set->GetNbTables () > 0)
    {
      gtk_tree_store_set (_table_set_treestore, &iter,
                          DisplayColumnId::TABLE_SET_STATUS_str, GTK_STOCK_EXECUTE,
                          -1);
    }
    else
    {
      gtk_tree_store_set (_table_set_treestore, &iter,
                          DisplayColumnId::TABLE_SET_STATUS_str, NULL,
                          -1);
    }

    SignalStatusUpdate ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnTableSetDisplayed (TableSet *table_set,
                                        Table    *from)
  {
    if ((_displayed_table_set == nullptr) || (table_set == _displayed_table_set))
    {
      {
        Table *previous = from->GetLeftTable ();
        Table *next     = from->GetRightTable ();

        gtk_widget_set_sensitive (_glade->GetWidget ("previous_button"),
                                  previous != nullptr);

        gtk_widget_set_sensitive (_glade->GetWidget ("next_button"),
                                  next && (next->_ready_to_fence || next->_is_over));

        gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("from_label")),
                            from->GetMiniName ());
      }

      {
        gchar *name_space = g_strdup_printf ("%d-%d.", table_set->GetFirstPlace (), from->GetSize ());

        gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("search_namespace")),
                            name_space);
        g_free (name_space);
      }

      {
        gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("search_entry")),
                            "");
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnTableOver (TableSet *table_set,
                                Table    *table)
  {
    GtkTreeIter iter;
    GtkTreeIter defeated_iter;

    {
      GtkTreePath *path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) table_set->GetPtrData (this, "tree_row_ref"));

      gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_treestore),
                               &iter,
                               path);
      gtk_tree_path_free (path);
    }

    if (   (table_set->GetNbTables () >= (table->GetNumber () + 3))
        && gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_table_set_treestore),
                                          &defeated_iter,
                                          &iter,
                                          table_set->GetNbTables () - table->GetNumber () - 3))
    {
      TableSet *defeated_table_set;

      gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &defeated_iter,
                          DisplayColumnId::TABLE_SET_TABLE_ptr, &defeated_table_set,
                          -1);

      if (defeated_table_set)
      {
        // Store its reference in the table it comes from
        table->_defeated_table_set = defeated_table_set;

        // Populate it
        {
          GSList *loosers;
          GSList *withdrawals;

          if (table->GetLoosers (&loosers,
                                 &withdrawals,
                                 nullptr) > 0)
          {
            defeated_table_set->SetAttendees (loosers,
                                              withdrawals);
            if (TableSetIsFenced (defeated_table_set))
            {
              ShowTableSet (defeated_table_set,
                            &defeated_iter);
              {
                GtkTreePath *path       = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) defeated_table_set->GetPtrData (this, "tree_row_ref"));
                GtkTreePath *child_path = gtk_tree_model_filter_convert_child_path_to_path (GTK_TREE_MODEL_FILTER (_table_set_filter),
                                                                                            path);

                gtk_tree_view_expand_to_path (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")),
                                              child_path);
                gtk_tree_path_free (child_path);
                gtk_tree_path_free (path);
              }
            }
          }
          else
          {
            defeated_table_set->SetAttendees (nullptr,
                                              withdrawals);
            HideTableSet (defeated_table_set,
                          &defeated_iter);
          }
        }
      }
      else
      {
        HideTableSet (defeated_table_set,
                      &defeated_iter);
      }
    }

    OnTableSetStatusUpdated (table_set);
    _contest->TweetFeeder (table);

    {
      Table *right_table = table->GetRightTable ();

      if (   right_table
          && ((right_table->_is_over == FALSE) || (right_table->GetSize () == 1)))
      {
        _contest->Archive (table->GetMiniName ());
      }
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_table_print_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnPrintTableSet ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPlugged ()
  {
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton")),
                                       FALSE);

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                       FALSE);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnUnPlugged ()
  {
    DeleteTableSets ();
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::ToggleTableSetLock (GtkTreeModel *model,
                                           GtkTreePath  *path,
                                           GtkTreeIter  *iter,
                                           gboolean      data)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);

    if (data)
    {
      table_set->Lock ();
    }
    else
    {
      table_set->UnLock ();
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::DumpToHTML (FILE *file)
  {
    if (_displayed_table_set)
    {
      _displayed_table_set->DumpToHTML (file);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnLocked ()
  {
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                       FALSE);

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            (GtkTreeModelForeachFunc) ToggleTableSetLock,
                            (void *) 1);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnUnLocked ()
  {
    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            (GtkTreeModelForeachFunc) ToggleTableSetLock,
                            nullptr);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnAttrListUpdated ()
  {
    Display ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::FillInConfig ()
  {
    Stage::FillInConfig ();

    if (_fenced_places->GetValue () == ALL_PLACES)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_places_radiobutton")),
                                    TRUE);
    }
    else if (_fenced_places->GetValue () == THIRD_PLACES)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("third_place_radiobutton")),
                                    TRUE);
    }
    else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("no_radiobutton")),
                                    TRUE);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::ApplyConfig ()
  {
    Stage::ApplyConfig ();

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_places_radiobutton"))))
    {
      _fenced_places->SetValue (ALL_PLACES);
    }
    else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("third_place_radiobutton"))))
    {
      _fenced_places->SetValue (THIRD_PLACES);
    }
    else
    {
      _fenced_places->SetValue (NONE);
    }

    OnAttrListUpdated ();
    SetTableSetsState ();

    gtk_widget_set_visible (_glade->GetWidget ("table_set_treeview"),
                            _fenced_places->GetValue () != NONE);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnStuffClicked ()
  {
    _displayed_table_set->OnStuffClicked ();
    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPreviousClicked ()
  {
    _displayed_table_set->OnDisplayPrevious ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnNextClicked ()
  {
    _displayed_table_set->OnDisplayNext ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnInputToggled (GtkWidget *widget)
  {
    if (_displayed_table_set)
    {
      _displayed_table_set->OnInputToggled (widget);
    }
  }

  // --------------------------------------------------------------------------------
  GSList *Supervisor::GetCurrentClassification ()
  {
    _result       = nullptr;
    _blackcardeds = nullptr;

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                            (GtkTreeModelForeachFunc) GetTableSetClassification,
                            this);

    {
      Player::AttributeId attr_id = Player::AttributeId ("rank", this);

      _result = g_slist_sort_with_data (_result,
                                        (GCompareDataFunc) Player::Compare,
                                        &attr_id);
    }

    _result = g_slist_concat (_result,
                              _blackcardeds);
    _blackcardeds = nullptr;

    return _result;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::GetTableSetClassification (GtkTreeModel *model,
                                                  GtkTreePath  *path,
                                                  GtkTreeIter  *iter,
                                                  Supervisor   *ts)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);

    for (GSList *p = table_set->GetCurrentClassification (); p; p = g_slist_next (p))
    {
      Player *player = (Player *) p->data;

      ts->_result = g_slist_remove (ts->_result,
                                    player);
      ts->_result = g_slist_append (ts->_result,
                                    player);
    }

    {
      GSList *blacardeds = table_set->GetBlackcardeds ();

      while (blacardeds)
      {
        Player *player = (Player *) blacardeds->data;

        if (g_slist_find (ts->_blackcardeds, player) == nullptr)
        {
          ts->_blackcardeds = g_slist_prepend (ts->_blackcardeds,
                                               player);
        }
        ts->_result = g_slist_remove (ts->_result,
                                      player);

        blacardeds = g_slist_next (blacardeds);
      }
      g_slist_free (blacardeds);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnTableSetTreeViewCursorChanged (GtkTreeView *treeview)
  {
    GtkTreePath *path;
    GtkTreeIter  iter;
    TableSet    *table_set;

    gtk_tree_view_get_cursor (treeview,
                              &path,
                              nullptr);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_filter),
                             &iter,
                             path);
    gtk_tree_path_free (path);

    gtk_tree_model_get (GTK_TREE_MODEL (_table_set_filter), &iter,
                        DisplayColumnId::TABLE_SET_TABLE_ptr, &table_set,
                        -1);

    if (_displayed_table_set != table_set)
    {
      OnTableSetSelected (table_set);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPrintTableSet ()
  {
    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton"))))
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        gchar *title = g_strdup_printf ("%s - %s", gettext ("Table classification"),
                                        GetName ());

        classification->Print (title);
        g_free (title);
      }
    }
    else if (_displayed_table_set)
    {
      _displayed_table_set->OnPrint ();
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPrintTableScoreSheets ()
  {
    if (_displayed_table_set)
    {
      _displayed_table_set->OnPrintScoreSheets ();
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnSearchMatch (GtkEditable *editable)
  {
    gtk_widget_modify_base (GTK_WIDGET (editable),
                            GTK_STATE_NORMAL,
                            nullptr);

    if (_displayed_table_set)
    {
      gchar *text = gtk_editable_get_chars (editable,
                                            0,
                                            -1);

      if (text && text[0])
      {
        gint number = (gint) g_ascii_strtoll (text,
                                              nullptr,
                                              10);

        if (_displayed_table_set->OnGotoMatch (number) == FALSE)
        {
          GdkColor *color = g_new (GdkColor, 1);

          gdk_color_parse ("#d52323",
                           color);
          gtk_widget_modify_base (GTK_WIDGET (editable),
                                  GTK_STATE_NORMAL,
                                  color);
          g_free (color);
        }
      }

      g_free (text);
    }
  }

  // --------------------------------------------------------------------------------
  gchar *Supervisor::GetPrintName ()
  {
    if (_displayed_table_set)
    {
      return _displayed_table_set->GetPrintName ();
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  GList *Supervisor::GetBookSections (StageView view)
  {
    if (view == StageView::RESULT)
    {
      if (_displayed_table_set)
      {
        return _displayed_table_set->GetBookSections (view);
      }
      return nullptr;
    }

    return Stage::GetBookSections (view);
  }

  // --------------------------------------------------------------------------------
  guint Supervisor::PreparePrint (GtkPrintOperation *operation,
                                  GtkPrintContext   *context)
  {
    if (   (GetStageView (operation) == StageView::CLASSIFICATION)
        || gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton"))))
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        return classification->PreparePrint (operation,
                                             context);
      }
    }
    else if (_displayed_table_set)
    {
      return _displayed_table_set->PreparePrint (operation,
                                                 context);
    }

    return 0;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::DrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr)
  {
    gboolean print_classification = FALSE;

    if (GetStageView (operation) == StageView::UNDEFINED)
    {
      print_classification = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton")));
    }
    if (GetStageView (operation) == StageView::CLASSIFICATION)
    {
      print_classification = TRUE;
    }

    if (print_classification)
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        DrawContainerPage (operation,
                           context,
                           page_nr);
        classification->DrawBarePage (operation,
                                      context,
                                      page_nr);
      }
    }
    else if (_displayed_table_set)
    {
      _displayed_table_set->DrawPage (operation,
                                      context,
                                      page_nr);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::DrawConfig (GtkPrintOperation *operation,
                               GtkPrintContext   *context,
                               gint               page_nr)
  {
    {
      gchar *text;

      if (_fenced_places->GetValue () == ALL_PLACES)
      {
        text = g_strdup_printf ("%s",
                                gettext ("All places fenced"));
      }
      else if (_fenced_places->GetValue () == THIRD_PLACES)
      {
        text = g_strdup_printf ("%s",
                                gettext ("Fence off for third place"));
      }
      else
      {
        text = g_strdup_printf ("%s",
                                gettext ("No fence off for third place"));
      }


      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }

    {
      gchar *text = g_strdup_printf ("%s : %d",
                                     gettext ("Max score"),
                                     _max_score->GetValue ());

      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }

    {
      gchar *text;

      if (_nb_qualified->IsValid ())
      {
        text = g_strdup_printf ("%s : %d",
                                gettext ("Qualified"),
                                _nb_qualified->GetValue ());
      }
      else
      {
        text = g_strdup_printf ("%s : %s",
                                gettext ("Qualified"),
                                "100%");
      }

      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_table_filter_toolbutton_clicked (GtkWidget *widget,
                                                                      Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnFilterClicked ("table_classification_toggletoolbutton");
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_table_classification_toggletoolbutton_toggled (GtkToggleToolButton *widget,
                                                                                    Object              *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->ToggleClassification (gtk_toggle_tool_button_get_active (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_table_stuff_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnStuffClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_input_toolbutton_toggled (GtkWidget *widget,
                                                               Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnInputToggled (widget);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_table_set_treeview_cursor_changed (GtkTreeView *treeview,
                                                                        Object      *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnTableSetTreeViewCursorChanged (treeview);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_previous_button_clicked (GtkWidget *widget,
                                                              Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnPreviousClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_next_button_clicked (GtkWidget *widget,
                                                          Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnNextClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_print_score_sheets_clicked (GtkWidget *widget,
                                                                 Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnPrintTableScoreSheets ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_search_entry_changed (GtkEditable *editable,
                                                           Object      *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnSearchMatch (editable);
  }
}
