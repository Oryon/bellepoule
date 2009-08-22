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

// --------------------------------------------------------------------------------
PoolSupervisor_c::PoolSupervisor_c (gchar *name)
  : Stage_c (name),
  Module_c ("pool.glade")
{
  _pool_allocator = NULL;
  _displayed_pool = NULL;

  // Callbacks binding
  {
    _glade->Bind ("print_pool_toolbutton", this);
  }

  {
    _menu_pool = gtk_menu_new ();
    gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (_glade->GetWidget ("pool_MenuToolButton")),
                                   _menu_pool);
  }
}

// --------------------------------------------------------------------------------
PoolSupervisor_c::~PoolSupervisor_c ()
{
  Object_c::Release (_pool_allocator);
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Manage (Pool_c *pool)
{
  GtkWidget *menu_item = gtk_menu_item_new_with_label (pool->GetName ());

  gtk_menu_shell_append (GTK_MENU_SHELL (_menu_pool),
                         menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "button-release-event",
                    G_CALLBACK (on_pool_selected), pool);

  g_object_set_data (G_OBJECT (menu_item),
                     "instance",
                     this);
  pool->SetData ("pool_supervisor::menu_item",
                 menu_item);

  if (_displayed_pool == NULL)
  {
    OnPoolSelected (pool);
  }
}

// --------------------------------------------------------------------------------
gboolean PoolSupervisor_c::on_pool_selected (GtkWidget      *widget,
                                             GdkEventButton *event,
                                             gpointer        user_data)
{
  PoolSupervisor_c *p = (PoolSupervisor_c *) g_object_get_data (G_OBJECT (widget),
                                                                "instance");
  p->OnPoolSelected ((Pool_c *) user_data);

  return TRUE;
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnPoolSelected (Pool_c *pool)
{
  GtkWidget *menu_item;

  if (_displayed_pool)
  {
    _displayed_pool->UnPlug ();

    menu_item = GTK_WIDGET (_displayed_pool->GetData ("pool_supervisor::menu_item"));
    gtk_widget_set_sensitive (menu_item,
                              TRUE);
  }

  {
    Plug (pool,
          _glade->GetWidget ("pool_hook"));

    menu_item = GTK_WIDGET (pool->GetData ("pool_supervisor::menu_item"));
    gtk_widget_set_sensitive (menu_item,
                              FALSE);
    _displayed_pool = pool;
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_print_pool_toolbutton_clicked (GtkWidget *widget,
                                                                  GdkEvent  *event,
                                                                  gpointer  *data)
{
  PoolSupervisor_c *p = (PoolSupervisor_c *) g_object_get_data (G_OBJECT (widget),
                                                                "instance");
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
  PoolAllocator_c *pool_allocator = dynamic_cast <PoolAllocator_c *> (GetPreviousStage ());

  if (pool_allocator)
  {
    pool_allocator->Retain ();
    _pool_allocator = pool_allocator;
    for (guint p = 0; p < _pool_allocator->GetNbPools (); p++)
    {
      Manage (_pool_allocator->GetPool (p));
    }
  }
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Load (xmlDoc *doc)
{
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::Save (xmlTextWriter *xml_writer)
{
}

// --------------------------------------------------------------------------------
void PoolSupervisor_c::OnLocked ()
{
  DisableSensitiveWidgets ();
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

    {
      GtkWidget *menu_item;

      menu_item = GTK_WIDGET (pool->GetData ("pool_supervisor::menu_item"));

      gtk_container_remove (GTK_CONTAINER (_menu_pool),
                            menu_item);
    }

    pool->ResetMatches ();
  }

  if (_displayed_pool)
  {
    _displayed_pool->UnPlug ();
    _displayed_pool = NULL;
  }
}
