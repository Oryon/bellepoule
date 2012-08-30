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

#include "table_set_border.hpp"

typedef enum
{
  NAME_COLUMN_str,
  STATUS_COLUMN_str,
  TABLE_COLUMN_ptr
} ColumnId;

// --------------------------------------------------------------------------------
TableSetBorder::TableSetBorder (Object    *owner,
                                GCallback  callback,
                                GtkWidget *container,
                                GtkWidget *widget)
: Object ("TableSetBorder")
{
  _owner     = owner;
  _callback  = callback;
  _liststore = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
  _container = GTK_CONTAINER (container);
  _widget    = widget;

  {
    g_object_ref (_widget);
    gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (_widget)),
                          _widget);
  }
}


// --------------------------------------------------------------------------------
TableSetBorder::~TableSetBorder ()
{
  g_object_unref (_widget);
}

// --------------------------------------------------------------------------------
void TableSetBorder::Plug ()
{
  gtk_container_add (_container,
                     _widget);
}

// --------------------------------------------------------------------------------
void TableSetBorder::UnPlug ()
{
  if (gtk_widget_get_parent (_widget))
  {
    gtk_container_remove (_container,
                          _widget);
  }
}

// --------------------------------------------------------------------------------
void TableSetBorder::Mute ()
{
  g_signal_handlers_disconnect_by_func (_widget,
                                        (void *) _callback,
                                        _owner);
  gtk_list_store_clear (_liststore);
}

// --------------------------------------------------------------------------------
void TableSetBorder::AddTable (Table *table)
{
  gchar       *text = table->GetImage ();
  GtkTreeIter  iter;

  gtk_list_store_prepend (_liststore,
                          &iter);
  gtk_list_store_set (_liststore, &iter,
                      STATUS_COLUMN_str, GTK_STOCK_EXECUTE,
                      NAME_COLUMN_str,   text,
                      TABLE_COLUMN_ptr,  table,
                      -1);
  g_free (text);
}

// --------------------------------------------------------------------------------
void TableSetBorder::UnMute ()
{
  g_signal_connect (_widget,
                    "changed",
                    _callback,
                    _owner);
}

// --------------------------------------------------------------------------------
void TableSetBorder::SelectTable (guint table)
{
  gtk_combo_box_set_active (GTK_COMBO_BOX (_widget),
                            table);
}

// --------------------------------------------------------------------------------
Table *TableSetBorder::GetSelectedTable ()
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (_widget),
                                     &iter))
  {
    Table *table;

    gtk_tree_model_get (GTK_TREE_MODEL (_liststore),
                        &iter,
                        TABLE_COLUMN_ptr, &table,
                        -1);
    return table;
  }
  return NULL;
}

// --------------------------------------------------------------------------------
void TableSetBorder::SetTableIcon (guint        table,
                                   const gchar *icon)
{
  GtkTreeIter iter;

  if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_liststore),
                                     &iter,
                                     NULL,
                                     table) == TRUE)
  {
    gtk_list_store_set (_liststore, &iter,
                        STATUS_COLUMN_str, icon,
                        -1);
  }
}
