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
#include <gdk/gdkkeysyms.h>

#include "util/attribute.hpp"
#include "util/player.hpp"
#include "application/classification.hpp"

#include "table_set.hpp"
#include "table_supervisor.hpp"

namespace Table
{
  const gchar *Supervisor::_class_name     = N_("Table");
  const gchar *Supervisor::_xml_class_name = "PhaseDeTableaux";

  typedef enum
  {
    FROM_NAME_COLUMN,
    FROM_STATUS_COLUMN
  } FromColumnId;

  typedef enum
  {
    TABLE_SET_NAME_COLUMN_str,
    TABLE_SET_TABLE_COLUMN_ptr,
    TABLE_SET_STATUS_COLUMN_str,
    TABLE_SET_VISIBILITY_COLUMN_bool
  } DisplayColumnId;

  // --------------------------------------------------------------------------------
  Supervisor::Supervisor (StageClass *stage_class)
    : Stage (stage_class),
    Module ("table_supervisor.glade")
  {
    _max_score = new Data ("ScoreMax",
                           10);

    _fenced_places = new Data ("PlacesTirees",
                               (guint) NONE);

    _displayed_table_set = NULL;

    {
      AddSensitiveWidget (_glade->GetWidget ("input_toolbutton"));
      AddSensitiveWidget (_glade->GetWidget ("qualified_table"));
      AddSensitiveWidget (_glade->GetWidget ("classification_vbox"));
      AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
      AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

      LockOnClassification (_glade->GetWidget ("from_vbox"));
      LockOnClassification (_glade->GetWidget ("to_vbox"));
      LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
      LockOnClassification (_glade->GetWidget ("input_toolbutton"));
    }

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "IP",
                                          "HS",
                                          "attending",
                                          "availability",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "participation_rate",
                                          "pool_nr",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

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
#endif
                                          "IP",
                                          "HS",
                                          "attending",
                                          "availability",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "participation_rate",
                                          "pool_nr",
                                          "promoted",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

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
                                              TABLE_SET_VISIBILITY_COLUMN_bool);
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
  void Supervisor::OnTableSetSelected (TableSet *table_set)
  {
    if (_displayed_table_set)
    {
      _displayed_table_set->UnPlug ();
      _displayed_table_set = NULL;
    }

    if (table_set)
    {
      Plug (table_set,
            _glade->GetWidget ("table_set_hook"));
      table_set->Display (GTK_RANGE (_glade->GetWidget ("zoom_scale")));

      _displayed_table_set = table_set;
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::IsOver ()
  {
    _is_over     = TRUE;
    _first_error = NULL;

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                            (GtkTreeModelForeachFunc) TableSetIsOver,
                            this);

    return _is_over;
  }

  // --------------------------------------------------------------------------------
  gchar *Supervisor::GetError ()
  {
    if (_first_error)
    {
      gchar *match_name = g_strdup_printf (gettext ("Match %s"), _first_error->GetName ());
      gchar *error      = g_strdup_printf (" <span foreground=\"black\" weight=\"800\">%s:</span> \n "
                                           " <span foreground=\"black\" style=\"italic\" weight=\"400\">\"%s\" </span>",
                                           match_name, gettext ("No winner!"));

      g_free (match_name);
      return error;
    }
    else
    {
      return NULL;
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
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
                        -1);
    if (table_set->GetFirstPlace () == 1)
    {
      ts->_is_over &= table_set->IsOver ();
    }

    if (ts->_first_error == NULL)
    {
      ts->_first_error = table_set->GetFirstError ();
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::CreateTableSets ()
  {
    if (_attendees)
    {
      guint nb_players = g_slist_length (_attendees->GetShortList ());
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
                             NULL);
        }

        {
          GtkTreeIter  iter;
          TableSet    *first_table_set;

          gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                         &iter);
          gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                              TABLE_SET_TABLE_COLUMN_ptr, &first_table_set,
                              -1);

          if (first_table_set)
          {
            first_table_set->SetAttendees (g_slist_copy (_attendees->GetShortList ()));
            _displayed_table_set = first_table_set;
          }
        }
      }
    }

    SetTableSetsState ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::SetTableSetsState ()
  {
    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            (GtkTreeModelForeachFunc) ActivateTableSet,
                            this);
    gtk_tree_view_expand_all (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")));

    if (_displayed_table_set == NULL)
    {
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                         &iter))
      {
        GtkTreePath *path;

        path = gtk_tree_model_get_path (GTK_TREE_MODEL (_table_set_treestore), &iter);

        gtk_tree_view_set_cursor (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")),
                                  path,
                                  NULL,
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
                        TABLE_SET_VISIBILITY_COLUMN_bool, TRUE,
                        -1);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::HideTableSet (TableSet    *table_set,
                                 GtkTreeIter *iter)
  {
    gtk_tree_store_set (_table_set_treestore, iter,
                        TABLE_SET_VISIBILITY_COLUMN_bool, FALSE,
                        -1);

    if (_displayed_table_set && (_displayed_table_set == table_set))
    {
      _displayed_table_set->UnPlug ();
      _displayed_table_set = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::TableSetIsFenced (TableSet *table_set)
  {
    if (table_set->GetFirstPlace () == 1)
    {
      return TRUE;
    }
    else if (_fenced_places->_value == ALL_PLACES)
    {
      return TRUE;
    }
    else if (_fenced_places->_value == THIRD_PLACES)
    {
      if (table_set->GetFirstPlace () <= 3)
      {
        return TRUE;
      }
    }
    else if (_fenced_places->_value == QUOTA)
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
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
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
  TableSet *Supervisor::GetTableSet (gchar *id)
  {
    GtkTreeIter  iter;
    GtkTreePath *path = gtk_tree_path_new_from_string (id);
    TableSet    *table_set = NULL;

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_treestore),
                                 &iter,
                                 path))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                          TABLE_SET_TABLE_COLUMN_ptr, &table_set,
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
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
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
      _displayed_table_set = NULL;
    }

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            DeleteTableSet,
                            NULL);
    gtk_tree_store_clear (_table_set_treestore);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Garnish ()
  {
    DeleteTableSets ();
    CreateTableSets ();
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
        if (strcmp (command, "ScoreSheet") == 0)
        {
          gtk_tree_view_set_cursor (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")),
                                    path,
                                    NULL,
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
        else if (strcmp (command, "Score") == 0)
        {
          GtkTreeIter  iter;
          TableSet    *table_set;

          gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_filter),
                                   &iter,
                                   path);

          gtk_tree_model_get (GTK_TREE_MODEL (_table_set_filter), &iter,
                              TABLE_SET_TABLE_COLUMN_ptr, &table_set,
                              -1);

          result = table_set->OnHttpPost (command,
                                          (const gchar**) &tokens[1],
                                          data);
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
    Stage::LoadConfiguration (xml_node);

    if (_fenced_places)
    {
      _fenced_places->Load (xml_node);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Load (xmlNode *xml_node)
  {
    LoadConfiguration (xml_node);

    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (strcmp ((char *) n->name, _xml_class_name) == 0)
        {
        }
        else if (strcmp ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          LoadAttendees (n);
        }
        else if (strcmp ((char *) n->name, "SuiteDeTableaux") == 0)
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
  void Supervisor::SaveConfiguration (xmlTextWriter *xml_writer)
  {
    Stage::SaveConfiguration (xml_writer);

    if (_fenced_places)
    {
      _fenced_places->Save (xml_writer);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::SaveHeader (xmlTextWriter *xml_writer)
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST _xml_class_name);

    SaveConfiguration (xml_writer);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Save (xmlTextWriter *xml_writer)
  {
    SaveHeader (xml_writer);

    SaveAttendees (xml_writer);

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                            (GtkTreeModelForeachFunc) SaveTableSet,
                            xml_writer);

    xmlTextWriterEndElement (xml_writer);
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::SaveTableSet (GtkTreeModel  *model,
                                     GtkTreePath   *path,
                                     GtkTreeIter   *iter,
                                     xmlTextWriter *xml_writer)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
                        -1);
    table_set->Save (xml_writer);

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
                                _glade->GetWidget ("from_vbox"),
                                _glade->GetWidget ("to_vbox"),
                                from_place);

      {
        GtkTreeRowReference *ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (_table_set_treestore),
                                                               path);
        table_set->SetData (this, "tree_row_ref",
                            ref,
                            (GDestroyNotify) gtk_tree_row_reference_free);
      }

      gtk_tree_path_free (path);

      table_set->SetDataOwner (this);
      table_set->SetStatusCbk ((TableSet::StatusCbk) OnTableSetStatusUpdated,
                               this);

      gtk_tree_store_set (_table_set_treestore, &iter,
                          TABLE_SET_NAME_COLUMN_str,        table_set->GetName (),
                          TABLE_SET_TABLE_COLUMN_ptr,       table_set,
                          TABLE_SET_VISIBILITY_COLUMN_bool, 0,
                          -1);
    }

    {
      guint nb_players   = g_slist_length (_attendees->GetShortList ());
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
  void Supervisor::OnTableSetStatusUpdated (TableSet   *table_set,
                                            Supervisor *ts)
  {
    GtkTreeIter iter;

    {
      GtkTreePath *path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) table_set->GetPtrData (ts, "tree_row_ref"));

      gtk_tree_model_get_iter (GTK_TREE_MODEL (ts->_table_set_treestore),
                               &iter,
                               path);
      gtk_tree_path_free (path);
    }

    if (table_set->IsOver ())
    {
      gtk_tree_store_set (ts->_table_set_treestore, &iter,
                          TABLE_SET_STATUS_COLUMN_str, GTK_STOCK_APPLY,
                          -1);
    }
    else if (table_set->HasError ())
    {
      gtk_tree_store_set (ts->_table_set_treestore, &iter,
                          TABLE_SET_STATUS_COLUMN_str, GTK_STOCK_DIALOG_WARNING,
                          -1);
    }
    else if (table_set->GetNbTables () > 0)
    {
      gtk_tree_store_set (ts->_table_set_treestore, &iter,
                          TABLE_SET_STATUS_COLUMN_str, GTK_STOCK_EXECUTE,
                          -1);
    }
    else
    {
      gtk_tree_store_set (ts->_table_set_treestore, &iter,
                          TABLE_SET_STATUS_COLUMN_str, NULL,
                          -1);
    }

    ts->SignalStatusUpdate ();
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
                          TABLE_SET_TABLE_COLUMN_ptr, &defeated_table_set,
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
                                 NULL) > 0)
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
            defeated_table_set->SetAttendees (NULL,
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

    OnTableSetStatusUpdated (table_set,
                             this);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_table_print_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnPrint ();
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
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
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
    DisableSensitiveWidgets ();

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                       FALSE);

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            (GtkTreeModelForeachFunc) ToggleTableSetLock,
                            (void *) 1);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnUnLocked ()
  {
    EnableSensitiveWidgets ();

    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            (GtkTreeModelForeachFunc) ToggleTableSetLock,
                            0);
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

    if (_fenced_places->_value == ALL_PLACES)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_places_radiobutton")),
                                    TRUE);
    }
    else if (_fenced_places->_value == THIRD_PLACES)
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
      _fenced_places->_value = ALL_PLACES;
    }
    else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("third_place_radiobutton"))))
    {
      _fenced_places->_value = THIRD_PLACES;
    }
    else
    {
      _fenced_places->_value = NONE;
    }

    OnAttrListUpdated ();
    SetTableSetsState ();
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::StuffTableSet (GtkTreeModel *model,
                                      GtkTreePath  *path,
                                      GtkTreeIter  *iter,
                                      gpointer      data)
  {
    TableSet *table_set;

    gtk_tree_model_get (model, iter,
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
                        -1);

    table_set->OnStuffClicked ();

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnStuffClicked ()
  {
    gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                            StuffTableSet,
                            NULL);
    OnAttrListUpdated ();
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
  void Supervisor::OnDisplayToggled (GtkWidget *widget)
  {
    GtkWidget *vbox = _glade->GetWidget ("display_vbox");

    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    {
      gtk_widget_show (vbox);
    }
    else
    {
      gtk_widget_hide (vbox);
    }
  }

  // --------------------------------------------------------------------------------
  GSList *Supervisor::GetCurrentClassification ()
  {
    _result       = NULL;
    _blackcardeds = NULL;

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
    _blackcardeds = NULL;

    return _result;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::GetTableSetClassification (GtkTreeModel *model,
                                                  GtkTreePath  *path,
                                                  GtkTreeIter  *iter,
                                                  Supervisor   *ts)
  {
    TableSet *table_set;
    GSList   *current_result;

    gtk_tree_model_get (model, iter,
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
                        -1);
    current_result = table_set->GetCurrentClassification ();

    while (current_result)
    {
      Player *player = (Player *) current_result->data;

      ts->_result = g_slist_remove (ts->_result,
                                    player);
      ts->_result = g_slist_append (ts->_result,
                                    player);

      current_result = g_slist_next (current_result);
    }

    {
      GSList *blacardeds = table_set->GetBlackcardeds ();

      while (blacardeds)
      {
        Player *player = (Player *) blacardeds->data;

        if (g_slist_find (ts->_blackcardeds, player) == NULL)
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
                              NULL);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_filter),
                             &iter,
                             path);
    gtk_tree_path_free (path);

    gtk_tree_model_get (GTK_TREE_MODEL (_table_set_filter), &iter,
                        TABLE_SET_TABLE_COLUMN_ptr, &table_set,
                        -1);

    if (_displayed_table_set != table_set)
    {
      OnTableSetSelected (table_set);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPrint ()
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
  gchar *Supervisor::GetPrintName ()
  {
    if (_displayed_table_set)
    {
      return _displayed_table_set->GetPrintName ();
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  guint Supervisor::PreparePrint (GtkPrintOperation *operation,
                                  GtkPrintContext   *context)
  {
    if (GetStageView (operation) == STAGE_VIEW_RESULT)
    {
      return 0;
    }
    else if (   (GetStageView (operation) == STAGE_VIEW_CLASSIFICATION)
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

    if (GetStageView (operation) == STAGE_VIEW_UNDEFINED)
    {
      print_classification = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton")));
    }
    if (GetStageView (operation) == Stage::STAGE_VIEW_CLASSIFICATION)
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

      if (_fenced_places->_value == ALL_PLACES)
      {
        text = g_strdup_printf ("%s : %s",
                                gettext ("Fenced places"),
                                gettext ("All"));
      }
      else if (_fenced_places->_value == THIRD_PLACES)
      {
        text = g_strdup_printf ("%s : %s",
                                gettext ("Fenced places"),
                                gettext ("Third place"));
      }
      else
      {
        text = g_strdup_printf ("%s : %s",
                                gettext ("Fenced places"),
                                gettext ("None"));
      }


      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }

    {
      gchar *text = g_strdup_printf ("%s : %d",
                                     gettext ("Max score"),
                                     _max_score->_value);

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
                                _nb_qualified->_value);
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
  extern "C" G_MODULE_EXPORT void on_display_toolbutton_toggled (GtkWidget *widget,
                                                                 Object    *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnDisplayToggled (widget);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_table_set_treeview_cursor_changed (GtkTreeView *treeview,
                                                                        Object      *owner)
  {
    Supervisor *t = dynamic_cast <Supervisor *> (owner);

    t->OnTableSetTreeViewCursorChanged (treeview);
  }
}
