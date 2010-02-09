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

#include "attribute.hpp"

#include "module.hpp"

GKeyFile *Module_c::_config_file = NULL;

// --------------------------------------------------------------------------------
Module_c::Module_c (gchar *glade_file,
                    gchar *root)
{
  _plugged_list = NULL;
  _owner        = NULL;
  _root         = NULL;
  _glade        = NULL;
  _toolbar      = NULL;
  _filter       = NULL;

  _sensitivity_trigger = new SensitivityTrigger ();

  if (glade_file)
  {
    _glade = new Glade_c (glade_file,
                          this);

    if (root)
    {
      _root = _glade->GetWidget (root);
    }
    else
    {
      _root = _glade->GetRootWidget ();
    }

    _glade->DetachFromParent (_root);
  }

  {
    _config_widget = _glade->GetWidget ("stage_configuration");

    if (_config_widget)
    {
      g_object_ref (_config_widget);
      gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (_config_widget)),
                                           _config_widget);
    }
  }
}

// --------------------------------------------------------------------------------
Module_c::~Module_c ()
{
  while (_plugged_list)
  {
    Module_c *module;

    module = (Module_c *) (g_slist_nth_data (_plugged_list, 0));
    module->UnPlug ();
  }

  UnPlug ();

  _sensitivity_trigger->Release ();

  Object_c::TryToRelease (_glade);
  g_object_unref (_root);

  if (_config_widget)
  {
    g_object_unref (_config_widget);
  }

  Object_c::TryToRelease (_filter);
}

// --------------------------------------------------------------------------------
GtkWidget *Module_c::GetConfigWidget ()
{
  return _config_widget;
}

// --------------------------------------------------------------------------------
GtkWidget *Module_c::GetWidget (gchar *name)
{
  return _glade->GetWidget (name);
}

// --------------------------------------------------------------------------------
void Module_c::SetFilter (Filter *filter)
{
  _filter = filter;

  if (_filter)
  {
    _filter->Retain ();
  }
}

// --------------------------------------------------------------------------------
void Module_c::SelectAttributes ()
{
  _filter->SelectAttributes ();
}

// --------------------------------------------------------------------------------
void Module_c::Plug (Module_c   *module,
                     GtkWidget  *in,
                     GtkToolbar *toolbar)
{
  if (in)
  {
    gtk_container_add (GTK_CONTAINER (in),
                       module->_root);

    module->_toolbar = toolbar;

    module->OnPlugged ();

    _plugged_list = g_slist_append (_plugged_list,
                                    module);
    module->_owner = this;
  }
}

// --------------------------------------------------------------------------------
void Module_c::UnPlug ()
{
  if (_root)
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
}

// --------------------------------------------------------------------------------
GtkWidget *Module_c::GetRootWidget ()
{
  return _root;
}

// --------------------------------------------------------------------------------
GtkToolbar *Module_c::GetToolbar ()
{
  return _toolbar;
}

// --------------------------------------------------------------------------------
void Module_c::AddSensitiveWidget (GtkWidget *w)
{
  _sensitivity_trigger->AddWidget (w);
}

// --------------------------------------------------------------------------------
void Module_c::EnableSensitiveWidgets ()
{
  _sensitivity_trigger->SwitchOn ();
}

// --------------------------------------------------------------------------------
void Module_c::DisableSensitiveWidgets ()
{
  _sensitivity_trigger->SwitchOff ();
}

// --------------------------------------------------------------------------------
void Module_c::SetCursor (GdkCursorType cursor_type)
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new (cursor_type);
  gdk_window_set_cursor (gtk_widget_get_window (_root),
                         cursor);
  gdk_cursor_unref (cursor);
}

// --------------------------------------------------------------------------------
void Module_c::ResetCursor ()
{
  gdk_window_set_cursor (gtk_widget_get_window (_root),
                         NULL);
}
