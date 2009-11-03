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

const gchar *PoolSupervisor_c::_class_name     = "pool";
const gchar *PoolSupervisor_c::_xml_class_name = "pool_stage";

typedef enum
{
  NAME_COLUMN,
  STATUS_COLUMN
} ColumnId;

// --------------------------------------------------------------------------------
PoolSupervisor_c::PoolSupervisor_c (StageClass *stage_class)
  : Stage_c (stage_class),
  Module_c ("pool_supervisor.glade")
{
  _pool_allocator = NULL;
  _displayed_pool = NULL;
  _max_score      = 5;

  _pool_liststore = GTK_LIST_STORE (_glade->GetObject ("pool_liststore"));

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
    AddSensitiveWidget (_glade->GetWidget ("qualified_fencer_spinbutton"));
  }

  // Filter
  {
    ShowAttribute ("name");
  }
}

// --------------------------------------------------------------------------------
PoolSupervisor_c::~PoolSupervisor_c ()
{
  Object_c::Release (_pool_allocator);

  gtk_list_store_clear (_pool_liststore);
  g_object_unref (_pool_liststore);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      EDITABLE);
}

// --------------------------------------------------------------------------------
Stage_c *PoolSupervisor_c::CreateInstance (StageClass *stage_class)
{
  return new PoolSupervisor_c (stage_class);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Manage (Pool_c *pool)
{
  GtkTreeIter iter;

  gtk_list_store_append (_pool_liststore,
                         &iter);

  gtk_list_store_set (_pool_liststore, &iter,
                      NAME_COLUMN, pool->GetName (),
                      -1);

  pool->SetRandSeed (0);
  pool->SetMaxScore (_max_score);
  pool->SetStatusCbk ((Pool_c::StatusCbk) OnPoolStatusUpdated,
                      this);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnPoolStatusUpdated (Pool_c           *pool,
                                            PoolSupervisor_c *ps)
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
                          STATUS_COLUMN, "",
                          -1);
    }
  }

  ps->SignalStatusUpdate ();
}

// --------------------------------------------------------------------------------
gboolean PoolSupervisor_c::IsOver ()
{
  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool_c *pool;

    pool = _pool_allocator->GetPool (p);
    if (pool->IsOver () == FALSE)
    {
      return FALSE;
    }
  }
  return TRUE;
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnPoolSelected (gint index)
{
  if ((index >= 0) && (_pool_allocator))
  {
    OnPoolSelected (_pool_allocator->GetPool (index));
    gtk_combo_box_set_active (GTK_COMBO_BOX (_glade->GetWidget ("pool_combobox")),
                              index);
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnPoolSelected (Pool_c *pool)
{
  if (_displayed_pool)
  {
    _displayed_pool->UnPlug ();
    _displayed_pool = NULL;
  }

  if (pool)
  {
    pool->CloneFilterList (this);
    Plug (pool,
          _glade->GetWidget ("pool_hook"));

    _displayed_pool = pool;
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_print_pool_toolbutton_clicked (GtkWidget *widget,
                                                                  Object_c  *owner)
{
  PoolSupervisor_c *p = dynamic_cast <PoolSupervisor_c *> (owner);

  p->OnPrintPoolToolbuttonClicked ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnPrintPoolToolbuttonClicked ()
{
  if (_displayed_pool)
  {
    _displayed_pool->Print ();
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Enter ()
{
  RetrievePools ();

  OnPoolSelected (0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnPlugged ()
{
  OnPoolSelected (0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::RetrievePools ()
{
  _pool_allocator = dynamic_cast <PoolAllocator_c *> (GetPreviousStage ());

  if (_pool_allocator)
  {
    _pool_allocator->Retain ();

    for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
    {
      Manage (_pool_allocator->GetPool (p));
    }
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Load (xmlNode *xml_node)
{
  static guint current_pool_index = 0;
  Stage_c *previous_stage = GetPreviousStage ();

  if (_pool_allocator == NULL)
  {
    current_pool_index = 0;
    RetrievePools ();
  }

  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, "match_list") == 0)
      {
        Pool_c *current_pool = _pool_allocator->GetPool (current_pool_index);

        current_pool->Load (n->children,
                            previous_stage->GetResult ());

        current_pool_index++;
      }
      else if (strcmp ((char *) n->name, _xml_class_name) != 0)
      {
        return;
      }

      Load (n->children);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "name",
                                     "%s", GetName ());

  if (_pool_allocator)
  {
    for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
    {
      Pool_c *pool;

      pool = _pool_allocator->GetPool (p);
      pool->Save (xml_writer);
    }
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnLocked ()
{
  DisableSensitiveWidgets ();

  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool_c *pool;

    pool = _pool_allocator->GetPool (p);
    pool->Lock ();
    for (guint i = 0; i < pool->GetNbPlayers (); i++)
    {
      Player_c *player;

      player = pool->GetPlayer (i);

      _result = g_slist_append (_result,
                                player);
    }
  }
  _result = g_slist_sort_with_data (_result,
                                    (GCompareDataFunc) Pool_c::ComparePlayer,
                                    0);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnUnLocked ()
{
  EnableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Wipe ()
{
  for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
  {
    Pool_c *pool;

    pool = _pool_allocator->GetPool (p);
    pool->ResetMatches ();
  }

  if (_displayed_pool)
  {
    _displayed_pool->UnPlug ();
    _displayed_pool = NULL;
  }

  gtk_list_store_clear (_pool_liststore);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::ApplyConfig ()
{
  {
    GtkWidget *name_w   = _glade->GetWidget ("name_entry");
    gchar     *name     = (gchar *) gtk_entry_get_text (GTK_ENTRY (name_w));
    Stage_c   *previous = GetPreviousStage ();

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
        _max_score = atoi (str);

        if (_pool_allocator)
        {
          for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
          {
            Pool_c *pool;

            pool = _pool_allocator->GetPool (p);
            pool->SetMaxScore (_max_score);
          }
        }
      }
      if (_displayed_pool)
      {
        OnPoolSelected (_displayed_pool);
      }
    }
  }
}

// --------------------------------------------------------------------------------
Stage_c *PoolSupervisor_c::GetInputProvider ()
{
  Stage_c *previous = GetPreviousStage ();

  if (previous)
  {
    Stage_c::StageClass *provider_class = previous->GetClass ();

    if (strcmp (provider_class->_name, PoolAllocator_c::_class_name) == 0)
    {
      return previous;
    }
  }

  return Stage_c::CreateInstance (PoolAllocator_c::_xml_class_name);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::FillInConfig ()
{
  gchar *text = g_strdup_printf ("%d", _max_score);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("max_score_entry")),
                      text);
  g_free (text);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("name_entry")),
                      GetName ());
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnAttrListUpdated ()
{
  if (_displayed_pool)
  {
    OnPoolSelected (_displayed_pool);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pool_filter_toollbutton_clicked (GtkWidget *widget,
                                                                    Object_c  *owner)
{
  PoolSupervisor_c *p = dynamic_cast <PoolSupervisor_c *> (owner);

  p->SelectAttributes ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pool_combobox_changed (GtkWidget *widget,
                                                          Object_c  *owner)
{
  PoolSupervisor_c *p          = dynamic_cast <PoolSupervisor_c *> (owner);
  gint              pool_index = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

  p->OnPoolSelected (pool_index);
}
