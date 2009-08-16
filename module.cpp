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

#include "module.hpp"

// --------------------------------------------------------------------------------
Module_c::Module_c (gchar *glade_file,
                    gchar *root)
{
  _plugged_list      = NULL;
  _owner             = NULL;
  _sensitive_widgets = NULL;

  if (glade_file)
  {
    _glade = new Glade_c (glade_file);

    if (root)
    {
      _root = _glade->GetWidget (root);
    }
    else
    {
      _root = _glade->GetRootWidget ();
    }

    {
      GtkWidget *parent = gtk_widget_get_parent (_root);

      if (parent)
      {
        gtk_container_forall (GTK_CONTAINER (parent),
                              (GtkCallback) g_object_ref,
                              NULL);
        gtk_container_remove (GTK_CONTAINER (parent),
                              _root);
      }
    }
  }
}

// --------------------------------------------------------------------------------
Module_c::~Module_c ()
{
  while (_plugged_list)
  {
    Module_c *module;

    module = (Module_c *) (g_slist_nth_data (_plugged_list, 0)),
    module->UnPlug ();
  }
  g_slist_free (_plugged_list);

  UnPlug ();

  g_slist_free (_sensitive_widgets);
  Object_c::Release (_glade);
  g_object_unref (_root);
}

// --------------------------------------------------------------------------------
void Module_c::Plug (Module_c  *module,
                     GtkWidget *in)
{
  gtk_container_add (GTK_CONTAINER (in),
                     module->_root);

  module->OnPlugged ();

  _plugged_list = g_slist_append (_plugged_list,
                                  module);
  module->_owner = this;
}

// --------------------------------------------------------------------------------
void Module_c::UnPlug ()
{
  GtkWidget *parent = gtk_widget_get_parent (_root);

  if (parent)
  {
    gtk_container_remove (GTK_CONTAINER (parent),
                          _root);
  }

  if (_owner)
  {
    _owner->_plugged_list = g_slist_remove (_owner->_plugged_list,
                                            this);
    _owner = NULL;
  }

  OnUnPlugged ();
}

// --------------------------------------------------------------------------------
void Module_c::OnPlugged ()
{
}

// --------------------------------------------------------------------------------
void Module_c::OnUnPlugged ()
{
}

// --------------------------------------------------------------------------------
GtkWidget *Module_c::GetRootWidget ()
{
  return _root;
}

// --------------------------------------------------------------------------------
void Module_c::AddSensitiveWidget (GtkWidget *w)
{
  _sensitive_widgets = g_slist_append (_sensitive_widgets,
                                       w);
}

// --------------------------------------------------------------------------------
void Module_c::EnableSensitiveWidgets ()
{
  for (guint i = 0; i < g_slist_length (_sensitive_widgets); i++)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (g_slist_nth_data (_sensitive_widgets, i)),
                              true);
  }
}

// --------------------------------------------------------------------------------
void Module_c::DisableSensitiveWidgets ()
{
  for (guint i = 0; i < g_slist_length (_sensitive_widgets); i++)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (g_slist_nth_data (_sensitive_widgets, i)),
                              false);
  }
}
