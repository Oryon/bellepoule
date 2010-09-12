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

#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"
#include "classification.hpp"

const gchar *PoolSupervisor::_class_name     = N_ ("Poules");
const gchar *PoolSupervisor::_xml_class_name = "pool_stage";

typedef enum
{
  NAME_COLUMN,
  STATUS_COLUMN
} ColumnId;

extern "C" G_MODULE_EXPORT void on_pool_combobox_changed (GtkWidget *widget,
                                                          Object    *owner);

// --------------------------------------------------------------------------------
PoolSupervisor::PoolSupervisor (StageClass *stage_class)
  : Stage (stage_class),
  Module ("pool_supervisor.glade")
{
  _pool_allocator = NULL;
  _displayed_pool = NULL;
  _max_score      = NULL;
  _nb_eliminated  = NULL;

  _pool_liststore = GTK_LIST_STORE (_glade->GetObject ("pool_liststore"));

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
    AddSensitiveWidget (_glade->GetWidget ("nb_eliminated_spinbutton"));
    AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

    LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
    LockOnClassification (_glade->GetWidget ("pool_combobox"));
  }

  // Filter
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                               "ref",
#endif
                               "final_rank",
                               "attending",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "HS",
                               "rank",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("name");

    SetFilter (filter);
    filter->Release ();
  }

  {
    GtkWidget *content_area;

    _print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_QUESTION,
                                                        GTK_BUTTONS_OK_CANCEL,
                                                        gettext ("<b><big>Imprimer ...</big></b>"));

    gtk_window_set_title (GTK_WINDOW (_print_dialog),
                          gettext ("Impression des feuilles de poule"));

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_print_dialog));

    gtk_widget_reparent (_glade->GetWidget ("print_dialog-vbox"),
                         content_area);
  }

  // Classification filter
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                               "ref",
#endif
                               "final_rank",
                               "attending",
                               "exported",
                               NULL);
    filter = new Filter (attr_list);

    filter->ShowAttribute ("rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("victories_ratio");
    filter->ShowAttribute ("indice");
    filter->ShowAttribute ("HS");

    SetClassificationFilter (filter);
    filter->Release ();
  }
}

// --------------------------------------------------------------------------------
PoolSupervisor::~PoolSupervisor ()
{
  Object::TryToRelease (_pool_allocator);
  gtk_widget_destroy (_print_dialog);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Init ()
{
  RegisterStageClass (gettext (_class_name),
                      _xml_class_name,
                      CreateInstance,
                      EDITABLE);
}

// --------------------------------------------------------------------------------
Stage *PoolSupervisor::CreateInstance (StageClass *stage_class)
{
  return new PoolSupervisor (stage_class);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnPlugged ()
{
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton")),
                                     FALSE);

  RetrievePools ();

  for (guint i = 0; i < _pool_allocator->GetNbPools (); i++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (i);
    pool->SortPlayers ();
  }

}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnUnPlugged ()
{
  for (guint i = 0; i < _pool_allocator->GetNbPools (); i++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (i);
    pool->ResetMatches (GetPreviousStage ());
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Display ()
{
  OnPoolSelected (0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Garnish ()
{
  _displayed_pool = _pool_allocator->GetPool (0);
}

// --------------------------------------------------------------------------------
gint PoolSupervisor::ComparePlayer (Player         *A,
                                    Player         *B,
                                    PoolSupervisor *pool_supervisor)
{
  return Pool::ComparePlayer (A,
                              B,
                              pool_supervisor,
                              0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnLocked (Reason reason)
{
  DisableSensitiveWidgets ();

  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (p);
    pool->Lock ();
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnUnLocked ()
{
  EnableSensitiveWidgets ();

  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (p);
    pool->UnLock ();
  }

  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Wipe ()
{
  if (_pool_allocator)
  {
    for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
    {
      Pool *pool;

      pool = _pool_allocator->GetPool (p);
      pool->CleanScores ();
      pool->Wipe ();
    }

    if (_displayed_pool)
    {
      _displayed_pool->UnPlug ();
      _displayed_pool = NULL;
    }

    gtk_list_store_clear (_pool_liststore);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Manage (Pool *pool)
{
  GtkTreeIter iter;

  gtk_list_store_append (_pool_liststore,
                         &iter);

  gtk_list_store_set (_pool_liststore, &iter,
                      NAME_COLUMN, pool->GetName (),
                      -1);

  pool->SetRandSeed (0);
  pool->SetDataOwner (this);
  pool->SetFilter (_filter);
  pool->SetStatusCbk ((Pool::StatusCbk) OnPoolStatusUpdated,
                      this);
  pool->RefreshScoreData ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnPoolStatusUpdated (Pool           *pool,
                                          PoolSupervisor *ps)
{
  GtkTreeIter iter;

  if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ps->_pool_liststore),
                                     &iter,
                                     NULL,
                                     pool->GetNumber () - 1))
  {
    if (pool->IsOver ())
    {
      gtk_list_store_set (ps->_pool_liststore, &iter,
                          STATUS_COLUMN, GTK_STOCK_APPLY,
                          -1);
    }
    else if (pool->HasError ())
    {
      gtk_list_store_set (ps->_pool_liststore, &iter,
                          STATUS_COLUMN, GTK_STOCK_DIALOG_WARNING,
                          -1);
    }
    else
    {
      gtk_list_store_set (ps->_pool_liststore, &iter,
                          STATUS_COLUMN, GTK_STOCK_EXECUTE,
                          -1);
    }
  }

  ps->SignalStatusUpdate ();
}

// --------------------------------------------------------------------------------
gboolean PoolSupervisor::IsOver ()
{
  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (p);
    if (pool->IsOver () == FALSE)
    {
      return FALSE;
    }
  }
  return TRUE;
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnPoolSelected (gint index)
{
  if ((index >= 0) && _pool_allocator)
  {
    OnPoolSelected (_pool_allocator->GetPool (index));

    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("pool_combobox"),
                                          (void *) on_pool_combobox_changed,
                                          (Object *) this);
    gtk_combo_box_set_active (GTK_COMBO_BOX (_glade->GetWidget ("pool_combobox")),
                              index);
    g_signal_connect (_glade->GetWidget ("pool_combobox"), "changed",
                      G_CALLBACK (on_pool_combobox_changed),
                      (Object *) this);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnPoolSelected (Pool *pool)
{
  if (_displayed_pool)
  {
    _displayed_pool->UnPlug ();
    _displayed_pool = NULL;
  }

  if (pool)
  {
    Plug (pool,
          _glade->GetWidget ("main_hook"));

    _displayed_pool = pool;
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_print_pool_toolbutton_clicked (GtkWidget *widget,
                                                                  Object    *owner)
{
  PoolSupervisor *p = dynamic_cast <PoolSupervisor *> (owner);

  p->OnPrintPoolToolbuttonClicked ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnPrintPoolToolbuttonClicked ()
{
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))))
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      classification->Print (gettext ("Classement du tour de poule"));
    }
  }
  else if (gtk_dialog_run (GTK_DIALOG (_print_dialog)) == GTK_RESPONSE_OK)
  {
    GtkWidget *w = _glade->GetWidget ("all_pool_radiobutton");

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    {
      _print_all_pool = TRUE;
    }
    else
    {
      _print_all_pool = FALSE;
    }

    Print ("Feuille de poule");
  }
  gtk_widget_hide (_print_dialog);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnBeginPrint (GtkPrintOperation *operation,
                                   GtkPrintContext   *context)
{
  {
    GtkWidget *w = _glade->GetWidget ("for_referees_radiobutton");

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    {
      g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) TRUE);
    }
    else
    {
      g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) FALSE);
    }
  }

  if (_print_all_pool)
  {
    gtk_print_operation_set_n_pages (operation,
                                     _pool_allocator->GetNbPools ());
  }
  else
  {
    gtk_print_operation_set_n_pages (operation,
                                     1);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnDrawPage (GtkPrintOperation *operation,
                                 GtkPrintContext   *context,
                                 gint               page_nr)
{
  Module::OnDrawPage (operation,
                      context,
                      page_nr);

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))) == FALSE)
  {
    Pool *pool;

    if (_print_all_pool)
    {
      pool = _pool_allocator->GetPool (page_nr);
    }
    else
    {
      pool = _displayed_pool;
    }

    pool->DrawPage (operation,
                    context,
                    page_nr);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::RetrievePools ()
{
  if (_pool_allocator)
  {
    for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
    {
      Manage (_pool_allocator->GetPool (p));
    }
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::ApplyConfig ()
{
  {
    GtkWidget *w        = _glade->GetWidget ("name_entry");
    gchar     *name     = (gchar *) gtk_entry_get_text (GTK_ENTRY (w));
    Stage     *previous = GetPreviousStage ();

    SetName (name);
    previous->SetName (name);
  }

  {
    GtkWidget *w = _glade->GetWidget ("max_score_entry");

    if (w)
    {
      gchar *str = (gchar *) gtk_entry_get_text (GTK_ENTRY (w));

      if (str)
      {
        _max_score->_value = atoi (str);

        if (_displayed_pool)
        {
          OnPoolSelected (_displayed_pool);
        }
      }
    }
  }

  {
    GtkWidget *w = _glade->GetWidget ("nb_eliminated_spinbutton");

    if (w)
    {
      _nb_eliminated->_value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (w));
    }
  }
}

// --------------------------------------------------------------------------------
Stage *PoolSupervisor::GetInputProvider ()
{
  Stage *previous = GetPreviousStage ();

  if (previous)
  {
    Stage::StageClass *provider_class = previous->GetClass ();

    if (strcmp (provider_class->_xml_name, PoolAllocator::_xml_class_name) == 0)
    {
      return previous;
    }
  }

  return Stage::CreateInstance (PoolAllocator::_xml_class_name);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::SetInputProvider (Stage *input_provider)
{
  _pool_allocator = dynamic_cast <PoolAllocator *> (input_provider);

  if (_pool_allocator)
  {
    _pool_allocator->Retain ();
    _max_score     = _pool_allocator->GetMaxScore     ();
    _nb_eliminated = _pool_allocator->GetNbEliminated ();
  }

  Stage::SetInputProvider (input_provider);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::FillInConfig ()
{
  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("name_entry")),
                      GetName ());

  {
    gchar *text = g_strdup_printf ("%d", _max_score->_value);

    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("max_score_entry")),
                        text);
    g_free (text);
  }

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (_glade->GetWidget ("nb_eliminated_spinbutton")),
                             _nb_eliminated->_value);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnAttrListUpdated ()
{
  if (_displayed_pool)
  {
    OnPoolSelected (_displayed_pool);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnFilterClicked ()
{
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))))
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      classification->SelectAttributes ();
    }
  }
  else
  {
    SelectAttributes ();
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnStuffClicked ()
{
  for (guint i = 0; i < _pool_allocator->GetNbPools (); i++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (i);
    pool->CleanScores ();
    pool->Stuff ();
  }

  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
GSList *PoolSupervisor::GetCurrentClassification ()
{
  GSList *result = NULL;

  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (p);
    for (guint i = 0; i < pool->GetNbPlayers (); i++)
    {
      Player *player;

      player = pool->GetPlayer (i);

      result = g_slist_prepend (result,
                                player);
    }
  }

  result = g_slist_sort_with_data (result,
                                   (GCompareDataFunc) ComparePlayer,
                                   this);

  return result;
}

// --------------------------------------------------------------------------------
GSList *PoolSupervisor::GetOutputShortlist ()
{
  GSList *shortlist         = Stage::GetOutputShortlist ();
  guint   nb_to_remove      = _nb_eliminated->_value * g_slist_length (shortlist) / 100;
  guint   rank              = g_slist_length (shortlist);
  Player::AttributeId *attr = new Player::AttributeId ("final_rank");

  for (guint i = 0; i < nb_to_remove; i++)
  {
    Player *player;

    player = (Player *) shortlist->data;
    player->SetAttributeValue (attr,
                               rank);
    g_print (">>> %s (%d)\n", player->GetName (), rank);
    rank--;

    shortlist = g_slist_delete_link (shortlist,
                                     g_slist_last (shortlist));
  }
  Object::TryToRelease (attr);

  return shortlist;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pool_filter_toolbutton_clicked (GtkWidget *widget,
                                                                   Object    *owner)
{
  PoolSupervisor *p = dynamic_cast <PoolSupervisor *> (owner);

  p->OnFilterClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pool_combobox_changed (GtkWidget *widget,
                                                          Object    *owner)
{
  PoolSupervisor *p          = dynamic_cast <PoolSupervisor *> (owner);
  gint            pool_index = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

  p->OnPoolSelected (pool_index);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pool_classification_toggletoolbutton_toggled (GtkWidget *widget,
                                                                                 Object    *owner)
{
  PoolSupervisor *p = dynamic_cast <PoolSupervisor *> (owner);

  p->ToggleClassification (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_stuff_toolbutton_clicked (GtkWidget *widget,
                                                             Object    *owner)
{
  PoolSupervisor *ps = dynamic_cast <PoolSupervisor *> (owner);

  ps->OnStuffClicked ();
}
