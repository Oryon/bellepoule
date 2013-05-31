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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <gtk/gtk.h>

#include "people_management/general_classification.hpp"
#include "people_management/checkin.hpp"
#include "people_management/checkin_supervisor.hpp"

#include "chapter.hpp"
#include "contest.hpp"

#include "schedule.hpp"

typedef enum
{
  NAME_str,
  LOCK_str,
  STAGE_ptr,
  VISIBILITY_bool
} ColumnId;

// --------------------------------------------------------------------------------
Schedule::Schedule (Contest *contest)
: Module ("schedule.glade",
          "schedule_notebook")
{
  _stage_list    = NULL;
  _current_stage = 0;
  _contest       = contest;

  _score_stuffing_allowed = FALSE;

  {
    _list_store        = GTK_LIST_STORE (_glade->GetGObject ("stage_liststore"));
    _list_store_filter = GTK_TREE_MODEL_FILTER (_glade->GetGObject ("stage_liststore_filter"));

    gtk_tree_model_filter_set_visible_column (_list_store_filter,
                                              VISIBILITY_bool);
  }

  // Error bg
  {
    GdkColor *color = g_new (GdkColor, 1);

    gdk_color_parse ("#c52222", color);

    gtk_widget_modify_bg (_glade->GetWidget ("error_viewport"),
                          GTK_STATE_NORMAL,
                          color);
    g_free (color);
  }

  // Formula dialog
  {
    GtkWidget *menu_pool = gtk_menu_new ();

    gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (_glade->GetWidget ("add_stage_toolbutton")),
                                   menu_pool);

    for (guint i = 0; i < Stage::GetNbStageClass (); i++)
    {
      Stage::StageClass *stage_class;
      GtkWidget         *menu_item;

      stage_class = Stage::GetClass (i);

      if (   (stage_class->_rights & Stage::EDITABLE)
          && (stage_class->_rights & Stage::REMOVABLE))
      {
        menu_item = gtk_menu_item_new_with_label (stage_class->_name);
        g_signal_connect (menu_item, "button-release-event",
                          G_CALLBACK (on_new_stage_selected), this);
        g_object_set_data (G_OBJECT (menu_item),
                           "Schedule::class_name",
                           (void *) stage_class->_xml_name);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu_pool),
                               menu_item);
        gtk_widget_show (menu_item);
      }
    }
  }
}

// --------------------------------------------------------------------------------
Schedule::~Schedule ()
{
  RemoveAllStages ();
}

// --------------------------------------------------------------------------------
People::CheckinSupervisor *Schedule::GetCheckinSupervisor ()
{
  return dynamic_cast <People::CheckinSupervisor *> (GetStage (0));
}

// --------------------------------------------------------------------------------
Stage *Schedule::CreateStage (const gchar *class_name)
{
  Stage *stage = Stage::CreateInstance (class_name);

  if (stage && (g_ascii_strcasecmp (class_name, "checkin_stage") == 0))
  {
    Module *module = dynamic_cast <Module *> (stage);

    if (module)
    {
      module->AddSensitiveWidget (GTK_WIDGET (_contest->GetPtrData (NULL,
                                                                    "SensitiveWidgetForCheckinStage")));
    }
  }

  return stage;
}

// --------------------------------------------------------------------------------
void Schedule::Freeze ()
{
  {
    gtk_widget_hide (_glade->GetWidget ("previous_stage_toolbutton"));
    gtk_widget_hide (_glade->GetWidget ("next_stage_toolbutton"));
  }

  gtk_widget_set_sensitive (_glade->GetWidget ("config_toolbar"),
                            FALSE);
  gtk_widget_set_sensitive (_glade->GetWidget ("module_config_hook"),
                            FALSE);
}

// --------------------------------------------------------------------------------
Stage *Schedule::GetStage (guint index)
{
  return (Stage *) g_list_nth_data (_stage_list,
                                    index);
}

// --------------------------------------------------------------------------------
void Schedule::GiveName (Stage *stage)
{
  Stage::StageClass *stage_class = stage->GetClass ();

  if (stage_class && (g_ascii_strcasecmp  (stage_class->_xml_name, "pool_stage") == 0))
  {
    GList *current     = _stage_list;
    guint  round_index = 0;

    while (current)
    {
      Stage             *current_stage       = (Stage *) current->data;
      Stage::StageClass *current_stage_class = current_stage->GetClass ();

      if (g_ascii_strcasecmp  (current_stage_class->_xml_name, "pool_stage") == 0)
      {
        round_index++;
      }

      if (current_stage == stage)
      {
        gchar *name = g_strdup_printf (gettext ("Round #%d"), round_index);

        stage->SetName (name);
        g_free (name);
        break;
      }

      current = g_list_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::CreateDefault (gboolean without_pools)
{
  if (_stage_list == NULL)
  {
    Stage *stage;

    stage = CreateStage ("checkin_stage");
    if (stage)
    {
      AddStage (stage);
      {
        PlugStage (stage);
        stage->RetrieveAttendees ();
        stage->Garnish ();
        stage->Display ();
      }
    }

    if (without_pools == FALSE)
    {
      stage = CreateStage ("pool_stage");
      if (stage)
      {
        AddStage (stage);
        GiveName (stage);
      }
    }

    stage = CreateStage ("PhaseDeTableaux");
    if (stage)
    {
      AddStage (stage);
    }

    stage = CreateStage ("ClassementGeneral");
    if (stage)
    {
      AddStage (stage);
    }
    SetCurrentStage (0);
  }

  DisplayConfig ();
}

// --------------------------------------------------------------------------------
void Schedule::SetScoreStuffingPolicy (gboolean allowed)
{
  _score_stuffing_allowed = allowed;

  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Stage *current_stage;

    current_stage = (Stage *) g_list_nth_data (_stage_list,
                                               i);
    current_stage->SetScoreStuffingPolicy (_score_stuffing_allowed);
  }
}

// --------------------------------------------------------------------------------
void Schedule::SetTeamEvent (gboolean team_event)
{
  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Stage *current_stage;

    current_stage = (Stage *) g_list_nth_data (_stage_list,
                                               i);
    current_stage->SetTeamEvent (team_event);
  }
}

// --------------------------------------------------------------------------------
gboolean Schedule::ScoreStuffingIsAllowed ()
{
  return _score_stuffing_allowed;
}

// --------------------------------------------------------------------------------
void Schedule::DisplayConfig ()
{
  gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("add_stage_toolbutton")),
                            TRUE);

  if (_stage_list)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_list_store),
                                                   &iter);
    while (iter_is_valid)
    {
      Stage *current_stage;

      gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                          &iter,
                          STAGE_ptr, &current_stage,
                          -1);
      if (current_stage->Locked ())
      {
        gtk_list_store_set (_list_store, &iter,
                            LOCK_str, GTK_STOCK_DIALOG_AUTHENTICATION,
                            -1);
      }
      else
      {
        gtk_list_store_set (_list_store, &iter,
                            LOCK_str, "",
                            -1);
      }

      current_stage->FillInConfig ();

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_list_store),
                                                &iter);
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::ApplyNewConfig ()
{
  Stage *current_stage;

  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    current_stage = (Stage *) g_list_nth_data (_stage_list,
                                               i);
    current_stage->ApplyConfig ();
  }

  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    current_stage = (Stage *) g_list_nth_data (_stage_list,
                                               i);
    RefreshStageName (current_stage);
  }

  MakeDirty ();
}

// --------------------------------------------------------------------------------
void Schedule::RefreshStageName (Stage *stage)
{
  GtkWidget *tab_widget = GTK_WIDGET (stage->GetPtrData (this, "viewport_stage"));

  if (tab_widget)
  {
    gchar *name = stage->GetFullName ();

    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (GetRootWidget ()),
                                     tab_widget,
                                     name);
    g_free (name);
  }
}

// --------------------------------------------------------------------------------
Module *Schedule::GetSelectedModule  ()
{
  GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  GtkTreeIter       filter_iter;

  if (gtk_tree_selection_get_selected (selection,
                                       NULL,
                                       &filter_iter))
  {
    Stage       *stage;
    GtkTreeIter  iter;

    gtk_tree_model_filter_convert_iter_to_child_iter (_list_store_filter,
                                                      &iter,
                                                      &filter_iter);
    gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                        &iter,
                        STAGE_ptr, &stage,
                        -1);

    return dynamic_cast <Module *> (stage);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Schedule::AddStage (Stage *stage)
{
  Stage *last_stage = NULL;

  if (_stage_list)
  {
    last_stage = (Stage *) (g_list_last (_stage_list)->data);
  }

  AddStage (stage,
            last_stage);

  {
    Stage *input_provider = stage->GetInputProvider ();

    if (input_provider)
    {
      if (input_provider && (last_stage != input_provider))
      {
        AddStage (input_provider,
                  last_stage);
      }
      stage->SetInputProvider (input_provider);
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::AddStage (Stage *stage,
                         Stage *after)
{
  if ((stage == NULL) || g_list_find (_stage_list, stage))
  {
    return;
  }
  else
  {
    stage->SetContest (_contest);
    stage->SetScoreStuffingPolicy (_score_stuffing_allowed);

    // Insert it in the global list
    {
      Stage *next = NULL;

      if (_stage_list == NULL)
      {
        _stage_list = g_list_append (_stage_list,
                                     stage);
      }
      else
      {
        guint index;

        if (after == NULL)
        {
          after = (Stage *) g_list_nth_data (_stage_list,
                                             0);
        }

        index = g_list_index (_stage_list,
                              after) + 1;
        next = (Stage *) g_list_nth_data (_stage_list,
                                          index);
        _stage_list = g_list_insert (_stage_list,
                                     stage,
                                     index);
      }

      stage->SetPrevious (after);
      if (next)
      {
        next->SetPrevious (stage);
      }
    }

    // Insert it in the formula list
    {
      GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
      GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
      GtkTreeIter       iter;

      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_list_store),
                                         &iter) == TRUE)
      {
        do
        {
          Stage *current_stage;

          gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                              &iter,
                              STAGE_ptr, &current_stage,
                              -1);

          if (current_stage == after)
          {
            gtk_list_store_insert_after (_list_store,
                                         &iter,
                                         &iter);
            break;
          }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (_list_store),
                                           &iter) == TRUE);
      }
      else
      {
        gtk_list_store_append (_list_store,
                               &iter);
      }

      gtk_list_store_set (_list_store, &iter,
                          NAME_str, stage->GetClassName (),
                          STAGE_ptr, stage,
                          VISIBILITY_bool, (stage->GetRights () & Stage::EDITABLE) != 0,
                          -1);

      {
        GtkTreeIter filter_iter;

        if (gtk_tree_model_filter_convert_child_iter_to_iter (_list_store_filter,
                                                              &filter_iter,
                                                              &iter))
        {
          gtk_tree_selection_select_iter (selection,
                                          &filter_iter);
        }
      }
    }

    RefreshSensitivity ();

#if 0
    for (guint i = 0; i < g_list_length (_stage_list); i++)
    {
      Stage *stage;
      Stage *previous;

      stage = ((Stage *) g_list_nth_data (_stage_list,
                                            i));
      previous = stage->GetPreviousStage ();
      g_print (">> %s", stage->GetClassName ());
      if (previous)
      {
      g_print (" / %s\n", previous->GetClassName ());
      }
      else
      {
      g_print ("\n");
      }
    }
    g_print ("\n");
#endif
  }
}

// --------------------------------------------------------------------------------
void Schedule::RemoveAllStages ()
{
  GList *current = g_list_last (_stage_list);

  while (current)
  {
    Stage *stage = (Stage *) current->data;

    RemoveStage (stage);

    current = g_list_last (_stage_list);
  }
}

// --------------------------------------------------------------------------------
void Schedule::RemoveStage (Stage *stage)
{
  if (stage)
  {
    GList *current       = g_list_find (_stage_list, stage);
    gint   current_index = g_list_index (_stage_list, stage);
    GList *next          = g_list_next (current);

    if (next)
    {
      Stage *next_stage = (Stage *) (next->data);

      if (next_stage)
      {
        Stage *previous_stage = NULL;
        GList *previous       = g_list_previous (current);

        if (previous)
        {
          previous_stage = (Stage *) (g_list_previous (current)->data);
        }
        next_stage->SetPrevious (previous_stage);
      }
    }

    {
      guint n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (GetRootWidget ()));

      if (   (n_pages > 0)
          && (_current_stage > 0)
          && (_current_stage == (guint) current_index))
      {
        {
          Module *module = (Module *) dynamic_cast <Module *> (stage);

          stage->Wipe ();
          module->UnPlug ();
        }

        SetCurrentStage (_current_stage-1);

        {
          Stage *new_current_stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);

          new_current_stage->UnLock ();
        }
      }
    }

    _stage_list = g_list_remove (_stage_list,
                                 stage);

    RemoveFromNotebook (stage);

    stage->Release ();
  }

  {
    GtkTreeIter  iter;
    gboolean     iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_list_store),
                                                   &iter);
    while (iter_is_valid)
    {
      Stage *current_stage;

      gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                          &iter,
                          STAGE_ptr, &current_stage,
                          -1);
      if (current_stage == stage)
      {
        {
          GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
          GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
          GtkTreeIter       filter_iter;

          if (gtk_tree_model_filter_convert_child_iter_to_iter (_list_store_filter,
                                                                &filter_iter,
                                                                &iter))
          {
            if (gtk_tree_model_iter_next (GTK_TREE_MODEL (_list_store_filter),
                                          &filter_iter))
            {
              gtk_tree_selection_select_iter (selection,
                                              &filter_iter);
              on_stage_selected ();
            }
          }
        }

        gtk_list_store_remove (_list_store,
                               &iter);
        break;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_list_store),
                                                &iter);
    }
  }
  RefreshSensitivity ();
}

// --------------------------------------------------------------------------------
void Schedule::RemoveFromNotebook (Stage *stage)
{
  gint page = GetNotebookPageNum (stage);

  if (page >= 0)
  {
    gtk_notebook_remove_page (GTK_NOTEBOOK (GetRootWidget ()),
                              GetNotebookPageNum (stage));
    stage->RemoveData (this,
                       "viewport_stage");
  }
}

// --------------------------------------------------------------------------------
void Schedule::SavePeoples (xmlTextWriter   *xml_writer,
                            People::Checkin *referees)
{
  Stage *stage = ((Stage *) g_list_nth_data (_stage_list,
                                             0));

  if (stage)
  {
    // Checkin - Player list
    stage->Save (xml_writer);

    // Referees
    referees->SaveList (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void Schedule::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "Phases");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "PhaseEnCours",
                                     "%d", _current_stage);

  for (guint i = 1; i < g_list_length (_stage_list); i++)
  {
    Stage *stage = ((Stage *) g_list_nth_data (_stage_list,
                                               i));
    stage->SetId (i);
    stage->Save (xml_writer);
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Schedule::Load (xmlDoc          *doc,
                     const gchar     *contest_keyword,
                     People::Checkin *referees)
{
  xmlXPathContext *xml_context         = xmlXPathNewContext (doc);
  gint             current_stage_index = -1;
  gboolean         display_all         = FALSE;
  Stage           *checkin_stage       = CreateStage ("checkin_stage");

  {
    xmlNodeSet     *xml_nodeset;
    gchar          *xml_object_path = g_strdup_printf ("/%s/Phases", contest_keyword);
    xmlXPathObject *xml_object      = xmlXPathEval (BAD_CAST xml_object_path, xml_context);

    g_free (xml_object_path);

    xml_nodeset = xml_object->nodesetval;
    if (xml_object->nodesetval->nodeNr)
    {
      char *attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0],
                                         BAD_CAST "PhaseEnCours");

      if (attr)
      {
        current_stage_index = atoi (attr);
        xmlFree (attr);
      }
      else
      {
        display_all = TRUE;
      }
    }

    xmlXPathFreeObject  (xml_object);
  }

  gtk_widget_show_all (GetRootWidget ());

  // Checkin - Player list
  if (checkin_stage)
  {
    AddStage  (checkin_stage);
    PlugStage (checkin_stage);

    checkin_stage->Load (xml_context,
                         contest_keyword);
    checkin_stage->Display ();

    if (display_all || (current_stage_index > 0))
    {
      checkin_stage->Lock ();
    }
  }

  referees->LoadList (xml_context,
                      contest_keyword);

  {
    gchar          *path        = g_strdup_printf ("/%s/Phases/*", contest_keyword);
    xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST path, xml_context);
    xmlNodeSet     *xml_nodeset = xml_object->nodesetval;
    guint           nb_stage    = 1;

    g_free (path);
    for (guint i = 0; i < (guint) xml_nodeset->nodeNr; i++)
    {
      Stage *stage = Stage::CreateInstance (xml_nodeset->nodeTab[i]);

      LoadStage (stage,
                 xml_nodeset->nodeTab[i],
                 &nb_stage,
                 current_stage_index);
    }

    xmlXPathFreeObject (xml_object);

    // Add a general classification stage
    // for xml files generated with other
    // tools than BellePoule
    if (nb_stage > 0)
    {
      if (dynamic_cast <People::GeneralClassification *> (GetStage (nb_stage-1)) == NULL)
      {
        Stage *stage;

        current_stage_index = nb_stage-1;
        if (nb_stage == 1)
        {
          stage = CreateStage ("pool_stage");
          if (stage)
          {
            AddStage (stage);
            GiveName (stage);
          }

          stage = CreateStage ("PhaseDeTableaux");
          if (stage)
          {
            AddStage (stage);
          }
        }

        stage = CreateStage ("ClassementGeneral");
        if (stage)
        {
          AddStage (stage);
        }
      }
    }
  }

  xmlXPathFreeContext (xml_context);

  SetCurrentStage (current_stage_index);
  DisplayConfig ();
}

// --------------------------------------------------------------------------------
void Schedule::LoadStage (Stage   *stage,
                          xmlNode *xml_node,
                          guint   *nb_stage,
                          gint     current_stage_index)
{
  if (stage)
  {
    gboolean display_all = (current_stage_index < 0);

    AddStage (stage);
    *nb_stage = *nb_stage + 1;

    if (display_all || ((*nb_stage-1) <= (guint) current_stage_index))
    {
      PlugStage (stage);
    }

    stage->Load (xml_node);
    RefreshStageName (stage);

    if (display_all || ((*nb_stage-1) <= (guint) current_stage_index))
    {
      stage->Display ();

      if (display_all || ((*nb_stage-1) < (guint) current_stage_index))
      {
        stage->Lock ();
      }
    }

    {
      Stage *input_provider_client = CreateStage (stage->GetInputProviderClient ());

      if (input_provider_client)
      {
        LoadStage (input_provider_client,
                   NULL,
                   nb_stage,
                   current_stage_index);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::OnLoadingCompleted ()
{
  GList *current = _stage_list;

  while (current)
  {
    Stage *stage = (Stage *) current->data;

    stage->OnLoadingCompleted ();
    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Schedule::SetCurrentStage (guint index)
{
  _current_stage = index;

  gtk_notebook_set_current_page  (GTK_NOTEBOOK (GetRootWidget ()),
                                  _current_stage);

  {
    Stage *stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);

    if (stage)
    {
      stage->SetStatusCbk ((Stage::StatusCbk) OnStageStatusUpdated,
                           this);
    }
  }

  RefreshSensitivity ();
}

// --------------------------------------------------------------------------------
void Schedule::OnStageStatusUpdated (Stage    *stage,
                                     Schedule *schedule)
{
  schedule->RefreshSensitivity ();
}

// --------------------------------------------------------------------------------
void Schedule::RefreshSensitivity ()
{
  gtk_widget_set_sensitive (_glade->GetWidget ("next_stage_toolbutton"),
                            FALSE);
  gtk_widget_set_sensitive (_glade->GetWidget ("previous_stage_toolbutton"),
                            FALSE);
  if (_current_stage > 0)
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("previous_stage_toolbutton"),
                              TRUE);
  }

  gtk_widget_hide (_glade->GetWidget ("error_toolbutton"));

  if (_stage_list
      && (_current_stage < g_list_length (_stage_list) - 1))
  {
    Stage *stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);

    if (stage->IsOver ())
    {
      gtk_widget_set_sensitive (_glade->GetWidget ("next_stage_toolbutton"),
                                TRUE);
    }
    else
    {
      gchar *error = stage->GetError ();

      if (error)
      {
        gtk_label_set_markup (GTK_LABEL (_glade->GetWidget ("error_label")),
                              error);
        gtk_widget_show (_glade->GetWidget ("error_toolbutton"));

        g_free (error);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::OnPlugged ()
{
  GtkWidget *w;

  {
    GtkToolbar *toolbar = GetToolbar ();

    w = _glade->GetWidget ("previous_stage_toolbutton");
    _glade->DetachFromParent (w);
    gtk_toolbar_insert (toolbar,
                        GTK_TOOL_ITEM (w),
                        -1);

    w = _glade->GetWidget ("next_stage_toolbutton");
    _glade->DetachFromParent (w);
    gtk_toolbar_insert (toolbar,
                        GTK_TOOL_ITEM (w),
                        -1);

    w = _glade->GetWidget ("error_toolbutton");
    _glade->DetachFromParent (w);
    gtk_toolbar_insert (toolbar,
                        GTK_TOOL_ITEM (w),
                        -1);
  }

  {
    GtkContainer *config_container = GetConfigContainer ();

    w = _glade->GetWidget ("config_vbox");
    _glade->DetachFromParent (w);
    gtk_container_add (config_container,
                       w);
  }
}

// --------------------------------------------------------------------------------
gboolean Schedule::on_new_stage_selected (GtkWidget      *widget,
                                          GdkEventButton *event,
                                          Schedule       *owner)
{
  gchar *class_name = (gchar *) g_object_get_data (G_OBJECT (widget),
                                                   "Schedule::class_name");
  Stage *stage = owner->CreateStage (class_name);
  Stage *after = NULL;

  {
    GtkWidget        *treeview  = owner->_glade->GetWidget ("formula_treeview");
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    GtkTreeIter       filter_iter;

    if (gtk_tree_selection_get_selected (selection,
                                         NULL,
                                         &filter_iter))
    {
      GtkTreeIter iter;

      gtk_tree_model_filter_convert_iter_to_child_iter (owner->_list_store_filter,
                                                        &iter,
                                                        &filter_iter);
      gtk_tree_model_get (GTK_TREE_MODEL (owner->_list_store),
                          &iter,
                          STAGE_ptr, &after,
                          -1);
    }
  }

  {
    Stage *input_provider = stage->GetInputProvider ();

    if (input_provider)
    {
      owner->AddStage (input_provider,
                       after);
      stage->SetInputProvider (input_provider);
      after = input_provider;
    }
  }

  owner->AddStage (stage,
                   after);
  owner->GiveName (stage);

  stage->FillInConfig ();
  owner->on_stage_selected ();
  return FALSE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_formula_treeview_cursor_changed (GtkWidget *widget,
                                                                    Object    *owner)
{
  Schedule *s = dynamic_cast <Schedule *> (owner);

  s->on_stage_selected ();
}

// --------------------------------------------------------------------------------
void Schedule::on_stage_selected ()
{
  {
    GtkContainer *container   = GTK_CONTAINER (_glade->GetWidget ("module_config_hook"));
    GList        *widget_list = gtk_container_get_children (container);

    if (widget_list)
    {
      GtkWidget *config_widget = (GtkWidget *) widget_list->data;

      if (config_widget)
      {
        gtk_container_remove (container,
                              config_widget);
      }
    }
  }

  {
    Module *selected_module = GetSelectedModule ();

    if (selected_module)
    {
      Stage     *stage    = dynamic_cast <Stage *> (selected_module);
      GtkWidget *config_w = selected_module->GetConfigWidget ();

      if (config_w)
      {
        gtk_container_add (GTK_CONTAINER (_glade->GetWidget ("module_config_hook")),
                           config_w);
      }

      gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("add_stage_toolbutton")),
                                TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("remove_stage_toolbutton")),
                                TRUE);

      if (stage->Locked ())
      {
        GList *list = g_list_find (_stage_list, stage);
        GList *next = g_list_next (list);

        if (next)
        {
          Stage *selected_stage = (Stage *) list->data;
          Stage *next_stage     = (Stage *) next->data;

          if (selected_stage->Locked () || next_stage->Locked ())
          {
            gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("add_stage_toolbutton")),
                                      FALSE);
          }
        }

        gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("remove_stage_toolbutton")),
                                  FALSE);
      }
    }
    else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("add_stage_toolbutton")),
                                TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("remove_stage_toolbutton")),
                                FALSE);
    }
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_previous_stage_toolbutton_clicked (GtkWidget *widget,
                                                                      Object    *owner)
{
  Schedule *s = dynamic_cast <Schedule *> (owner);

  s->on_previous_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
gint Schedule::GetNotebookPageNum (Stage *stage)
{
  GtkWidget *viewport = (GtkWidget *) stage->GetPtrData (this, "viewport_stage");

  return gtk_notebook_page_num (GTK_NOTEBOOK (GetRootWidget ()),
                                viewport);
}

// --------------------------------------------------------------------------------
void Schedule::on_previous_stage_toolbutton_clicked ()
{
  GtkWidget *dialog = gtk_message_dialog_new_with_markup (NULL,
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          gettext ("<b><big>Do you really want to cancel the current round?</big></b>"));

  gtk_window_set_title (GTK_WINDOW (dialog),
                        gettext ("Go back to the previous round?"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            gettext ("All the inputs of this round will be cancelled."));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    {
      Stage  *stage  = (Stage *) g_list_nth_data (_stage_list, _current_stage);
      Module *module = (Module *) dynamic_cast <Module *> (stage);

      stage->Wipe ();
      module->UnPlug ();

      RemoveFromNotebook (stage);
    }

    {
      Stage *stage;

      SetCurrentStage (_current_stage-1);

      stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);
      stage->UnLock ();
    }

    MakeDirty ();
  }
  gtk_widget_destroy (dialog);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_next_stage_toolbutton_clicked (GtkWidget *widget,
                                                                  Object    *owner)
{
  Schedule *s = dynamic_cast <Schedule *> (owner);

  s->on_next_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Schedule::on_next_stage_toolbutton_clicked ()
{
  Stage *stage;

  stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);
  stage->Lock ();

  stage = (Stage *) g_list_nth_data (_stage_list, _current_stage+1);
  PlugStage (stage);
  stage->RetrieveAttendees ();
  stage->Garnish ();
  stage->Display ();

  SetCurrentStage (_current_stage+1);
  MakeDirty ();
}

// --------------------------------------------------------------------------------
void Schedule::PlugStage (Stage *stage)
{
  Module    *module   = (Module *) dynamic_cast <Module *> (stage);
  GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
  gchar     *name     = stage->GetFullName ();

  stage->SetData (this, "viewport_stage",
                  viewport);
  gtk_notebook_append_page (GTK_NOTEBOOK (GetRootWidget ()),
                            viewport,
                            gtk_label_new (name));
  g_free (name);

  {
    Stage *previous = stage->GetPreviousStage ();

    if (previous)
    {
      stage->SetRandSeed (previous->GetRandSeed ());
    }
  }

  Plug (module,
        viewport);

  gtk_widget_show_all (viewport);
}

// --------------------------------------------------------------------------------
void Schedule::on_stage_removed ()
{
  {
    GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    GtkTreeIter       filter_iter;

    if (gtk_tree_selection_get_selected (selection,
                                         NULL,
                                         &filter_iter))
    {
      Stage             *stage;
      Stage::StageClass *stage_class;
      GtkTreeIter        iter;

      gtk_tree_model_filter_convert_iter_to_child_iter (_list_store_filter,
                                                        &iter,
                                                        &filter_iter);
      gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                          &iter,
                          STAGE_ptr, &stage,
                          -1);

      stage_class = stage->GetClass ();
      if (stage_class->_rights & Stage::REMOVABLE)
      {
        Stage *input_provider = stage->GetInputProvider ();

        RemoveStage (stage);
        RemoveStage (input_provider);

        {
          GtkContainer *container   = GTK_CONTAINER (_glade->GetWidget ("module_config_hook"));
          GList        *widget_list = gtk_container_get_children (container);

          if (widget_list)
          {
            GtkWidget *config_widget = (GtkWidget *) widget_list->data;

            if (config_widget)
            {
              gtk_container_remove (container,
                                    config_widget);
            }
          }
        }
      }
    }

    // Select the last item if the focus is lost
    if (gtk_tree_selection_get_selected (selection,
                                         NULL,
                                         &filter_iter) == FALSE)
    {
      guint n = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (_list_store_filter),
                                                NULL);

      if (n)
      {
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_list_store_filter),
                                       &filter_iter,
                                       NULL,
                                       n-1);
        gtk_tree_selection_select_iter (selection,
                                        &filter_iter);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::PrepareBookPrint (GtkPrintOperation *operation,
                                 GtkPrintContext   *context)
{
  _book = new Book ();
  _book->Prepare (operation,
                  context,
                  _stage_list);
}

// --------------------------------------------------------------------------------
void Schedule::DrawBookPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr)
{
  _book->Print (operation,
                context,
                page_nr);
}

// --------------------------------------------------------------------------------
void Schedule::StopBookPrint (GtkPrintOperation *operation,
                              GtkPrintContext   *context)
{
  _book->Stop (operation,
               context);
  _book->Release ();
  _book = NULL;

}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_stage_toolbutton_clicked (GtkWidget *widget,
                                                                    Object    *owner)
{
  Schedule *s = dynamic_cast <Schedule *> (owner);

  s->on_stage_removed ();
}
