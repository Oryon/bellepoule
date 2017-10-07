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

#include "util/canvas.hpp"
#include "network/advertiser.hpp"
#include "actors/referees_list.hpp"
#include "rounds/classification/general_classification.hpp"
#include "rounds/checkin/checkin_supervisor.hpp"

#include "book/book.hpp"
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
Schedule::Schedule (Contest *contest,
                    GList   *advertisers,
                    Data    *minimum_team_size,
                    Data    *manual_classification,
                    Data    *default_classification)
  : Object ("Schedule"),
    Module ("schedule.glade", "schedule_notebook")
{
   _stage_list    = NULL;
   _current_stage = 0;
   _contest       = contest;
   _advertisers   = advertisers;

  _minimum_team_size      = minimum_team_size;
  _manual_classification  = manual_classification;
  _default_classification = default_classification;
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
  gtk_list_store_clear (_list_store);
  FreeFullGList (Stage, _stage_list);
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
    {
      Module *module = dynamic_cast <Module *> (stage);

      if (module)
      {
        module->AddSensitiveWidget (GTK_WIDGET (_contest->GetPtrData (NULL,
                                                                      "SensitiveWidgetForCheckinStage")));
      }
    }

    {
      People::CheckinSupervisor *checkin = dynamic_cast <People::CheckinSupervisor *> (stage);

      checkin->SetTeamData (_minimum_team_size,
                            _default_classification,
                            _manual_classification);
    }
  }

  return stage;
}

// --------------------------------------------------------------------------------
gboolean Schedule::IsOver ()
{
  if (_current_stage < g_list_length (_stage_list)-1)
  {
    return FALSE;
  }

  return dynamic_cast <People::GeneralClassification *> (GetStage (_current_stage)) != NULL;
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

  if (stage_class && (g_ascii_strcasecmp  (stage_class->_xml_name, "TourDePoules") == 0))
  {
    GList *current     = _stage_list;
    guint  round_index = 0;

    while (current)
    {
      Stage             *current_stage       = (Stage *) current->data;
      Stage::StageClass *current_stage_class = current_stage->GetClass ();

      if (g_ascii_strcasecmp  (current_stage_class->_xml_name, "TourDePoules") == 0)
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
      stage = CreateStage ("TourDePoules");
      if (stage)
      {
        AddStage (stage);
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
void Schedule::DisplayLocks ()
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
void Schedule::AddStage (Stage *stage,
                         Stage *after)
{
  if ((after == NULL) && _stage_list)
  {
    after = (Stage *) (g_list_last (_stage_list)->data);
  }

  stage->SetContest (_contest);

  {
    Stage *input_provider = stage->GetInputProvider ();

    if (input_provider && (after != input_provider))
    {
      AddStage (input_provider,
                after);

      stage->SetInputProvider (input_provider);
      after = input_provider;
    }
  }

  InsertStage (stage,
               after);

  GiveStagesAnId ();
}

// --------------------------------------------------------------------------------
void Schedule::GiveStagesAnId ()
{
  GList *current = _stage_list;

  for (guint i = 0; current != NULL; i++)
  {
    Stage *stage = (Stage *) current->data;

    stage->SetId (i);

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Schedule::InsertStage (Stage *stage,
                            Stage *after)
{
  if ((stage == NULL) || g_list_find (_stage_list, stage))
  {
    return;
  }
  else
  {
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
                          NAME_str, stage->GetKlassName (),
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

    GiveName (stage);
    stage->FillInConfig ();
    RefreshSensitivity ();
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
        stage->SetPrevious (NULL);
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

          stage->Reset ();
          module->UnPlug ();
        }

        SetCurrentStage (_current_stage-1);

        {
          Stage *new_current_stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);

          new_current_stage->UnLock ();
          DisplayLocks ();
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
  GiveStagesAnId ();
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
gboolean Schedule::OnMessage (Net::Message *message)
{
  GList *current  = _stage_list;

  while (current)
  {
    Stage *stage = (Stage *) current->data;

    if (stage->GetNetID () == message->GetInteger ("stage"))
    {
      return stage->OnMessage (message);
    }

    current = g_list_next (current);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Schedule::OnHttpPost (const gchar *command,
                               const gchar **ressource,
                               const gchar *data)
{
  if (ressource[0])
  {
    guint  phase_id = atoi (ressource[0]);
    GList *current  = _stage_list;

    while (current)
    {
      Stage *stage = (Stage *) current->data;

      if (stage->GetId () == phase_id)
      {
        if (g_strcmp0 (command, "ScoreSheet") == 0)
        {
          gtk_notebook_set_current_page  (GTK_NOTEBOOK (GetRootWidget ()),
                                          phase_id);
        }
        else if (g_strcmp0 (command, "Score") == 0)
        {
          if (stage->Locked ())
          {
            return FALSE;
          }
        }

        return stage->OnHttpPost (command,
                                  &ressource[1],
                                  data);
      }

      current = g_list_next (current);
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Schedule::OnPrint ()
{
  Print (gettext ("Formula"));
}

// --------------------------------------------------------------------------------
guint Schedule::PreparePrint (GtkPrintOperation *operation,
                              GtkPrintContext   *context)
{
  return 1;
}

// --------------------------------------------------------------------------------
void Schedule::DrawPage (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gint               page_nr)
{
  DrawContainerPage (operation,
                     context,
                     page_nr);

  if (g_strcmp0 ((const gchar *) g_object_get_data (G_OBJECT (operation), "Print::PageName"),
                 gettext ("Formula")) == 0)

  {
    cairo_t *cr      = gtk_print_context_get_cairo_context (context);
    gdouble  paper_w = gtk_print_context_get_width (context);
    GList   *current = _stage_list;

    while (current && g_list_next (current))
    {
      Stage *stage = (Stage *) current->data;

      if (stage->GetInputProvider () == NULL)
      {
        Stage::StageClass *stage_class = stage->GetClass ();

        if (stage->GetInputProviderClient ())
        {
          stage_class = stage->GetNextStage()->GetClass();
        }

        cairo_translate (cr,
                         0.0,
                         5.0 * paper_w / 100);

        {
          GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);
          char      *text;

          text = g_strdup_printf ("%s %s",
                                        stage_class->_name,
                                        stage->GetName ());

          goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                               text,
                               0.0, 0.0,
                               -1.0,
                               GTK_ANCHOR_W,
                               "fill-color", "black",
                               "font", BP_FONT "Bold 2px", NULL);
          g_free (text);

          goo_canvas_render (canvas,
                             gtk_print_context_get_cairo_context (context),
                             NULL,
                             1.0);
          gtk_widget_destroy (GTK_WIDGET (canvas));
        }

        stage->DrawConfig (operation,
                           context,
                           page_nr);

        cairo_translate (cr,
                         0.0,
                         2.0 * paper_w / 100);
      }

      current = g_list_next (current);
    }

    cairo_translate (cr,
                     0.0,
                     4.0 * paper_w / 100);

    {
      FlashCode *flash = _contest->GetFlashCode ();

      if (flash)
      {
        GooCanvas     *canvas = Canvas::CreatePrinterCanvas (context);
        GooCanvasItem *item;
        gdouble        image_w;

        item = goo_canvas_image_new (goo_canvas_get_root_item (canvas),
                                     flash->GetPixbuf (),
                                     0.0,
                                     0.0,
                                     NULL);

        {
          GooCanvasBounds bounds;
          gdouble         h = 0.0;

          goo_canvas_item_get_bounds (item,
                                      &bounds);
          image_w = bounds.x2- bounds.x1;

          goo_canvas_convert_to_item_space (canvas,
                                            item,
                                            &image_w,
                                            &h);
        }

        goo_canvas_item_scale (item,
                               20/image_w,   // 20% of paper_w
                               20/image_w);  // 20% of paper_w

        {
          gchar *url = flash->GetText ();

          goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                               url,
                               2.0, 21.0,
                               -1.0,
                               GTK_ANCHOR_W,
                               "fill-color", "blue",
                               "font", BP_FONT "Bold 2px", NULL);
          g_free (url);
        }

        goo_canvas_render (canvas,
                           gtk_print_context_get_cairo_context (context),
                           NULL,
                           1.0);
        gtk_widget_destroy (GTK_WIDGET (canvas));
      }
    }
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
    stage->Save (xml_writer);
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Schedule::DumpToHTML (FILE *file)
{
  {
    GList *current = _stage_list;

    fprintf (file,
             "      <div id=\"menu\">\n"
             "        <ul id=\"menu_bar\">\n");

    while (current)
    {
      Stage *stage = (Stage *) current->data;

      if (stage->GetInputProviderClient () == NULL)
      {
        Stage::StageClass *stage_class = stage->GetClass ();

        fprintf (file, "          <li><a href=\"\" onclick=\"javascript:OnTabClicked (\'round_%d\'); return false;\">%s %s</a></li>\n",
                 stage->GetId (),
                 stage_class->_name,
                 stage->GetName ());
      }

      current = g_list_next (current);
    }

    fprintf (file,
             "        </ul>\n"
             "      </div>\n\n");
  }

  {
    GList *current = _stage_list;

    for (guint i = 0; current != NULL; i++)
    {
      Stage *stage = (Stage *) current->data;

      if (stage->GetInputProviderClient () == NULL)
      {
        Stage::StageClass *stage_class = stage->GetClass ();
        Module            *module      = (Module *) dynamic_cast <Module *> (stage);

        fprintf (file,
                 "      <div class=\"Round\" id=\"round_%d\"",
                 stage->GetId ());
        if (i == 0)
        {
          fprintf (file, " style=\"display: inline\">\n");
        }
        else
        {
          fprintf (file, ">\n");
        }
        fprintf (file,
                 "        <h1>%s</h1>\n",
                 stage_class->_name);

        if (i <= _current_stage)
        {
          module->DumpToHTML (file);
        }

        fprintf (file,
                 "      </div>\n");
      }

      current = g_list_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::Load (xmlDoc               *doc,
                     const gchar          *contest_keyword,
                     People::RefereesList *referees)
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

  referees->LoadList (xml_context,
                      contest_keyword);
  referees->GiveRefereesAnId ();
  referees->Disclose ("Referee");
  referees->Spread ();

  // Checkin - Player list
  if (checkin_stage)
  {
    AddStage  (checkin_stage);
    PlugStage (checkin_stage);

    checkin_stage->Load (xml_context,
                         contest_keyword);
    checkin_stage->FillInConfig ();
    checkin_stage->ApplyConfig ();
    checkin_stage->Display ();

    if (display_all || (current_stage_index > 0))
    {
      checkin_stage->Lock ();
      DisplayLocks ();
    }
  }

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
          stage = CreateStage ("TourDePoules");
          if (stage)
          {
            AddStage (stage);
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
}

// --------------------------------------------------------------------------------
void Schedule::LoadStage (Stage   *stage,
                          xmlNode *xml_node,
                          guint   *nb_stage,
                          gint     current_stage_index)
{
  if (stage)
  {
    AddStage (stage);

    {
      Stage *input_provider = stage->GetInputProvider ();

      if (input_provider)
      {
        (*nb_stage)++;
        LoadStage (input_provider,
                   xml_node,
                   *nb_stage,
                   current_stage_index);
      }
    }

    (*nb_stage)++;
    LoadStage (stage,
               xml_node,
               *nb_stage,
               current_stage_index);
  }
}

// --------------------------------------------------------------------------------
void Schedule::LoadStage (Stage   *stage,
                          xmlNode *xml_node,
                          guint    nb_stage,
                          gint     current_stage_index)
{
  gboolean display_all = (current_stage_index < 0);

  if (display_all || ((nb_stage-1) <= (guint) current_stage_index))
  {
    PlugStage (stage);
  }

  stage->Load (xml_node);
  stage->FillInConfig ();
  stage->ApplyConfig ();
  RefreshStageName (stage);

  if (display_all || ((nb_stage-1) <= (guint) current_stage_index))
  {
    stage->Display ();

    if (display_all || ((nb_stage-1) < (guint) current_stage_index))
    {
      stage->Lock ();
      DisplayLocks ();
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
      stage->SetStatusListener (this);
    }
  }

  RefreshSensitivity ();
}

// --------------------------------------------------------------------------------
void Schedule::OnStageStatusChanged (Stage *stage)
{
  RefreshSensitivity ();
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

  owner->AddStage (stage,
                   after);

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
  GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GetRootWidget ())),
                                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          gettext ("<b><big>Do you really want to cancel the current round?</big></b>"));

  gtk_window_set_title (GTK_WINDOW (dialog),
                        gettext ("Go back to the previous round?"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            gettext ("All the inputs of this round will be cancelled."));

  if (RunDialog (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    Stage             *stage       = (Stage *) g_list_nth_data (_stage_list, _current_stage);
    Stage::StageClass *stage_class = stage->GetClass ();

    if (g_strcmp0 (stage_class->_xml_name, "Barrage") == 0)
    {
      RemoveStage (stage);
    }
    else
    {
      Module *module = (Module *) dynamic_cast <Module *> (stage);

      stage->Recall ();
      stage->Reset ();
      module->UnPlug ();

      RemoveFromNotebook (stage);

      {
        SetCurrentStage (_current_stage-1);

        stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);
        stage->UnLock ();
        DisplayLocks ();
      }
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
  DisplayLocks ();

  g_list_foreach (_advertisers,
                  (GFunc) Net::Advertiser::TweetFeeder,
                  stage);

  if (stage->GetQuotaExceedance ())
  {
    GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GetRootWidget ())),
                                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                                            GTK_MESSAGE_QUESTION,
                                                            GTK_BUTTONS_YES_NO,
                                                            gettext ("<b><big>Because of ties, the quota is exceeded.\nDo you wish to add a barrage round?</big></b>"));

    gtk_window_set_title (GTK_WINDOW (dialog),
                          gettext ("Barrage"));

    if (RunDialog (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
    {
      Stage *barrage = CreateStage ("Barrage");

      AddStage (barrage,
                stage);
    }
    gtk_widget_destroy (dialog);
  }

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
    Stage *previous = stage->GetInputProvider ();

    if (previous == NULL)
    {
      previous = stage->GetPreviousStage ();
    }

    if (previous)
    {
      const guint32  seed[] = {stage->GetId (), previous->GetRandSeed ()};
      GRand         *rand;
      gint           rand_seed;

      rand = g_rand_new_with_seed_array (seed,
                                         sizeof (seed) / sizeof (guint32));
      rand_seed = (gint) g_rand_int (rand);
      g_rand_free (rand);

      stage->SetRandSeed (rand_seed);
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

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_print_schedule_toolbutton_clicked (GtkWidget *widget,
                                                                      Object    *owner)
{
  Schedule *s = dynamic_cast <Schedule *> (owner);

  s->OnPrint ();
}
