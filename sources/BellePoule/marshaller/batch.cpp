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
#include "util/fie_time.hpp"
#include "util/glade.hpp"
#include "util/dnd_config.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"

#include "job.hpp"
#include "job_board.hpp"
#include "batch.hpp"
#include "competition.hpp"
#include "affinities.hpp"
#include "end_of_burst.hpp"
#include "slot.hpp"

namespace Marshaller
{
  enum class ColumnId
  {
    NAME_str,
    JOB_ptr,
    JOB_color,
    JOB_status,
    JOB_warning_color,
    JOB_position
  };

  // --------------------------------------------------------------------------------
    Batch::Batch (Net::Message *message,
                  Competition  *competition,
                  Listener     *listener)
    : Object ("Batch"),
    Module ("batch.glade")
  {
    _name                  = message->GetString ("name");
    _listener              = listener;
    _scheduled_list        = nullptr;
    _partly_scheduled_list = nullptr;
    _pending_list          = nullptr;
    _competition           = competition;

    _assign_button = _glade->GetWidget ("assign_toolbutton");
    _cancel_button = _glade->GetWidget ("cancel_toolbutton");

    _status = Status::INCOMPLETE;
    _competition->OnNewBatchStatus (this);

    _id        = message->GetNetID ();
    _stage     = message->GetInteger ("stage");
    _batch_ref = message->GetString  ("batch");

    _job_store = GTK_LIST_STORE (_glade->GetGObject ("liststore"));

    _job_board = new JobBoard ();

    _eob = new EndOfBurst (_competition->GetId (),
                           _stage,
                           _batch_ref);

    {
      GtkTreeView *treeview = GTK_TREE_VIEW (_glade->GetWidget ("competition_treeview"));

      gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview),
                                   GTK_SELECTION_MULTIPLE);
    }

    {
      GtkWidget *source = _glade->GetWidget ("competition_treeview");

      _dnd_config->AddTarget ("bellepoule/job", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

      _dnd_config->SetOnAWidgetSrc (source,
                                    GDK_MODIFIER_MASK,
                                    GDK_ACTION_COPY);

      ConnectDndSource (source);
    }

    {
      gchar *extension1 = message->GetString ("extension1");
      gchar *extension2 = message->GetString ("extension2");

      if (extension1)
      {
        gtk_button_set_label (GTK_BUTTON (_glade->GetWidget ("extension1")),
                              extension1);
        gtk_widget_show (_glade->GetWidget ("extension_viewport"));

        if (extension2)
        {
          gtk_button_set_label (GTK_BUTTON (_glade->GetWidget ("extension2")),
                                extension2);
          gtk_widget_show (_glade->GetWidget ("extension2"));
        }
        else
        {
          gtk_widget_hide (_glade->GetWidget ("extension2"));
        }
      }
      else
      {
        gtk_widget_hide (_glade->GetWidget ("extension_viewport"));
      }

      g_free (extension1);
      g_free (extension2);
    }
  }

  // --------------------------------------------------------------------------------
  Batch::~Batch ()
  {
    Mute ();

    g_list_free (_scheduled_list);
    _scheduled_list = nullptr;

    g_list_free (_partly_scheduled_list);
    _partly_scheduled_list = nullptr;

    g_list_free (_pending_list);
    _pending_list = nullptr;

    {
      GtkTreeIter iter;
      gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_job_store),
                                                                 &iter);

      while (iter_is_valid)
      {
        Job *job;

        gtk_tree_model_get (GTK_TREE_MODEL (_job_store), &iter,
                            ColumnId::JOB_ptr, &job,
                            -1);

        job->Release ();

        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                  &iter);
      }

      gtk_list_store_clear (_job_store);
    }

    g_free (_name);

    _job_board->Release ();

    g_free (_batch_ref);

    _eob->Release ();
  }

  // --------------------------------------------------------------------------------
  Batch::Status Batch::GetStatus ()
  {
    return _status;
  }

  // --------------------------------------------------------------------------------
  guint Batch::GetId ()
  {
    return _id;
  }

  // --------------------------------------------------------------------------------
  Competition *Batch::GetCompetition ()
  {
    return _competition;
  }

  // --------------------------------------------------------------------------------
  const gchar *Batch::GetName ()
  {
    return _name;
  }

  // --------------------------------------------------------------------------------
  void Batch::Mute ()
  {
    _eob->Suspend ();
  }

  // --------------------------------------------------------------------------------
  void Batch::UnMute ()
  {
    _eob->Resume ();
  }

  // --------------------------------------------------------------------------------
  gboolean Batch::IsModifiable ()
  {
    return _competition->BatchIsModifiable (this);
  }

  // --------------------------------------------------------------------------------
  void Batch::RaiseOverlapWarning ()
  {
    _listener->OnJobOverlapWarning (this);
  }

  // --------------------------------------------------------------------------------
  void Batch::FixOverlapWarnings ()
  {
    for (GList *current = _scheduled_list; current; current = g_list_next (current))
    {
      Job  *job  = (Job *) current->data;
      Slot *slot = job->GetSlot ();

      slot->FixOverlaps (FALSE);
    }
  }

  // --------------------------------------------------------------------------------
  void Batch::Recall ()
  {
    for (GList *current = _scheduled_list; current; current = g_list_next (current))
    {
      Job *job = (Job *) current->data;

      job->Recall ();
    }

    _eob->Spread ();
  }

  // --------------------------------------------------------------------------------
  void Batch::RefreshControlPanel ()
  {
    gtk_widget_set_sensitive (_assign_button, TRUE);
    gtk_widget_set_sensitive (_cancel_button, TRUE);

    if ((_pending_list == nullptr) && (_partly_scheduled_list == nullptr))
    {
      gtk_widget_set_sensitive (_assign_button, FALSE);

      _status = Status::COMPLETE;
    }
    else
    {
      _status = Status::INCOMPLETE;
    }
    _competition->OnNewBatchStatus (this);

    if (_scheduled_list == nullptr)
    {
      gtk_widget_set_sensitive (_cancel_button, FALSE);
    }

    _eob->Spread ();
  }

  // --------------------------------------------------------------------------------
  GList *Batch::RetreivePendingSelected ()
  {
    GList            *result    = nullptr;
    GtkTreeView      *tree_view = GTK_TREE_VIEW (_glade->GetWidget ("competition_treeview"));
    GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);

    if (selection)
    {
      GtkTreeModel *model          = gtk_tree_view_get_model (tree_view);
      GList        *selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                                           nullptr);
      for (GList *current = selection_list; current; current = g_list_next (current))
      {
        GtkTreeIter  iter;
        Job         *job;

        gtk_tree_model_get_iter (model,
                                 &iter,
                                 (GtkTreePath *) current->data);
        gtk_tree_model_get (model, &iter,
                            ColumnId::JOB_ptr, &job,
                            -1);

        if (g_list_find (_pending_list,
                         job))
        {
          result = g_list_append (result,
                                  job);
        }
        else if (g_list_find (_partly_scheduled_list,
                              job))
        {
          result = g_list_append (result,
                                  job);
        }
      }

      g_list_free_full (selection_list,
                        (GDestroyNotify) gtk_tree_path_free);
    }

    if (result)
    {
      result = g_list_sort (result,
                            (GCompareFunc) Job::ComparePosition);
    }

    return result;
  }

  // --------------------------------------------------------------------------------
  void Batch::OnNewAffinitiesRule (Job *job)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_job_store),
                                                               &iter);

    while (iter_is_valid)
    {
      Job *current_job;

      gtk_tree_model_get (GTK_TREE_MODEL (_job_store), &iter,
                          ColumnId::JOB_ptr, &current_job,
                          -1);

      if (current_job == job)
      {
        guint        kinship = 0;
        const gchar *status  = nullptr;

        {
          GList *node = g_list_find (_pending_list,
                                     job);

          if (job->HasPiste ())
          {
            if (node)
            {
              _pending_list = g_list_remove (_pending_list,
                                             job);
              _scheduled_list = g_list_insert_sorted (_scheduled_list,
                                                      job,
                                                      (GCompareFunc) Job::CompareStartTime);
              job->Spread ();
            }
          }
          else if (node == nullptr)
          {
            _scheduled_list = g_list_remove (_scheduled_list,
                                             job);
            _partly_scheduled_list = g_list_remove (_partly_scheduled_list,
                                                    job);
            _pending_list = g_list_prepend (_pending_list,
                                            job);
            status = GTK_STOCK_DIALOG_QUESTION;
            job->Recall ();
          }
        }

        {
          Slot  *slot = job->GetSlot ();
          GList *node = g_list_find (_partly_scheduled_list,
                                     job);

          if (slot && slot->GetRefereeList ())
          {
            kinship = job->GetKinship ();

            if (node)
            {
              _partly_scheduled_list = g_list_remove (_partly_scheduled_list,
                                                      job);
              if (status)
              {
                status = GTK_STOCK_DIALOG_ERROR;
              }
              job->Spread ();
            }
          }
          else if ((node == nullptr) && job->HasPiste ())
          {
            _partly_scheduled_list = g_list_prepend (_partly_scheduled_list,
                                                     job);
            job->Spread ();
          }
        }

        gtk_list_store_set (_job_store, &iter,
                            ColumnId::JOB_status,        status,
                            ColumnId::JOB_warning_color, Affinities::GetColor (kinship),
                            -1);
        break;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                &iter);
    }
  }

  // --------------------------------------------------------------------------------
  void Batch::OnNewJobStatus (Job *job)
  {
    OnNewAffinitiesRule (job);
    RefreshControlPanel ();
  }

  // --------------------------------------------------------------------------------
  GList *Batch::GetScheduledJobs ()
  {
    return _scheduled_list;
  }

  // --------------------------------------------------------------------------------
  GList *Batch::RetreivePendingJobs ()
  {
    GList *list = g_list_copy (_pending_list);

    list = g_list_concat (list,
                          g_list_copy (_partly_scheduled_list));

    list = g_list_sort (list,
                        (GCompareFunc) Job::ComparePosition);

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

      gtk_tree_model_get (GTK_TREE_MODEL (_job_store), &iter,
                          ColumnId::JOB_ptr, &current_job,
                          -1);

      if (message->GetNetID () == current_job->GetNetID ())
      {
        gtk_list_store_remove (_job_store,
                               &iter);

        _pending_list = g_list_remove (_pending_list,
                                       current_job);

        _partly_scheduled_list = g_list_remove (_partly_scheduled_list,
                                                current_job);

        _scheduled_list = g_list_remove (_scheduled_list,
                                         current_job);

        RefreshControlPanel ();
        current_job->Release ();

        break;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                &iter);
    }
  }

  // --------------------------------------------------------------------------------
  Job *Batch::GetJob (guint netid)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_job_store),
                                                               &iter);

    while (iter_is_valid)
    {
      Job *current_job;

      gtk_tree_model_get (GTK_TREE_MODEL (_job_store), &iter,
                          ColumnId::JOB_ptr, &current_job,
                          -1);

      if (current_job->GetNetID () == netid)
      {
        return current_job;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_job_store),
                                                &iter);
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  Job *Batch::Load (Net::Message  *message,
                    guint         *piste_id,
                    GList        **referees,
                    FieTime      **start_time)
  {
    Job *job = nullptr;

    *piste_id   = 0;
    *referees   = nullptr;
    *start_time = nullptr;

    {
      gchar     *xml = message->GetString ("xml");
      xmlDocPtr  doc = xmlParseMemory (xml, strlen (xml));

      if (doc)
      {
        xmlXPathInit ();

        {
          const char *heading;

          xmlXPathContext *xml_context = xmlXPathNewContext (doc);
          xmlXPathObject  *xml_object;
          xmlNodeSet      *xml_nodeset;

          xml_object = xmlXPathEval (BAD_CAST "/Poule", xml_context);
          xml_nodeset = xml_object->nodesetval;
          heading = "Pool";

          if (xml_nodeset->nodeNr == 0)
          {
            xml_object = xmlXPathEval (BAD_CAST "/Match", xml_context);
            xml_nodeset = xml_object->nodesetval;
            heading = "Match";
          }

          if (xml_nodeset->nodeNr == 1)
          {
            xmlNode     *root_node     = xml_nodeset->nodeTab[0];
            GtkTreeIter  iter;
            gchar       *attr;
            gchar       *name;

            job = new Job (this,
                           message,
                           _competition->GetColor ());

            attr = (gchar *) xmlGetProp (root_node, BAD_CAST "ID");
            if (attr)
            {
              name = g_strdup_printf ("%s %s", gettext (heading), attr);
              xmlFree (attr);
            }
            else
            {
              name = g_strdup (gettext ("Pool"));
            }

            attr = (gchar *) xmlGetProp (root_node, BAD_CAST "Piste");
            if (attr)
            {
              *piste_id = atoi (attr);
              xmlFree (attr);
            }

            {
              gchar *date = (gchar *) xmlGetProp (root_node, BAD_CAST "Date");
              gchar *time = (gchar *) xmlGetProp (root_node, BAD_CAST "Heure");

              if (date && time)
              {
                *start_time = new FieTime (date,
                                           time);
              }

              xmlFree (date);
              xmlFree (time);
            }

            gtk_list_store_append (_job_store,
                                   &iter);

            job->SetName (name);
            gtk_list_store_set (_job_store, &iter,
                                ColumnId::NAME_str,     name,
                                ColumnId::JOB_ptr,      job,
                                ColumnId::JOB_position, job->GetDisplayPosition (),
                                ColumnId::JOB_status,   GTK_STOCK_DIALOG_QUESTION,
                                -1);
            g_free (name);

            {
              GtkTreeSortable *sortable = GTK_TREE_SORTABLE (_glade->GetWidget ("treemodelsort"));

              gtk_tree_sortable_set_sort_column_id (sortable,
                                                    GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                                    GTK_SORT_ASCENDING);
              gtk_tree_sortable_set_sort_column_id (sortable,
                                                    (int) ColumnId::JOB_position,
                                                    GTK_SORT_ASCENDING);
            }

            _pending_list = g_list_append (_pending_list,
                                           job);
            _job_board->AddJob (job);

            for (xmlNode *n = xml_nodeset->nodeTab[0]->children; n != nullptr; n = n->next)
            {
              if (n->type == XML_ELEMENT_NODE)
              {
                if (   (g_strcmp0 ((gchar *)n->name, "Tireur") == 0)
                    || (g_strcmp0 ((gchar *)n->name, "Equipe") == 0))
                {
                  attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");
                  if (attr)
                  {
                    Player *fencer = _competition->GetFencer (atoi (attr));

                    if (fencer == nullptr)
                    {
                      g_error ("Fencer %s not found!\n", attr);
                    }
                    else
                    {
                      job->AddFencer (fencer);
                    }
                    xmlFree (attr);
                  }
                }
                else if (g_strcmp0 ((gchar *)n->name, "Arbitre") == 0)
                {
                  attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");
                  if (attr)
                  {
                    *referees = g_list_append (*referees,
                                               GUINT_TO_POINTER (atoi (attr)));
                    xmlFree (attr);
                  }
                }
              }
            }
          }

          xmlXPathFreeObject  (xml_object);
          xmlXPathFreeContext (xml_context);
        }
        xmlFreeDoc (doc);
      }
      g_free (xml);

      RefreshControlPanel ();
    }

    return job;
  }

  // --------------------------------------------------------------------------------
  void Batch::OnDragDataGet (GtkWidget        *widget,
                             GdkDragContext   *drag_context,
                             GtkSelectionData *data,
                             guint             key,
                             guint             time)
  {
    GList *selected = RetreivePendingSelected ();

    if (selected)
    {
      Job     *job      = (Job *) selected->data;
      guint32  job_ref  = job->GetNetID ();

      gtk_selection_data_set (data,
                              gtk_selection_data_get_target (data),
                              32,
                              (guchar *) &job_ref,
                              sizeof (job_ref));

      g_list_free (selected);
    }
  }

  // --------------------------------------------------------------------------------
  void Batch::OnExtension ()
  {
    OnCancelAssign ();

    {
      guint  factor  = 1;
      GList *current = _pending_list;

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("extension1"))))
      {
        factor = 2;
      }
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("extension2"))))
      {
        factor = 3;
      }

      while (current)
      {
        Job *job = (Job *) current->data;

        job->ExtendDuration (factor);

        current = g_list_next (current);
      }
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
  void Batch::Spread ()
  {
    for (GList *current = _scheduled_list; current; current = g_list_next (current))
    {
      Job *job = (Job *) current->data;

      job->Spread ();
    }

    _eob->Spread ();
  }

  // --------------------------------------------------------------------------------
  void Batch::Dump ()
  {
    g_print ("_pending_list          ==> %d\n", g_list_length (_pending_list));
    g_print ("_partly_scheduled_list ==> %d\n", g_list_length (_partly_scheduled_list));
    g_print ("_scheduled_list        ==> %d\n", g_list_length (_scheduled_list));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_competition_treeview_row_activated  (GtkTreeView       *tree_view,
                                                                          GtkTreePath       *path,
                                                                          GtkTreeViewColumn *column,
                                                                          Object            *owner)
  {
    Batch *b = dynamic_cast <Batch *> (owner);

    b->on_competition_treeview_row_activated (path);
  }

  // --------------------------------------------------------------------------------
  void Batch::on_competition_treeview_row_activated (GtkTreePath *path)
  {
    GtkTreeModel *model = GTK_TREE_MODEL (_job_store);
    Job          *job;
    GtkTreeIter   iter;

    gtk_tree_model_get_iter (model,
                             &iter,
                             path);
    gtk_tree_model_get (model, &iter,
                        ColumnId::JOB_ptr, &job,
                        -1);

    _job_board->Display (job);
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

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_extension_toggled (GtkToggleButton *togglebutton,
                                                        Object          *owner)
  {
    if (gtk_toggle_button_get_active (togglebutton))
    {
      Batch *b = dynamic_cast <Batch *> (owner);

      b->OnExtension ();
    }
  }
}
