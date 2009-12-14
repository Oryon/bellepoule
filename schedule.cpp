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

#include "schedule.hpp"

typedef enum
{
  NAME_COLUMN,
  LOCK_COLUMN,
  STAGE_COLUMN
} ColumnId;

// --------------------------------------------------------------------------------
Schedule_c::Schedule_c ()
: Module_c ("schedule.glade",
            "schedule_notebook")
{
  _stage_list    = NULL;
  _current_stage = 0;

  _list_store = GTK_LIST_STORE (_glade->GetObject ("stage_liststore"));

  // Formula dialog
  {
    GtkWidget *menu_pool = gtk_menu_new ();
    GtkWidget *content_area;

    _formula_dlg = gtk_dialog_new_with_buttons ("New competition",
                                                NULL,
                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_STOCK_OK,
                                                GTK_RESPONSE_ACCEPT,
                                                GTK_STOCK_CANCEL,
                                                GTK_RESPONSE_REJECT,
                                                NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_formula_dlg));

    gtk_widget_reparent (_glade->GetWidget ("dialog-vbox"),
                         content_area);

    gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (_glade->GetWidget ("add_stage_toolbutton")),
                                   menu_pool);

    for (guint i = 0; i < Stage_c::GetNbStageClass (); i++)
    {
      gchar            *name;
      Stage_c::Creator  creator;
      Stage_c::Rights   rights;
      GtkWidget        *menu_item;

      Stage_c::GetStageClass (i,
                              &name,
                              &creator,
                              &rights);

      if (rights & Stage_c::EDITABLE)
      {
        menu_item = gtk_menu_item_new_with_label (name);
        g_signal_connect (menu_item, "button-release-event",
                          G_CALLBACK (on_new_stage_selected), this);
        g_object_set_data (G_OBJECT (menu_item),
                           "Schedule_c::class_name",
                           (void *) name);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu_pool),
                               menu_item);
        gtk_widget_show (menu_item);
      }
    }
  }
}

// --------------------------------------------------------------------------------
Schedule_c::~Schedule_c ()
{
  guint nb_stages = g_list_length (_stage_list);

  for (guint i = 0; i < nb_stages; i++)
  {
    Stage_c *stage;

    stage = ((Stage_c *) g_list_nth_data (_stage_list,
                                          nb_stages - i-1));
    stage->Wipe ();
    Object_c::Release (stage);
  }
  g_list_free (_stage_list);

  gtk_widget_destroy (_formula_dlg);

  gtk_list_store_clear (_list_store);
  g_object_unref (_list_store);
}

// --------------------------------------------------------------------------------
void Schedule_c::DisplayList ()
{
  if (_stage_list == NULL)
  {
    for (guint i = 0; i < Stage_c::GetNbStageClass (); i++)
    {
      gchar            *name;
      Stage_c::Creator  creator;
      Stage_c::Rights   rights;

      Stage_c::GetStageClass (i,
                              &name,
                              &creator,
                              &rights);

      if (rights & Stage_c::MANDATORY)
      {
        guint    nb_stages;
        Stage_c *stage = Stage_c::CreateInstance (name);

        AddStage (stage);
        nb_stages = g_list_length (_stage_list);
        if (nb_stages == 1)
        {
          PlugStage (stage);
          stage->RetrieveAttendees ();
          stage->Garnish ();
          stage->Display ();
        }
      }
    }
  }
  else
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_list_store),
                                                   &iter);
    while (iter_is_valid)
    {
      Stage_c *current_stage;

      gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                          &iter,
                          STAGE_COLUMN, &current_stage, -1);
      if (   current_stage->Locked ()
          || (current_stage->GetRights () & Stage_c::MANDATORY))
      {
        gtk_list_store_set (_list_store, &iter,
                            LOCK_COLUMN, GTK_STOCK_DIALOG_AUTHENTICATION,
                            -1);
      }
      else
      {
        gtk_list_store_set (_list_store, &iter,
                            LOCK_COLUMN, "",
                            -1);
      }

      current_stage->FillInConfig ();

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_list_store),
                                                &iter);
    }
  }

  if (gtk_dialog_run (GTK_DIALOG (_formula_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    Stage_c *current_stage;

    for (guint i = 0; i < g_list_length (_stage_list); i++)
    {
      current_stage = (Stage_c *) g_list_nth_data (_stage_list,
                                                   i);
      current_stage->ApplyConfig ();
    }

    for (guint i = 0; i < g_list_length (_stage_list); i++)
    {
      GtkWidget *tab_widget;

      current_stage = (Stage_c *) g_list_nth_data (_stage_list,
                                                   i);
      tab_widget = (GtkWidget *) current_stage->GetData ("Schedule_c::viewport_stage");

      if (tab_widget)
      {
        gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (GetRootWidget ()),
                                         (GtkWidget *) current_stage->GetData ("Schedule_c::viewport_stage"),
                                         current_stage->GetFullName ());
      }
    }
  }
  gtk_widget_hide (_formula_dlg);
}

// --------------------------------------------------------------------------------
Module_c *Schedule_c::GetSelectedModule  ()
{
  GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  GtkTreeIter       iter;

  if (gtk_tree_selection_get_selected (selection,
                                       NULL,
                                       &iter))
  {
    Stage_c *stage;

    gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                        &iter,
                        STAGE_COLUMN, &stage, -1);

    return dynamic_cast <Module_c *> (stage);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Schedule_c::AddStage (Stage_c *stage,
                           gint     index)
{
  if ((stage == NULL) || g_list_find (_stage_list, stage))
  {
    return;
  }
  else
  {
    Stage_c *previous = NULL;

    if (_stage_list)
    {
      previous = (Stage_c *) (g_list_last (_stage_list)->data);
    }

    if (stage->GetRights () & Stage_c::EDITABLE)
    {
      GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
      GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
      GtkTreeIter       iter;

      if (gtk_tree_selection_get_selected (selection,
                                           NULL,
                                           &iter))
      {
        gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                            &iter,
                            STAGE_COLUMN, &previous,
                            -1);

        if (index == -1)
        {
          index = g_list_index (_stage_list,
                                previous) + 1;
        }

        gtk_list_store_insert_after (_list_store,
                                     &iter,
                                     &iter);

        gtk_tree_selection_select_iter (selection,
                                        &iter);
      }
      else
      {
        gtk_list_store_append (_list_store,
                               &iter);
      }

      gtk_list_store_set (_list_store, &iter,
                          NAME_COLUMN, stage->GetClassName (),
                          STAGE_COLUMN, stage, -1);
    }

    stage->SetPrevious (previous);

    _stage_list = g_list_insert (_stage_list,
                                 stage,
                                 index);

    {
      Stage_c *input_provider = stage->GetInputProvider ();

      stage->SetData ("Schedule_c::attached_stage",
                      input_provider);

      if (   input_provider
          && (g_list_find (_stage_list, input_provider) == NULL))
      {
        AddStage (input_provider,
                  index);
        input_provider->SetPrevious (previous);
        previous = input_provider;
        stage->SetPrevious (previous);
      }
    }

    RefreshSensitivity ();

#if 0
    for (guint i = 0; i < g_list_length (_stage_list); i++)
    {
      Stage_c *stage;
      Stage_c *previous;

      stage = ((Stage_c *) g_list_nth_data (_stage_list,
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
void Schedule_c::RemoveStage (Stage_c *stage)
{
  if (stage)
  {
    gint   page    = GetNotebookPageNum (stage);
    GList *current = g_list_find (_stage_list, stage);
    GList *next    = g_list_next (current);

    if (next)
    {
      Stage_c *next_stage = (Stage_c *) (next->data);

      if (next)
      {
        Stage_c *previous_stage = NULL;
        GList   *previous       = g_list_previous (current);

        if (previous)
        {
          previous_stage = (Stage_c *) (g_list_previous (current)->data);
        }
        next_stage->SetPrevious (previous_stage);
      }
    }

    {
      guint n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (GetRootWidget ()));

      _stage_list = g_list_remove (_stage_list,
                                   stage);

      if ((n_pages > 0) && (_current_stage >= n_pages-1))
      {
        Stage_c *stage;

        SetCurrentStage (_current_stage-1);

        stage = (Stage_c *) g_list_nth_data (_stage_list, _current_stage);
        stage->UnLock ();
      }
    }

    stage->Release ();

    if (page > 0)
    {
      gtk_notebook_remove_page (GTK_NOTEBOOK (GetRootWidget ()),
                                page);
    }
  }

  {
    GtkTreeIter  iter;
    gboolean     iter_is_valid;
    GtkTreePath *path_to_select;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_list_store),
                                                   &iter);
    while (iter_is_valid)
    {
      Stage_c *current_stage;

      gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                          &iter,
                          STAGE_COLUMN, &current_stage, -1);
      if (current_stage == stage)
      {
        path_to_select = gtk_tree_model_get_path (GTK_TREE_MODEL (_list_store),
                                                  &iter);

        gtk_list_store_remove (_list_store,
                               &iter);
        break;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_list_store),
                                                &iter);
    }

    if (iter_is_valid)
    {
      GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
      GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_list_store),
                                   &iter,
                                   path_to_select) == FALSE)
      {
        gtk_tree_path_prev (path_to_select);
      }

      gtk_tree_model_get_iter (GTK_TREE_MODEL (_list_store),
                               &iter,
                               path_to_select);
      gtk_tree_path_free (path_to_select);

      gtk_tree_selection_select_iter (selection,
                                      &iter);
      on_stage_selected ();
    }
  }
  RefreshSensitivity ();
}

// --------------------------------------------------------------------------------
void Schedule_c::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "schedule");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "current_stage",
                                     "%d", _current_stage);

  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Stage_c *stage;

    stage = ((Stage_c *) g_list_nth_data (_stage_list,
                                          i));
    stage->Save (xml_writer);
  }
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Schedule_c::Load (xmlDoc *doc)
{
  xmlXPathContext *xml_context = xmlXPathNewContext (doc);
  guint            current_stage_index = 0;
  Stage_c         *stage = (Stage_c *) g_list_nth_data (_stage_list,
                                                        0);

  {
    xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST "/contest/schedule", xml_context);
    xmlNodeSet     *xml_nodeset = xml_object->nodesetval;
    char           *attr        = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0],
                                                        BAD_CAST "current_stage");

    if (attr)
    {
      current_stage_index = atoi (attr);
    }

    xmlXPathFreeObject  (xml_object);
  }

  {
    xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST "/contest/schedule/*", xml_context);
    xmlNodeSet     *xml_nodeset = xml_object->nodesetval;

    for (guint i = 0; i < (guint) xml_nodeset->nodeNr; i++)
    {
      stage = Stage_c::CreateInstance (xml_nodeset->nodeTab[i]);
      if (stage)
      {
        gchar *attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i],
                                            BAD_CAST "name");
        stage->SetName (attr);
        AddStage (stage);

        if (i <= current_stage_index)
        {
          PlugStage (stage);
        }

        stage->RetrieveAttendees ();
        stage->Load (xml_nodeset->nodeTab[i]);

        if (i <= current_stage_index)
        {
          stage->Display ();

          if (i < current_stage_index)
          {
            stage->Lock ();
          }
        }
      }
    }

    xmlXPathFreeObject  (xml_object);
  }

  xmlXPathFreeContext (xml_context);

  gtk_widget_show_all (GetRootWidget ());
  SetCurrentStage (current_stage_index);
}

// --------------------------------------------------------------------------------
void Schedule_c::SetCurrentStage (guint index)
{
  _current_stage = index;

  gtk_notebook_set_current_page  (GTK_NOTEBOOK (GetRootWidget ()),
                                  _current_stage);

  {
    Stage_c *stage = (Stage_c *) g_list_nth_data (_stage_list, _current_stage);
    gchar   *name  = g_strdup_printf ("%s / %s", stage->GetClassName (), stage->GetName ());

    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("stage_entry")),
                        name);

    stage->SetStatusCbk ((Stage_c::StatusCbk) OnStageStatusUpdated,
                         this);
  }

  RefreshSensitivity ();
}

// --------------------------------------------------------------------------------
void Schedule_c::OnStageStatusUpdated (Stage_c    *stage,
                                       Schedule_c *schedule)
{
  schedule->RefreshSensitivity ();
}

// --------------------------------------------------------------------------------
void Schedule_c::RefreshSensitivity ()
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

  if (_current_stage < g_list_length (_stage_list) - 1)
  {
    Stage_c *stage = (Stage_c *) g_list_nth_data (_stage_list, _current_stage);

    if (stage->IsOver ())
    {
      gtk_widget_set_sensitive (_glade->GetWidget ("next_stage_toolbutton"),
                                TRUE);
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule_c::OnPlugged ()
{
  GtkToolbar *toolbar = GetToolbar ();
  GtkWidget  *w;

  w = _glade->GetWidget ("previous_stage_toolbutton");
  _glade->DetachFromParent (w);
  gtk_toolbar_insert (toolbar,
                      GTK_TOOL_ITEM (w),
                      -1);

  w = _glade->GetWidget ("stage_name_toolbutton");
  _glade->DetachFromParent (w);
  gtk_toolbar_insert (toolbar,
                      GTK_TOOL_ITEM (w),
                      -1);

  w = _glade->GetWidget ("next_stage_toolbutton");
  _glade->DetachFromParent (w);
  gtk_toolbar_insert (toolbar,
                      GTK_TOOL_ITEM (w),
                      -1);
}

// --------------------------------------------------------------------------------
gboolean Schedule_c::on_new_stage_selected (GtkWidget      *widget,
                                            GdkEventButton *event,
                                            Schedule_c     *owner)
{
  gchar *class_name = (gchar *) g_object_get_data (G_OBJECT (widget),
                                                   "Schedule_c::class_name");
  Stage_c *stage = Stage_c::CreateInstance (class_name);

  owner->AddStage (stage);
  stage->FillInConfig ();
  owner->on_stage_selected ();
  return FALSE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_formula_treeview_cursor_changed (GtkWidget *widget,
                                                                    Object_c  *owner)
{
  Schedule_c *s = dynamic_cast <Schedule_c *> (owner);

  s->on_stage_selected ();
}

// --------------------------------------------------------------------------------
void Schedule_c::on_stage_selected ()
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
    Module_c *selected_module = GetSelectedModule ();

    if (selected_module)
    {
      Stage_c   *stage    = dynamic_cast <Stage_c *> (selected_module);
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

      if (   stage->Locked ()
          || (stage->GetRights () & Stage_c::MANDATORY))
      {
        GList *list = g_list_find (_stage_list, stage);
        GList *next = g_list_next (list);

        if (next)
        {
          Stage_c *selected_stage = (Stage_c *) list->data;
          Stage_c *next_stage     = (Stage_c *) next->data;

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
                                FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("remove_stage_toolbutton")),
                                FALSE);
    }
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_previous_stage_toolbutton_clicked (GtkWidget *widget,
                                                                      Object_c  *owner)
{
  Schedule_c *s = dynamic_cast <Schedule_c *> (owner);

  s->on_previous_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
gint Schedule_c::GetNotebookPageNum (Stage_c *stage)
{
  GtkWidget *viewport = (GtkWidget *) stage->GetData ("Schedule_c::viewport_stage");

  return gtk_notebook_page_num (GTK_NOTEBOOK (GetRootWidget ()),
                                viewport);
}

// --------------------------------------------------------------------------------
void Schedule_c::on_previous_stage_toolbutton_clicked ()
{
  {
    Stage_c  *stage  = (Stage_c *) g_list_nth_data (_stage_list, _current_stage);
    gint      page    = GetNotebookPageNum (stage);
    Module_c *module = (Module_c *) dynamic_cast <Module_c *> (stage);

    stage->Wipe ();
    module->UnPlug ();

    gtk_notebook_remove_page (GTK_NOTEBOOK (GetRootWidget ()),
                              page);
  }

  {
    Stage_c *stage;

    SetCurrentStage (_current_stage-1);

    stage = (Stage_c *) g_list_nth_data (_stage_list, _current_stage);
    stage->UnLock ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_next_stage_toolbutton_clicked (GtkWidget *widget,
                                                                  Object_c  *owner)
{
  Schedule_c *s = dynamic_cast <Schedule_c *> (owner);

  s->on_next_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Schedule_c::on_next_stage_toolbutton_clicked ()
{
  Stage_c *stage;

  stage = (Stage_c *) g_list_nth_data (_stage_list, _current_stage);
  stage->Lock ();

  stage = (Stage_c *) g_list_nth_data (_stage_list, _current_stage+1);
  PlugStage (stage);
  stage->RetrieveAttendees ();
  stage->Garnish ();
  stage->Display ();

  SetCurrentStage (_current_stage+1);
}

// --------------------------------------------------------------------------------
void Schedule_c::PlugStage (Stage_c *stage)
{
  Module_c  *module   = (Module_c *) dynamic_cast <Module_c *> (stage);
  GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
  gchar     *name     = stage->GetFullName ();

  stage->SetData ("Schedule_c::viewport_stage",
                  viewport);
  gtk_notebook_append_page (GTK_NOTEBOOK (GetRootWidget ()),
                            viewport,
                            gtk_label_new (name));
  g_free (name);

  Plug (module,
        viewport);

  gtk_widget_show_all (GetRootWidget ());
}

// --------------------------------------------------------------------------------
void Schedule_c::on_stage_removed ()
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
    GtkWidget        *treeview = _glade->GetWidget ("formula_treeview");
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    GtkTreeIter       iter;

    if (gtk_tree_selection_get_selected (selection,
                                         NULL,
                                         &iter))
    {
      Stage_c *stage;
      Stage_c *attached_stage;

      gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                          &iter,
                          STAGE_COLUMN, &stage, -1);

      attached_stage = (Stage_c *) stage->GetData ("Schedule_c::attached_stage");

      RemoveStage (stage);
      RemoveStage (attached_stage);
    }

    if (gtk_tree_selection_get_selected (selection,
                                         NULL,
                                         &iter) == FALSE)
    {
      gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_list_store),
                                     &iter,
                                     NULL,
                                     g_list_length (_stage_list) - 1);

      gtk_tree_selection_select_iter (selection,
                                      &iter);
    }
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_stage_toolbutton_clicked (GtkWidget *widget,
                                                                    Object_c  *owner)
{
  Schedule_c *s = dynamic_cast <Schedule_c *> (owner);

  s->on_stage_removed ();
}
