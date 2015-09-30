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

#include "job.hpp"
#include "batch.hpp"

typedef enum
{
  NAME_str,
  JOB_ptr,
  JOB_visibility
} ColumnId;

// --------------------------------------------------------------------------------
Batch::Batch (const gchar *id,
              Listener    *listener)
  : Module ("batch.glade")
{
  _name     = NULL;
  _listener = listener;

  _id = (guint32) g_ascii_strtoull (id,
                                    NULL,
                                    16);

  _job_store = GTK_LIST_STORE (_glade->GetGObject ("liststore"));

  {
    GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (_glade->GetGObject ("treemodelfilter"));

    gtk_tree_model_filter_set_visible_column (filter,
                                              JOB_visibility);
  }

  _gdk_color = NULL;

  {
    GtkWidget *source = _glade->GetWidget ("treeview");

    _dnd_key = _dnd_config->AddTarget ("bellepoule/job", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

    _dnd_config->SetOnAWidgetSrc (source,
                                  GDK_MODIFIER_MASK,
                                  GDK_ACTION_COPY);

    ConnectDndSource (source);
  }
}

// --------------------------------------------------------------------------------
Batch::~Batch ()
{
  gdk_color_free (_gdk_color);
  g_free (_name);

  {
    GtkTreeIter iter;
    gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_job_store),
                                                               &iter);

    while (iter_is_valid)
    {
      Job *job;

      gtk_tree_model_get (GTK_TREE_MODEL (_job_store),
                          &iter,
                          JOB_ptr, &job,
                          -1);

      job->Release ();

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                &iter);
    }

    gtk_list_store_clear (_job_store);
  }
}

// --------------------------------------------------------------------------------
guint32 Batch::GetId ()
{
  return _id;
}

// --------------------------------------------------------------------------------
const gchar *Batch::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
void Batch::SetProperty (Net::Message *message,
                         const gchar  *property)
{
  gchar    *property_widget = g_strdup_printf ("contest_%s_label", property);
  GtkLabel *label           = GTK_LABEL (_glade->GetGObject (property_widget));
  gchar    *value;

  value = message->GetString (property);
  gtk_label_set_text (label,
                      gettext (value));

  g_free (property_widget);
  g_free (value);
}

// --------------------------------------------------------------------------------
void Batch::SetProperties (Net::Message *message)
{
  SetProperty (message, "gender");
  SetProperty (message, "weapon");
  SetProperty (message, "category");

  {
    GtkWidget *tab   = _glade->GetWidget ("notebook_title");
    gchar     *color = message->GetString ("color");

    _gdk_color = g_new (GdkColor, 1);

    gdk_color_parse (color,
                     _gdk_color);

    gtk_widget_modify_bg (tab,
                          GTK_STATE_NORMAL,
                          _gdk_color);
    gtk_widget_modify_bg (tab,
                          GTK_STATE_ACTIVE,
                          _gdk_color);

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
GSList *Batch::GetCurrentSelection ()
{
  GSList           *result    = NULL;
  GtkTreeView      *tree_view = GTK_TREE_VIEW (_glade->GetWidget ("treeview"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);

  if (selection)
  {
    GList        *current;
    GtkTreeModel *model          = gtk_tree_view_get_model (tree_view);
    GList        *selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                                         NULL);

    current = selection_list;
    while (current)
    {
      GtkTreeIter  iter;
      Job         *job;

      gtk_tree_model_get_iter (model,
                               &iter,
                               (GtkTreePath *) current->data);
      gtk_tree_model_get (model, &iter,
                          JOB_ptr,
                          &job, -1);

      result = g_slist_append (result,
                               job);

      current = g_list_next (current);
    }

    g_list_free_full (selection_list,
                      (GDestroyNotify) gtk_tree_path_free);
  }

  return result;
}

// --------------------------------------------------------------------------------
void Batch::SetVisibility (Job      *job,
                           gboolean  visibility)
{
  GtkTreeIter iter;
  gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_job_store),
                                                             &iter);

  while (iter_is_valid)
  {
    Job *current_job;

    gtk_tree_model_get (GTK_TREE_MODEL (_job_store),
                        &iter,
                        JOB_ptr, &current_job,
                        -1);

    if (current_job == job)
    {
      gtk_list_store_set (_job_store, &iter,
                          JOB_visibility, visibility,
                          -1);
      break;
    }

    iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                              &iter);
  }
}

// --------------------------------------------------------------------------------
GList *Batch::RetreiveJobList ()
{
  GList       *list = NULL;
  GtkTreeIter  iter;
  gboolean     iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_job_store),
                                                              &iter);

  while (iter_is_valid)
  {
    Job      *current_job;
    gboolean  visibility;

    gtk_tree_model_get (GTK_TREE_MODEL (_job_store),
                        &iter,
                        JOB_ptr,        &current_job,
                        JOB_visibility, &visibility,
                        -1);

    if (visibility)
    {
      list = g_list_append (list,
                            current_job);
    }

    iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                              &iter);
  }

  return list;
}

// --------------------------------------------------------------------------------
void Batch::RemoveJob (Net::Message *message)
{
  GtkTreeIter iter;
  gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_job_store),
                                                             &iter);

  while (iter_is_valid)
  {
    Job *current_job;

    gtk_tree_model_get (GTK_TREE_MODEL (_job_store),
                        &iter,
                        JOB_ptr, &current_job,
                        -1);

    if (current_job->GetUUID () == current_job->GetUUID ())
    {
      current_job->Release ();
      gtk_list_store_remove (_job_store,
                             &iter);
      break;
    }

    iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                              &iter);
  }
}

// --------------------------------------------------------------------------------
void Batch::Load (Net::Message *message)
{
  gchar     *xml = message->GetString ("xml");
  xmlDocPtr  doc = xmlParseMemory (xml, strlen (xml));

  if (doc)
  {
    xmlXPathInit ();

    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);
      xmlXPathObject  *xml_object;
      xmlNodeSet      *xml_nodeset;

      xml_object = xmlXPathEval (BAD_CAST "/Poule", xml_context);
      xml_nodeset = xml_object->nodesetval;

      if (xml_nodeset->nodeNr == 1)
      {
        LoadJob (xml_nodeset->nodeTab[0],
                 message->GetUUID ());
      }

      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }
    xmlFreeDoc (doc);
  }

  g_free (xml);

  g_free (_name);
  _name = message->GetString ("round");
}

// --------------------------------------------------------------------------------
void Batch::LoadJob (xmlNode *xml_node,
                     guint    uuid)
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
        g_free (name);
      }
      else if (strcmp ((char *) n->name, "Poule") == 0)
      {
        GtkTreeIter  iter;
        gchar       *attr;
        gchar       *name;
        Job         *job = new Job (this, uuid, _gdk_color);

        attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");
        if (attr)
        {
          name = g_strdup_printf ("%s%s", gettext ("Pool #"), attr);
        }
        else
        {
          name = g_strdup (gettext ("Pool"));
        }

        gtk_list_store_append (_job_store,
                               &iter);

        job->SetName (name);
        gtk_list_store_set (_job_store, &iter,
                            NAME_str,       name,
                            JOB_ptr,        job,
                            JOB_visibility, 1,
                            -1);
        g_free (name);
      }

      LoadJob (n->children,
               uuid);
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

// --------------------------------------------------------------------------------
void Batch::OnAssign ()
{
  _listener->OnBatchAssignmentRequest (this);
}

// --------------------------------------------------------------------------------
void Batch::OnCancelAssign ()
{
  _listener->OnBatchAssignmentRequest (this);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_assign_toolbutton_clicked (GtkToolButton *widget,
                                                              Object        *owner)
{
  Batch *b = dynamic_cast <Batch *> (owner);

  b->OnAssign ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_cancel_toolbutton_clicked (GtkToolButton *widget,
                                                              Object        *owner)
{
  Batch *b = dynamic_cast <Batch *> (owner);

  b->OnCancelAssign ();
}
