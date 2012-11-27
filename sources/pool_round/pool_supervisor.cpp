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
#include "classification.hpp"
#include "pool_supervisor.hpp"

const gchar *PoolSupervisor::_class_name     = N_ ("Pools");
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

  _single_owner = new Object ();

  _pool_liststore = GTK_LIST_STORE (_glade->GetGObject ("pool_liststore"));

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
    AddSensitiveWidget (_glade->GetWidget ("qualified_table"));
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
                               "availability",
                               "participation_rate",
                               "level",
                               "status",
                               "global_status",
                               "start_rank",
                               "final_rank",
                               "attending",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "pool_nr",
                               "HS",
                               "rank",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");

    SetFilter (filter);
    filter->Release ();
  }

  {
    GtkWidget *content_area;

    _print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_QUESTION,
                                                        GTK_BUTTONS_OK_CANCEL,
                                                        gettext ("<b><big>Print...</big></b>"));

    gtk_window_set_title (GTK_WINDOW (_print_dialog),
                          gettext ("Pool sheets printing"));

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
                               "availability",
                               "participation_rate",
                               "level",
                               "global_status",
                               "start_rank",
                               "final_rank",
                               "attending",
                               "exported",
                               NULL);
    filter = new Filter (attr_list);

    filter->ShowAttribute ("rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("pool_nr");
    filter->ShowAttribute ("victories_ratio");
    filter->ShowAttribute ("indice");
    filter->ShowAttribute ("HS");
    filter->ShowAttribute ("status");

    SetClassificationFilter (filter);
    filter->Release ();
  }

  {
    Classification *classification = GetClassification ();

    classification->SetSortFunction ((GtkTreeIterCompareFunc) CompareClassification,
                                     this);
  }
}

// --------------------------------------------------------------------------------
PoolSupervisor::~PoolSupervisor ()
{
  Object::TryToRelease (_pool_allocator);
  gtk_widget_destroy (_print_dialog);

  _single_owner->Release ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Declare ()
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

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("single_radiobutton")),
                                TRUE);

  gtk_widget_set_sensitive (_glade->GetWidget ("seeding_viewport"),
                            FALSE);

  RetrievePools ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnUnPlugged ()
{
  gtk_widget_set_sensitive (_glade->GetWidget ("seeding_viewport"),
                            TRUE);

  for (guint i = 0; i < _pool_allocator->GetNbPools (); i++)
  {
    Pool *pool = _pool_allocator->GetPool (i);

    pool->DeleteMatchs ();
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Display ()
{
  Classification *classification = GetClassification ();

  OnPoolSelected (0);
  ToggleClassification (FALSE);

  classification->SetDataOwner (_single_owner);
  classification->SortDisplay ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Garnish ()
{
  _displayed_pool = _pool_allocator->GetPool (0);
}

// --------------------------------------------------------------------------------
gint PoolSupervisor::CompareSingleClassification (Player         *A,
                                                  Player         *B,
                                                  PoolSupervisor *pool_supervisor)
{
  guint policy = Pool::WITH_CALCULUS | Pool::WITH_RANDOM;

  if (pool_supervisor->_pool_allocator->SeedingIsBalanced () == FALSE)
  {
    policy |= Pool::WITH_POOL_NR;
  }

  return Pool::ComparePlayer (A,
                              B,
                              pool_supervisor->_single_owner,
                              pool_supervisor->_rand_seed,
                              pool_supervisor->GetDataOwner (),
                              policy);
}

// --------------------------------------------------------------------------------
gint PoolSupervisor::CompareCombinedClassification (Player         *A,
                                                    Player         *B,
                                                    PoolSupervisor *pool_supervisor)
{
  guint policy = Pool::WITH_CALCULUS | Pool::WITH_RANDOM;

  if (pool_supervisor->_pool_allocator->SeedingIsBalanced () == FALSE)
  {
    policy |= Pool::WITH_POOL_NR;
  }

  return Pool::ComparePlayer (A,
                              B,
                              pool_supervisor,
                              pool_supervisor->_rand_seed,
                              pool_supervisor->GetDataOwner (),
                              policy);
}

// --------------------------------------------------------------------------------
gint PoolSupervisor::CompareClassification (GtkTreeModel   *model,
                                            GtkTreeIter    *a,
                                            GtkTreeIter    *b,
                                            PoolSupervisor *pool_supervisor)
{
  GtkWidget      *w              = pool_supervisor->_glade->GetWidget ("single_radiobutton");
  Classification *classification = pool_supervisor->GetClassification ();
  Player         *A              = classification->GetPlayer (model, a);
  Player         *B              = classification->GetPlayer (model, b);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
  {
    return CompareSingleClassification (A,
                                        B,
                                        pool_supervisor);
  }
  else
  {
    return CompareCombinedClassification (A,
                                          B,
                                          pool_supervisor);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnLocked ()
{
  DisableSensitiveWidgets ();

  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool *pool = _pool_allocator->GetPool (p);

    pool->Lock ();
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnUnLocked ()
{
  EnableSensitiveWidgets ();

  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool *pool = _pool_allocator->GetPool (p);

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
      Pool *pool = _pool_allocator->GetPool (p);

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

  {
    Stage          *previous_stage = GetPreviousStage ();
    PoolSupervisor *previous_pool = dynamic_cast <PoolSupervisor *> (previous_stage->GetPreviousStage ());

    pool->SetDataOwner (_single_owner,
                        this,
                        previous_pool);
  }
  pool->CopyPlayersStatus (_pool_allocator);

  pool->SetFilter (_filter);
  pool->SetStatusCbk ((Pool::StatusCbk) OnPoolStatusUpdated,
                      this);

  gtk_list_store_set (_pool_liststore, &iter,
                      STATUS_COLUMN, GTK_STOCK_EXECUTE,
                      -1);
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
    PoolZone *zone = ps->_pool_allocator->GetZone (pool->GetNumber () - 1);

    if (pool->IsOver ())
    {
      zone->FreeReferees ();
      gtk_list_store_set (ps->_pool_liststore, &iter,
                          STATUS_COLUMN, GTK_STOCK_APPLY,
                          -1);
    }
    else if (pool->HasError ())
    {
      zone->BookReferees ();
      gtk_list_store_set (ps->_pool_liststore, &iter,
                          STATUS_COLUMN, GTK_STOCK_DIALOG_WARNING,
                          -1);
    }
    else
    {
      zone->BookReferees ();
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
    Pool *pool = _pool_allocator->GetPool (p);

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
  gchar *title = NULL;

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))))
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      title = g_strdup_printf ("%s - %s", gettext ("Pools classification"), GetName ());
      classification->Print (title);
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

    title = g_strdup_printf ("%s - %s", gettext ("Pools"), GetName ());
    Print (title);
  }
  g_free (title);
  gtk_widget_hide (_print_dialog);
}

// --------------------------------------------------------------------------------
gchar *PoolSupervisor::GetPrintName ()
{
  return g_strdup_printf ("%s - %s", gettext ("Pools"), GetName ());
}

// --------------------------------------------------------------------------------
guint PoolSupervisor::PreparePrint (GtkPrintOperation *operation,
                                    GtkPrintContext   *context)
{
  if (GetStageView (operation) == STAGE_VIEW_CLASSIFICATION)
  {
    return 0;
  }

  if (GetStageView (operation) == STAGE_VIEW_RESULT)
  {
    _print_all_pool = TRUE;
  }

  {
    GtkWidget *w = _glade->GetWidget ("for_referees_radiobutton");

    if (   (GetStageView (operation) == STAGE_VIEW_UNDEFINED)
        && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    {
      g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) TRUE);
    }
    else
    {
      g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) FALSE);
    }
  }

  if (_displayed_pool)
  {
    _displayed_pool->Wipe ();
  }

  if (_print_all_pool)
  {
    return _pool_allocator->GetNbPools ();
  }
  else
  {
    return 1;
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::DrawPage (GtkPrintOperation *operation,
                               GtkPrintContext   *context,
                               gint               page_nr)
{
  DrawContainerPage (operation,
                     context,
                     page_nr);

  if (   (GetStageView (operation) == STAGE_VIEW_RESULT)
      || (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))) == FALSE))
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
void PoolSupervisor::OnEndPrint (GtkPrintOperation *operation,
                                 GtkPrintContext   *context)
{
  OnAttrListUpdated ();
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
  Stage::ApplyConfig ();

  if (_displayed_pool)
  {
    OnPoolSelected (_displayed_pool);
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
    _max_score = _pool_allocator->GetMaxScore ();
  }

  Stage::SetInputProvider (input_provider);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::FillInConfig ()
{
  Stage::FillInConfig ();
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
    Pool *pool = _pool_allocator->GetPool (i);

    pool->CleanScores ();
    pool->Stuff ();
  }

  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Load (xmlNode *xml_node)
{
  LoadAttendees (NULL);

  {
    guint nb_pools = _pool_allocator->GetNbPools ();

    for (guint p = 0; p < nb_pools; p++)
    {
      Pool *pool = _pool_allocator->GetPool (p);

      OnPoolStatusUpdated (pool,
                           this);
    }
  }
}

// --------------------------------------------------------------------------------
GSList *PoolSupervisor::GetCurrentClassification ()
{
  GSList *result = NULL;

  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool   *pool           = _pool_allocator->GetPool (p);
    GSList *current_player = pool->GetFencerList ();

    while (current_player)
    {
      Player *player = (Player *) current_player->data;

      result = g_slist_prepend (result,
                                player);
      current_player = g_slist_next (current_player);
    }
  }

  result = EvaluateClassification (result,
                                   _single_owner,
                                   (GCompareDataFunc) CompareSingleClassification);

  return EvaluateClassification (result,
                                 this,
                                 (GCompareDataFunc) CompareCombinedClassification);
}

// --------------------------------------------------------------------------------
GSList *PoolSupervisor::EvaluateClassification (GSList           *list,
                                                Object           *rank_owner,
                                                GCompareDataFunc  CompareFunction)
{
  Player::AttributeId *attr_id = new Player::AttributeId ("rank", rank_owner);
  GSList              *result;
  GSList              *current;
  guint                previous_rank   = 0;
  Player              *previous_player = NULL;

  if (CompareFunction)
  {
    result = g_slist_sort_with_data (list,
                                     CompareFunction,
                                     this);
  }
  else
  {
    result = list;
  }

  current = result;
  for (guint i = 1; current; i++)
  {
    Player *player = (Player *) current->data;

    if (   previous_player
        && (Pool::ComparePlayer (player,
                                 previous_player,
                                 rank_owner,
                                 0,
                                 GetDataOwner (),
                                 Pool::WITH_CALCULUS) == 0))
    {
      player->SetAttributeValue (attr_id,
                                 previous_rank);
    }
    else
    {
      player->SetAttributeValue (attr_id,
                                 i);
      previous_rank = i;
    }

    previous_player = player;
    current = g_slist_next (current);
  }
  attr_id->Release ();

  return result;
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnToggleSingleClassification (gboolean single_selected)
{
  Classification *classification = GetClassification ();

  if (single_selected)
  {
    classification->SetDataOwner (_single_owner);
  }
  else
  {
    classification->SetDataOwner (this);
  }

  ToggleClassification (TRUE);
  classification->SortDisplay ();
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
extern "C" G_MODULE_EXPORT void on_pool_classification_toggletoolbutton_toggled (GtkToggleToolButton *widget,
                                                                                 Object              *owner)
{
  PoolSupervisor *p = dynamic_cast <PoolSupervisor *> (owner);

  p->ToggleClassification (gtk_toggle_tool_button_get_active (widget));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_stuff_toolbutton_clicked (GtkWidget *widget,
                                                             Object    *owner)
{
  PoolSupervisor *ps = dynamic_cast <PoolSupervisor *> (owner);

  ps->OnStuffClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_single_radiobutton_toggled (GtkWidget *widget,
                                                               Object    *owner)
{
  PoolSupervisor *p = dynamic_cast <PoolSupervisor *> (owner);

  p->OnToggleSingleClassification (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_all_radiobutton_toggled (GtkWidget *widget,
                                                            Object    *owner)
{
  Module *module = dynamic_cast <Module *> (owner);
  Stage  *stage  = dynamic_cast <Stage *> (owner);


  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
  {
    stage->DeactivateNbQualified ();
  }
  else
  {
    stage->ActivateNbQualified ();
  }
}
