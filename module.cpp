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

GKeyFile *Module::_config_file = NULL;

// --------------------------------------------------------------------------------
Module::Module (gchar *glade_file,
                gchar *root)
{
  _plugged_list = NULL;
  _owner        = NULL;
  _data_owner   = this;
  _root         = NULL;
  _glade        = NULL;
  _toolbar      = NULL;
  _filter       = NULL;

  _sensitivity_trigger = new SensitivityTrigger ();

  if (glade_file)
  {
    _glade = new Glade (glade_file,
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
Module::~Module ()
{
  while (_plugged_list)
  {
    Module *module;

    module = (Module *) (g_slist_nth_data (_plugged_list, 0));
    module->UnPlug ();
  }

  UnPlug ();

  _sensitivity_trigger->Release ();

  Object::TryToRelease (_glade);
  g_object_unref (_root);

  if (_config_widget)
  {
    g_object_unref (_config_widget);
  }

  Object::TryToRelease (_filter);
}

// --------------------------------------------------------------------------------
void Module::SetDataOwner (Object *data_owner)
{
  _data_owner = data_owner;
}

// --------------------------------------------------------------------------------
Object *Module::GetDataOwner ()
{
  return _data_owner;
}

// --------------------------------------------------------------------------------
GtkWidget *Module::GetConfigWidget ()
{
  return _config_widget;
}

// --------------------------------------------------------------------------------
GtkWidget *Module::GetWidget (gchar *name)
{
  return _glade->GetWidget (name);
}

// --------------------------------------------------------------------------------
void Module::SetFilter (Filter *filter)
{
  _filter = filter;

  if (_filter)
  {
    _filter->Retain ();
  }
}

// --------------------------------------------------------------------------------
void Module::SelectAttributes ()
{
  _filter->SelectAttributes ();
}

// --------------------------------------------------------------------------------
void Module::Plug (Module     *module,
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
void Module::UnPlug ()
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
GtkWidget *Module::GetRootWidget ()
{
  return _root;
}

// --------------------------------------------------------------------------------
GtkToolbar *Module::GetToolbar ()
{
  return _toolbar;
}

// --------------------------------------------------------------------------------
void Module::AddSensitiveWidget (GtkWidget *w)
{
  _sensitivity_trigger->AddWidget (w);
}

// --------------------------------------------------------------------------------
void Module::EnableSensitiveWidgets ()
{
  _sensitivity_trigger->SwitchOn ();
}

// --------------------------------------------------------------------------------
void Module::DisableSensitiveWidgets ()
{
  _sensitivity_trigger->SwitchOff ();
}

// --------------------------------------------------------------------------------
void Module::SetCursor (GdkCursorType cursor_type)
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new (cursor_type);
  gdk_window_set_cursor (gtk_widget_get_window (_root),
                         cursor);
  gdk_cursor_unref (cursor);
}

// --------------------------------------------------------------------------------
void Module::ResetCursor ()
{
  gdk_window_set_cursor (gtk_widget_get_window (_root),
                         NULL);
}
