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

#include "util/player.hpp"
#include "actors/player_factory.hpp"

#include "job.hpp"
#include "batch.hpp"

namespace Marshaller
{
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
    _name           = NULL;
    _listener       = listener;
    _scheduled_list = NULL;
    _pending_list   = NULL;
    _fencer_list    = NULL;
    _weapon         = NULL;

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
      GtkTreeView *treeview = GTK_TREE_VIEW (_glade->GetWidget ("treeview"));

      gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview),
                                   GTK_SELECTION_MULTIPLE);
    }

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

    g_list_free (_scheduled_list);
    g_list_free (_pending_list);

    g_list_free_full (_fencer_list,
                      (GDestroyNotify) Object::TryToRelease);

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

    g_free (_weapon);
  }

  // --------------------------------------------------------------------------------
  guint32 Batch::GetId ()
  {
    return _id;
  }

  // --------------------------------------------------------------------------------
  GdkColor *Batch::GetColor ()
  {
    return _gdk_color;
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
    gchar         *property_widget = g_strdup_printf ("contest_%s_label", property);
    GtkLabel      *label           = GTK_LABEL (_glade->GetGObject (property_widget));
    AttributeDesc *desc            = AttributeDesc::GetDescFromCodeName (property);
    gchar         *xml             = message->GetString (property);
    gchar         *image           = desc->GetUserImage (xml, AttributeDesc::LONG_TEXT);

    gtk_label_set_text (label,
                        gettext (image));

    g_free (image);
    g_free (property_widget);
    g_free (xml);
  }

  // --------------------------------------------------------------------------------
  void Batch::SetProperties (Net::Message *message)
  {
    SetProperty (message, "gender");
    SetProperty (message, "weapon");
    SetProperty (message, "category");

    _weapon = message->GetString ("weapon");

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
  const gchar *Batch::GetWeaponCode ()
  {
    return _weapon;
  }

  // --------------------------------------------------------------------------------
  void Batch::AttachTo (GtkNotebook *to)
  {
    GtkWidget *root = GetRootWidget ();

    g_object_set_data (G_OBJECT (root),
                       "batch",
                       this);
    gtk_notebook_append_page (to,
                              root,
                              _glade->GetWidget ("notebook_title"));
  }

  // --------------------------------------------------------------------------------
  GList *Batch::GetCurrentSelection ()
  {
    GList            *result    = NULL;
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

        result = g_list_append (result,
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

        if (visibility == FALSE)
        {
          GList *node = g_list_find (_pending_list,
                                     job);
          _pending_list = g_list_delete_link (_pending_list,
                                              node);

          _scheduled_list = g_list_insert_sorted (_scheduled_list,
                                                  job,
                                                  (GCompareFunc) Job::CompareStartTime);
        }
        else
        {
          GList *node = g_list_find (_scheduled_list,
                                     job);
          _scheduled_list = g_list_delete_link (_scheduled_list,
                                                node);

          _pending_list = g_list_insert_sorted (_pending_list,
                                                job,
                                                (GCompareFunc) Job::CompareSiblingOrder);
        }
        break;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                &iter);
    }
  }

  // --------------------------------------------------------------------------------
  GList *Batch::GetScheduledJobs ()
  {
    return _scheduled_list;
  }

  // --------------------------------------------------------------------------------
  GList *Batch::GetPendingJobs ()
  {
    return _pending_list;
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

      if (message->GetUUID () == current_job->GetUUID ())
      {
        gtk_list_store_remove (_job_store,
                               &iter);

        {
          GList *node = g_list_find (_pending_list,
                                     current_job);
          _pending_list = g_list_delete_link (_pending_list,
                                              node);
        }

        {
          GList *node = g_list_find (_scheduled_list,
                                     current_job);
          _scheduled_list = g_list_delete_link (_scheduled_list,
                                                node);
        }

        current_job->Release ();

        break;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                &iter);
    }
  }

  // --------------------------------------------------------------------------------
  Job *Batch::GetJob (guint uuid)
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

      if (current_job->GetUUID () == uuid)
      {
        return current_job;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                &iter);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Batch::Load (Net::Message *message)
  {
    if (GetJob (message->GetUUID ()) == NULL)
    {
      gchar     *xml = message->GetString ("xml");
      xmlDocPtr  doc = xmlParseMemory (xml, strlen (xml));

      printf ("%s\n", xml);
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
  }

  // --------------------------------------------------------------------------------
  void Batch::LoadJob (xmlNode *xml_node,
                       guint    uuid,
                       Job     *job)
  {
    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (g_strcmp0 ((char *) n->name, "TourDePoules") == 0)
        {
          GtkLabel *label = GTK_LABEL (_glade->GetWidget ("batch_label"));
          gchar    *name;

          name = g_strdup_printf ("%s %s",
                                  gettext ("Pool"),
                                  (gchar *) xmlGetProp (n, BAD_CAST "ID"));
          gtk_label_set_text (label, name);
          g_free (name);
        }
        else if (g_strcmp0 ((char *) n->name, "Poule") == 0)
        {
          GtkTreeIter  iter;
          gchar       *attr;
          gchar       *name;
          guint        sibling_order = g_list_length (_pending_list);

          job = new Job (this, uuid, sibling_order, _gdk_color);

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

          _pending_list = g_list_append (_pending_list,
                                         job);
        }
        else if (g_strcmp0 ((char *) n->name, "Tireur") == 0)
        {
          job->AddFencer (GetFencer (atoi ((gchar *) xmlGetProp (n, BAD_CAST "REF"))));
        }

        LoadJob (n->children,
                 uuid,
                 job);
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
    _listener->OnBatchAssignmentCancel (this);
  }

  // --------------------------------------------------------------------------------
  void Batch::ManageFencer (Net::Message *message)
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

        xml_object = xmlXPathEval (BAD_CAST "/Tireur", xml_context);
        xml_nodeset = xml_object->nodesetval;

        if (xml_nodeset->nodeNr == 1)
        {
          Player *fencer = PlayerFactory::CreatePlayer ("Fencer");

          fencer->Load (xml_nodeset->nodeTab[0]);

          fencer->SetData (this,
                           "uuid",
                           GUINT_TO_POINTER (message->GetUUID ()));

          _fencer_list = g_list_prepend (_fencer_list,
                                         fencer);
        }

        xmlXPathFreeObject  (xml_object);
        xmlXPathFreeContext (xml_context);
      }
      xmlFreeDoc (doc);
    }

    g_free (xml);
  }

  // --------------------------------------------------------------------------------
  void Batch::DeleteFencer (Net::Message *message)
  {
    guint  id      = message->GetUUID ();
    GList *current = _fencer_list;

    while (current)
    {
      Player *fencer     = (Player *) current->data;
      guint   current_id = fencer->GetIntData (this, "uuid");

      if (current_id == id)
      {
        break;
      }

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  Player *Batch::GetFencer (guint ref)
  {
    GList *current = _fencer_list;

    while (current)
    {
      Player *fencer = (Player *) current->data;

      if (fencer->GetRef () == ref)
      {
        return fencer;
      }

      current = g_list_next (current);
    }

    return NULL;
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
}
