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

#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/flash_code.hpp"
#include "util/canvas.hpp"
#include "util/player.hpp"
#include "util/glade.hpp"
#include "util/xml_scheme.hpp"
#include "network/advertiser.hpp"
#include "network/message.hpp"
#include "actors/referees_list.hpp"
#include "rounds/classification/general_classification.hpp"
#include "rounds/checkin/checkin_supervisor.hpp"

#include "book/book.hpp"
#include "contest.hpp"

#include "schedule.hpp"

enum class ColumnId
{
  NAME_str,
  LOCK_str,
  STAGE_ptr,
  VISIBILITY_bool
};

// --------------------------------------------------------------------------------
Schedule::Schedule (Contest *contest,
                    GList   *advertisers,
                    Data    *minimum_team_size,
                    Data    *manual_classification,
                    Data    *default_classification)
  : Object ("Schedule"),
    Module ("schedule.glade", "schedule_notebook")
{
   _stage_list    = nullptr;
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
                                              (gint) ColumnId::VISIBILITY_bool);
  }

  // Formula dialog
  {
    _menu_shell = GTK_MENU_SHELL (gtk_menu_new ());

    gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (_glade->GetWidget ("add_stage_toolbutton")),
                                   GTK_WIDGET (_menu_shell));

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
        gtk_menu_shell_append (_menu_shell,
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

  for (GList *s = g_list_last (_stage_list); s; s = g_list_previous (s))
  {
    Stage *stage = (Stage *) s->data;

    stage->Release ();
  }
  g_list_free (_stage_list);
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

  if (stage && (g_ascii_strcasecmp (class_name, "Pointage") == 0))
  {
    People::CheckinSupervisor *checkin = dynamic_cast <People::CheckinSupervisor *> (stage);

    checkin->SetTeamData (_minimum_team_size,
                          _default_classification,
                          _manual_classification);
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

  return dynamic_cast <People::GeneralClassification *> (GetStage (_current_stage)) != nullptr;
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
  if (_stage_list == nullptr)
  {
    Stage *stage;

    stage = CreateStage ("Pointage");
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

  _contest->UnLock ();
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
                          ColumnId::STAGE_ptr, &current_stage,
                          -1);
      if (current_stage->Locked ())
      {
        _contest->Lock ();
        gtk_list_store_set (_list_store, &iter,
                            ColumnId::LOCK_str, GTK_STOCK_DIALOG_AUTHENTICATION,
                            -1);
      }
      else
      {
        gtk_list_store_set (_list_store, &iter,
                            ColumnId::LOCK_str, "",
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
                                       nullptr,
                                       &filter_iter))
  {
    Stage       *stage;
    GtkTreeIter  iter;

    gtk_tree_model_filter_convert_iter_to_child_iter (_list_store_filter,
                                                      &iter,
                                                      &filter_iter);
    gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                        &iter,
                        ColumnId::STAGE_ptr, &stage,
                        -1);

    return dynamic_cast <Module *> (stage);
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
void Schedule::AddStage (Stage *stage,
                         Stage *after)
{
  if ((after == nullptr) && _stage_list)
  {
    after = (Stage *) (g_list_last (_stage_list)->data);
  }

  stage->SetContest     (_contest);
  stage->ShareAttendees (after);

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

  for (guint i = 0; current != nullptr; i++)
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
  if ((stage == nullptr) || g_list_find (_stage_list, stage))
  {
    return;
  }
  else
  {
    stage->SetScoreStuffingPolicy (_score_stuffing_allowed);

    // Insert it in the global list
    {
      Stage *next = nullptr;

      if (_stage_list == nullptr)
      {
        _stage_list = g_list_append (_stage_list,
                                     stage);
      }
      else
      {
        guint index;

        if (after == nullptr)
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
                              ColumnId::STAGE_ptr, &current_stage,
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
                          ColumnId::NAME_str,        stage->GetPurpose (),
                          ColumnId::STAGE_ptr,       stage,
                          ColumnId::VISIBILITY_bool, (stage->GetRights () & Stage::EDITABLE) != 0,
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
    Stage *input_provider = stage->GetInputProvider ();

    {
      GList *current       = g_list_find (_stage_list, stage);
      gint   current_index = g_list_index (_stage_list, stage);
      GList *next          = g_list_next (current);

      if (next)
      {
        Stage *next_stage = (Stage *) (next->data);

        if (next_stage)
        {
          Stage *previous_stage = nullptr;
          GList *previous       = g_list_previous (current);

          if (previous)
          {
            previous_stage = (Stage *) (g_list_previous (current)->data);
          }
          next_stage->SetPrevious (previous_stage);
          stage->SetPrevious (nullptr);
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

      stage->Recall ();
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
                            ColumnId::STAGE_ptr, &current_stage,
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

    RemoveStage (input_provider);
  }
}

// --------------------------------------------------------------------------------
void Schedule::Mutate (const gchar *from,
                       const gchar *to)
{
  if (_current_stage == 0)
  {
    gboolean remaining = (_stage_list != nullptr);

    while (remaining)
    {
      for (GList *current = _stage_list; current != nullptr; current = g_list_next (current))
      {
        Stage             *from_stage  = (Stage *) current->data;
        Stage::StageClass *stage_class = from_stage->GetClass ();

        if (g_list_next (current) == nullptr)
        {
          remaining = FALSE;
        }

        if (g_strcmp0 (stage_class->_xml_name,
                       from) == 0)
        {
          Stage *to_stage = CreateStage (to);

          if (to_stage)
          {
            AddStage (to_stage,
                      from_stage);
            RemoveStage (from_stage);
            break;
          }
        }
      }
    }

    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (_menu_shell));

      for (GList *c = children; c; c = g_list_next (c))
      {
        GtkWidget *child      = (GtkWidget *) c->data;
        gchar     *class_name = (gchar *) g_object_get_data (G_OBJECT (child),
                                                             "Schedule::class_name");

        if (g_strcmp0 (class_name, from) == 0)
        {
          gtk_widget_hide (child);
        }
        else if (g_strcmp0 (class_name, to) == 0)
        {
          gtk_widget_show (child);
        }
      }
    }
  }
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
  for (GList *s = _stage_list; s; s = g_list_next (s))
  {
    Stage *stage = (Stage *) s->data;

    if (stage->GetId () > _current_stage)
    {
      break;
    }

    if (message->HasField ("stage"))
    {
      if (stage->GetNetID () == message->GetInteger ("stage"))
      {
        if (message->Is ("SmartPoule::Score"))
        {
          if (stage->GetInputProviderClient ())
          {
            stage = stage->GetNextStage ();
          }

          if (stage->Locked ())
          {
            return FALSE;
          }
        }

        return stage->OnMessage (message);
      }
    }
    else
    {
      if (stage->OnMessage (message))
      {
        return TRUE;
      }
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
    if (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "Print::FormulaPrinted")) == FALSE)
    {
      cairo_t *cr      = gtk_print_context_get_cairo_context (context);
      gdouble  paper_w = gtk_print_context_get_width (context);
      GList   *current = _stage_list;

      {
        GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

        goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                             "Paramètres généraux",
                             0.0, 0.0,
                             -1.0,
                             GTK_ANCHOR_W,
                             "fill-color", "black",
                             "font", BP_FONT "Bold 2px", NULL);

        goo_canvas_render (canvas,
                           gtk_print_context_get_cairo_context (context),
                           nullptr,
                           1.0);
        gtk_widget_destroy (GTK_WIDGET (canvas));

        _contest->DrawConfig (operation,
                              context,
                              page_nr);
      }

      while (current && g_list_next (current))
      {
        Stage *stage = (Stage *) current->data;

        if (stage->GetInputProvider () == nullptr)
        {
          Stage::StageClass *stage_class = stage->GetClass ();
          Module            *module      = (Module *) dynamic_cast <Module *> (stage);

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
                               nullptr,
                               1.0);
            gtk_widget_destroy (GTK_WIDGET (canvas));
          }

          module->DrawConfig (operation,
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
        GValue gvalue = {0,{{0}}};

        g_value_init (&gvalue, G_TYPE_STRING);
        g_object_get_property (G_OBJECT (operation), "export-filename", &gvalue);

        // Do not print QRCode in PDF
        if (g_value_get_string (&gvalue) == nullptr)
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
                               nullptr,
                               1.0);
            gtk_widget_destroy (GTK_WIDGET (canvas));
          }
        }
      }

      g_object_set_data (G_OBJECT (operation),
                         "Print::FormulaPrinted", GUINT_TO_POINTER (TRUE));
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::SaveFinalResults (XmlScheme *xml_scheme)
{
  People::CheckinSupervisor *checkin = GetCheckinSupervisor ();

  if (checkin)
  {
    if (_contest->IsTeamEvent ())
    {
      checkin->SaveList (xml_scheme, "Team");
    }
    else
    {
      checkin->SaveList (xml_scheme, "Fencer");
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule::SavePeoples (XmlScheme       *xml_scheme,
                            People::Checkin *referees)
{
  People::CheckinSupervisor *checkin = GetCheckinSupervisor ();

  if (checkin)
  {
    checkin->SaveList (xml_scheme, "Fencer");
    checkin->SaveList (xml_scheme, "Team");

    referees->SaveList (xml_scheme, nullptr);
  }
}

// --------------------------------------------------------------------------------
void Schedule::Save (XmlScheme *xml_scheme)
{
  xml_scheme->StartElement ("Phases");
  xml_scheme->WriteFormatAttribute ("PhaseEnCours", "%d", _current_stage);

  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Stage *stage = ((Stage *) g_list_nth_data (_stage_list,
                                               i));
    stage->Save (xml_scheme);
  }

  xml_scheme->EndElement ();
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

      if (stage->GetInputProviderClient () == nullptr)
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

    for (guint i = 0; current != nullptr; i++)
    {
      Stage *stage = (Stage *) current->data;

      if (stage->GetInputProviderClient () == nullptr)
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

  // Get the current stage
  {
    gchar          *xml_object_path = g_strdup_printf ("%s/Phases", contest_keyword);
    xmlXPathObject *xml_object      = xmlXPathEval (BAD_CAST xml_object_path, xml_context);
    xmlNodeSet     *xml_nodeset     = xml_object->nodesetval;

    g_free (xml_object_path);

    if (xml_object->nodesetval->nodeNr)
    {
      char *attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0],
                                         BAD_CAST "PhaseEnCours");

      if (attr)
      {
        current_stage_index = atoi (attr);
        xmlFree (attr);
      }
    }

    xmlXPathFreeObject  (xml_object);
  }

  gtk_widget_show_all (GetRootWidget ());

  // Referees
  {
    referees->LoadList (xml_context,
                        contest_keyword);
    referees->Disclose ("BellePoule::Referee");
    referees->Spread ();
  }

  // Stages
  {
    guint nb_stage = 0;

    // Add Checkin stage if missing
    {
      gchar          *xml_object_path = g_strdup_printf ("%s/Phases/Pointage", contest_keyword);
      xmlXPathObject *xml_object      = xmlXPathEval (BAD_CAST xml_object_path, xml_context);
      xmlNodeSet     *xml_nodeset     = xml_object->nodesetval;

      g_free (xml_object_path);

      if (xml_nodeset->nodeNr == 0)
      {
        Stage *first_stage = CreateStage ("Pointage");

        LoadPeoples (first_stage,
                     xml_context,
                     contest_keyword);

        AddStage  (first_stage);
        PlugStage (first_stage);
        first_stage->Display ();

        nb_stage++;
      }

      xmlXPathFreeObject  (xml_object);
    }

    {
      gchar          *path        = g_strdup_printf ("%s/Phases/*", contest_keyword);
      xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST path, xml_context);
      xmlNodeSet     *xml_nodeset = xml_object->nodesetval;

      g_free (path);

      for (guint i = 0; i < (guint) xml_nodeset->nodeNr; i++)
      {
        Stage *stage = CreateStage ((gchar *) xml_nodeset->nodeTab[i]->name);

        if (i == 0)
        {
          if (nb_stage == 0)
          {
            LoadPeoples (stage,
                         xml_context,
                         contest_keyword);
          }
          else
          {
            Stage *injected_checkin_stage = GetStage (0);

            injected_checkin_stage->Lock ();
            DisplayLocks ();
          }
        }

        LoadStage (stage,
                   xml_nodeset->nodeTab[i],
                   &nb_stage,
                   current_stage_index);
      }

      xmlXPathFreeObject (xml_object);
    }

    // Add additional stages
    // for xml files generated with other
    // tools than BellePoule
    if (nb_stage > 0)
    {
      if (dynamic_cast <People::GeneralClassification *> (GetStage (nb_stage-1)) == nullptr)
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
void Schedule::LoadPeoples (Stage           *stage_host,
                            xmlXPathContext *xml_context,
                            const gchar     *contest_keyword)
{
  People::CheckinSupervisor *checkin_stage = dynamic_cast <People::CheckinSupervisor *> (stage_host);

  if (checkin_stage)
  {
    checkin_stage->LoadList (xml_context,
                             contest_keyword,
                             "Team");

    checkin_stage->LoadList (xml_context,
                             contest_keyword,
                             "Fencer");
  }
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
      Error *error = stage->GetError ();

      if (error)
      {
        gtk_widget_show (_glade->GetWidget ("error_toolbutton"));

        {
          gchar *error_msg = error->GetText ();

          gtk_label_set_markup (GTK_LABEL (_glade->GetWidget ("error_label")),
                                error_msg);
          g_free (error_msg);
        }

        {
          GdkColor *color = g_new (GdkColor, 1);

          gdk_color_parse (error->GetColor (), color);
          gtk_widget_modify_bg (_glade->GetWidget ("error_viewport"),
                                GTK_STATE_NORMAL,
                                color);
          g_free (color);
        }

        error->Release ();
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
  Stage *after = nullptr;

  {
    GtkWidget        *treeview  = owner->_glade->GetWidget ("formula_treeview");
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    GtkTreeIter       filter_iter;

    if (gtk_tree_selection_get_selected (selection,
                                         nullptr,
                                         &filter_iter))
    {
      GtkTreeIter iter;

      gtk_tree_model_filter_convert_iter_to_child_iter (owner->_list_store_filter,
                                                        &iter,
                                                        &filter_iter);
      gtk_tree_model_get (GTK_TREE_MODEL (owner->_list_store),
                          &iter,
                          ColumnId::STAGE_ptr, &after,
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
  Stage *stage = (Stage *) g_list_nth_data (_stage_list, _current_stage);

  if (stage->ConfigValidated ())
  {
    _contest->Archive (stage->GetName ());
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
}

// --------------------------------------------------------------------------------
void Schedule::PlugStage (Stage *stage)
{
  Module    *module   = (Module *) dynamic_cast <Module *> (stage);
  GtkWidget *viewport = gtk_viewport_new (nullptr, nullptr);
  gchar     *name     = stage->GetFullName ();

  stage->SetData (this, "viewport_stage",
                  viewport);
  gtk_notebook_append_page (GTK_NOTEBOOK (GetRootWidget ()),
                            viewport,
                            gtk_label_new (name));
  g_free (name);

  {
    Stage *previous = stage->GetInputProvider ();

    if (previous == nullptr)
    {
      previous = stage->GetPreviousStage ();
    }

    if (previous)
    {
      const guint32  seed[] = {stage->GetId (), previous->GetAntiCheatToken ()};
      GRand         *rand;
      gint           anti_cheat_token;

      rand = g_rand_new_with_seed_array (seed,
                                         sizeof (seed) / sizeof (guint32));
      anti_cheat_token = (gint) g_rand_int (rand);
      g_rand_free (rand);

      stage->SetAntiCheatToken (anti_cheat_token);
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
                                         nullptr,
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
                          ColumnId::STAGE_ptr, &stage,
                          -1);

      stage_class = stage->GetClass ();
      if (stage_class->_rights & Stage::REMOVABLE)
      {
        RemoveStage (stage);

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
                                         nullptr,
                                         &filter_iter) == FALSE)
    {
      guint n = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (_list_store_filter),
                                                nullptr);

      if (n)
      {
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_list_store_filter),
                                       &filter_iter,
                                       nullptr,
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
  _book = new Book (this);
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
  _book = nullptr;

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
