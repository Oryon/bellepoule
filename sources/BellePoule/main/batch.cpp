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

#include "batch.hpp"

typedef enum
{
  NAME_str,
  SHA1_ptr
} ColumnId;

// --------------------------------------------------------------------------------
Batch::Batch (const gchar *id)
  : Module ("batch.glade")
{
  _id = (guint32) g_ascii_strtoull (id,
                                    NULL,
                                    16);

  _list_store = GTK_LIST_STORE (_glade->GetGObject ("liststore"));

  {
    GtkWidget *source = _glade->GetWidget ("treeview");

    _dnd_key = _dnd_config->CreateTarget ("bellepoule/job", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

    gtk_drag_source_set (source,
                         GDK_MODIFIER_MASK,
                         _dnd_config->GetTargetTable (),
                         _dnd_config->GetTargetTableSize (),
                         GDK_ACTION_COPY);

    ConnectDndSource (source);
  }
}

// --------------------------------------------------------------------------------
Batch::~Batch ()
{
}

// --------------------------------------------------------------------------------
guint32 Batch::GetId ()
{
  return _id;
}

// --------------------------------------------------------------------------------
void Batch::SetProperty (GKeyFile    *key_file,
                         const gchar *property)
{
  gchar    *property_widget = g_strdup_printf ("contest_%s_label", property);
  GtkLabel *label           = GTK_LABEL (_glade->GetGObject (property_widget));
  gchar    *value;

  value = g_key_file_get_string (key_file,
                                 "Contest",
                                 property,
                                 NULL);
  gtk_label_set_text (label,
                      gettext (value));

  g_free (property_widget);
  g_free (value);
}

// --------------------------------------------------------------------------------
void Batch::SetProperties (GKeyFile *key_file)
{
  SetProperty (key_file, "gender");
  SetProperty (key_file, "weapon");
  SetProperty (key_file, "category");

  {
    GdkColor  *gdk_color;
    GtkWidget *tab   = _glade->GetWidget ("notebook_title");
    gchar     *color = g_key_file_get_string (key_file,
                                              "Contest",
                                              "color",
                                              NULL);

    gdk_color = g_new (GdkColor, 1);

    gdk_color_parse (color,
                     gdk_color);

    gtk_widget_modify_bg (tab,
                          GTK_STATE_NORMAL,
                          gdk_color);
    gtk_widget_modify_bg (tab,
                          GTK_STATE_ACTIVE,
                          gdk_color);

    gdk_color_free (gdk_color);
    g_free (color);
  }
}

// --------------------------------------------------------------------------------
void Batch::AttachTo (GtkNotebook *to)
{
  gtk_notebook_append_page (to,
                            GetRootWidget (),
                            _glade->GetWidget ("notebook_title"));
}

// --------------------------------------------------------------------------------
gboolean Batch::HasJob (GChecksum *sha1)
{
  GtkTreeIter iter;
  gboolean    iter_is_valid;

  iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_list_store),
                                                 &iter);
  while (iter_is_valid)
  {
    GChecksum *current_sha1;

    gtk_tree_model_get (GTK_TREE_MODEL (_list_store),
                        &iter,
                        SHA1_ptr, &current_sha1,
                        -1);

    if (strcmp (g_checksum_get_string (sha1),
                g_checksum_get_string (current_sha1)) == 0)
    {
      return TRUE;
    }

    iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_list_store),
                                              &iter);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Batch::LoadJob (xmlNode   *xml_node,
                     GChecksum *sha1)
{
  if (HasJob (sha1) == FALSE)
  {
    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (strcmp ((char *) n->name, "TourDePoules") == 0)
        {
          GtkLabel *label = GTK_LABEL (_glade->GetWidget ("batch_label"));
          gchar    *name;

          name = g_strdup_printf ("%s %s",
                                  gettext ("Pool"),
                                  (gchar *) xmlGetProp (n, BAD_CAST "ID"));
          gtk_label_set_text (label, name);
        }
        else if (strcmp ((char *) n->name, "Poule") == 0)
        {
          GtkTreeIter  iter;
          gchar       *attr;
          gchar       *name;

          attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");
          if (attr)
          {
            name = g_strdup_printf ("%s%s", gettext ("Pool #"), attr);
          }
          else
          {
            name = g_strdup (gettext ("Pool"));
          }

          gtk_list_store_append (_list_store,
                                 &iter);

          gtk_list_store_set (_list_store, &iter,
                              NAME_str, name,
                              SHA1_ptr, g_checksum_copy (sha1),
                              -1);
        }

        LoadJob (n->children,
                 sha1);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Batch::OnDragDataGet (GtkWidget        *widget,
                           GdkDragContext   *drag_context,
                           GtkSelectionData *data,
                           guint             key,
                           guint             time)
{
  if (key == _dnd_key)
  {
    gtk_selection_data_set (data,
                            gtk_selection_data_get_target (data),
                            32,
                            (guchar *) &_id,
                            sizeof (_id));
  }
}
