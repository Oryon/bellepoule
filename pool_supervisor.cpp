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

const gchar *PoolSupervisor::_class_name     = "Poules";
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

  _pool_liststore = GTK_LIST_STORE (_glade->GetObject ("pool_liststore"));

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
    AddSensitiveWidget (_glade->GetWidget ("qualified_fencer_spinbutton"));
    AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

    LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
    LockOnClassification (_glade->GetWidget ("pool_combobox"));
  }

  // Filter
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
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

  // Classification filter
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
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
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Init ()
{
  RegisterStageClass (_class_name,
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
}

// --------------------------------------------------------------------------------
void PoolSupervisor::OnUnPlugged ()
{
  //Object::TryToRelease (_pool_allocator);
  //_pool_allocator = NULL;
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Display ()
{
  for (guint i = 0; i < _pool_allocator->GetNbPools (); i++)
  {
    Pool *pool;

    pool = _pool_allocator->GetPool (i);
    pool->SortPlayers ();
  }

  OnPoolSelected (0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Garnish ()
{
  _displayed_pool = _pool_allocator->GetPool (0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::LoadConfiguration (xmlNode *xml_node)
{
  Stage::LoadConfiguration (xml_node);

  if (_max_score)
  {
    _max_score->Load (xml_node);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Load (xmlNode *xml_node)
{
  LoadConfiguration (xml_node);

  Load (xml_node,
        0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Load (xmlNode *xml_node,
                           guint    current_pool_index)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, "match_list") == 0)
      {
        Pool *current_pool = _pool_allocator->GetPool (current_pool_index);

        current_pool->Load (n->children,
                            _attendees);
        current_pool->RefreshScoreData ();

        current_pool_index++;
      }
      else if (strcmp ((char *) n->name, _xml_class_name) != 0)
      {
        if (_pool_allocator)
        {
          _displayed_pool = _pool_allocator->GetPool (0);
        }
        return;
      }

      Load (n->children,
            current_pool_index);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::SaveConfiguration (xmlTextWriter *xml_writer)
{
  Stage::SaveConfiguration (xml_writer);

  if (_max_score)
  {
    _max_score->Save (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);

  if (_pool_allocator)
  {
    for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
    {
      Pool *pool;

      pool = _pool_allocator->GetPool (p);
      pool->Save (xml_writer);
    }
  }

  xmlTextWriterEndElement (xml_writer);
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
      pool->ResetMatches ();
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
      classification->Print ();
    }
  }
  else if (_displayed_pool)
  {
    _displayed_pool->Print ();
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
    GtkWidget *name_w   = _glade->GetWidget ("name_entry");
    gchar     *name     = (gchar *) gtk_entry_get_text (GTK_ENTRY (name_w));
    Stage     *previous = GetPreviousStage ();

    SetName (name);
    previous->SetName (name);
  }

  {
    GtkWidget *max_score_w = _glade->GetWidget ("max_score_entry");

    if (max_score_w)
    {
      gchar *str = (gchar *) gtk_entry_get_text (GTK_ENTRY (max_score_w));

      if (str)
      {
        _max_score->_value = atoi (str);
      }

      if (_displayed_pool)
      {
        OnPoolSelected (_displayed_pool);
      }
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

    if (strcmp (provider_class->_name, PoolAllocator::_class_name) == 0)
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
  gchar *text = g_strdup_printf ("%d", _max_score->_value);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("max_score_entry")),
                      text);
  g_free (text);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("name_entry")),
                      GetName ());
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
    pool->ResetMatches ();
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

      result = g_slist_append (result,
                               player);
    }
  }
  return g_slist_sort_with_data (result,
                                 (GCompareDataFunc) ComparePlayer,
                                 this);
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
