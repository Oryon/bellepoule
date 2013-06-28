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

#include "common/classification.hpp"
#include "common/contest.hpp"
#include "network/uploader.hpp"
#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"

namespace Pool
{
  const gchar *Supervisor::_class_name     = N_ ("Pools");
  const gchar *Supervisor::_xml_class_name = "TourDePoules";

  typedef enum
  {
    NAME_COLUMN,
    STATUS_COLUMN
  } ColumnId;

  extern "C" G_MODULE_EXPORT void on_pool_combobox_changed (GtkWidget *widget,
                                                            Object    *owner);

  // --------------------------------------------------------------------------------
  Supervisor::Supervisor (StageClass *stage_class)
    : Stage (stage_class),
    Module ("pool_supervisor.glade")
  {
    _allocator      = NULL;
    _displayed_pool = NULL;
    _max_score      = NULL;

    _single_owner = new Object ();

    _pool_liststore = GTK_LIST_STORE (_glade->GetGObject ("pool_liststore"));

    // Sensitive widgets
    {
      AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
      AddSensitiveWidget (_glade->GetWidget ("qualified_table"));
      AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

      LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
      LockOnClassification (_glade->GetWidget ("pool_combobox"));
    }

    // Filter
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "HS",
                                          "attending",
                                          "availability",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "participation_rate",
                                          "pool_nr",
                                          "rank",
                                          "start_rank",
                                          "status",
                                          "team",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");

      SetFilter (filter);
      filter->Release ();
    }

    {
      GtkWidget *content_area;

      _print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          gettext ("<b><big>Print...</big></b>"));

      gtk_window_set_title (GTK_WINDOW (_print_dialog),
                            gettext ("Pool sheets printing"));

      content_area = gtk_dialog_get_content_area (GTK_DIALOG (_print_dialog));

      gtk_widget_reparent (_glade->GetWidget ("print_dialog-vbox"),
                           content_area);
    }

    // Classification filter
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "attending",
                                          "availability",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "level",
                                          "participation_rate",
                                          "start_rank",
                                          "team",
                                          NULL);
      filter = new Filter (attr_list);

      filter->ShowAttribute ("rank");
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("pool_nr");
      filter->ShowAttribute ("victories_ratio");
      filter->ShowAttribute ("indice");
      filter->ShowAttribute ("HS");
      filter->ShowAttribute ("status");

      SetClassificationFilter (filter);
      filter->Release ();
    }

    {
      Classification *classification = GetClassification ();

      classification->SetSortFunction ((GtkTreeIterCompareFunc) CompareClassification,
                                       this);
    }
  }

  // --------------------------------------------------------------------------------
  Supervisor::~Supervisor ()
  {
    Object::TryToRelease (_allocator);
    gtk_widget_destroy (_print_dialog);

    _single_owner->Release ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);
  }

  // --------------------------------------------------------------------------------
  Stage *Supervisor::CreateInstance (StageClass *stage_class)
  {
    return new Supervisor (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPlugged ()
  {
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton")),
                                       FALSE);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("single_radiobutton")),
                                  TRUE);

    gtk_widget_set_sensitive (_glade->GetWidget ("seeding_viewport"),
                              FALSE);

    RetrievePools ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnUnPlugged ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("seeding_viewport"),
                              TRUE);

    for (guint i = 0; i < _allocator->GetNbPools (); i++)
    {
      Pool *pool = _allocator->GetPool (i);

      pool->DeleteMatchs ();
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Display ()
  {
    Classification *classification = GetClassification ();

    OnPoolSelected (0);
    ToggleClassification (FALSE);

    classification->SetDataOwner (_single_owner);
    classification->SortDisplay ();
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::OnUploaderStatus (Net::Uploader::Status *status)
  {
    Supervisor *supervisor = dynamic_cast <Supervisor *> (status->_object);

    for (guint p = 0; p < supervisor->_allocator->GetNbPools (); p++)
    {
      Pool   *pool            = supervisor->_allocator->GetPool (p);
      GSList *current_referee = pool->GetRefereeList ();

      while (current_referee)
      {
        Player              *referee = (Player *) current_referee->data;
        Player::AttributeId  attr_id ("IP");
        Attribute           *ip_attr = referee->GetAttribute (&attr_id);

        if (ip_attr)
        {
          gchar *ip = ip_attr->GetStrValue ();

          if (ip && (strcmp (ip, status->_peer) == 0))
          {
            Player::AttributeId  connection_attr_id ("connection");

            if (status->_peer_status == Net::Uploader::CONN_ERROR)
            {
              referee->SetAttributeValue (&connection_attr_id,
                                          "Broken");
            }
            else
            {
              referee->SetAttributeValue (&connection_attr_id,
                                          "Waiting");
            }
            goto done;
          }
        }

        current_referee = g_slist_next (current_referee);
      }
    }

done:
    status->Release ();
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnSmartPouleClicked ()
  {
    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool   *pool            = _allocator->GetPool (p);
      GSList *current_referee = pool->GetRefereeList ();

      while (current_referee)
      {
        Player              *referee = (Player *) current_referee->data;
        Player::AttributeId  attr_id ("IP");
        Attribute           *ip_attr = referee->GetAttribute (&attr_id);

        if (ip_attr)
        {
          gchar *ip = ip_attr->GetStrValue ();

          if (ip && (ip[0] != 0))
          {
            xmlBuffer *xml_buffer = xmlBufferCreate ();

            {
              xmlTextWriter *xml_writer = xmlNewTextWriterMemory (xml_buffer, 0);

              _contest->SaveHeader (xml_writer);
              _allocator->SaveHeader (xml_writer);
              pool->Save (xml_writer);
              xmlTextWriterEndElement (xml_writer);
              xmlTextWriterEndElement (xml_writer);
              xmlTextWriterEndDocument (xml_writer);

              xmlFreeTextWriter (xml_writer);
            }

            {
              gchar         *url      = g_strdup_printf ("http://%s:35830/bouts/batch1", ip);
              Net::Uploader *uploader = new Net::Uploader (url,
                                                           (Net::Uploader::UploadStatus) OnUploaderStatus, this,
                                                           NULL, NULL);

              uploader->UploadString ((const gchar *) xml_buffer->content);
              g_free (url);
            }

            xmlBufferFree (xml_buffer);
          }
        }

        current_referee = g_slist_next (current_referee);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Garnish ()
  {
    _displayed_pool = _allocator->GetPool (0);
  }

  // --------------------------------------------------------------------------------
  gint Supervisor::CompareSingleClassification (Player         *A,
                                                Player         *B,
                                                Supervisor *pool_supervisor)
  {
    guint policy = Pool::WITH_CALCULUS | Pool::WITH_RANDOM;

    if (pool_supervisor->_allocator->SeedingIsBalanced () == FALSE)
    {
      policy |= Pool::WITH_POOL_NR;
    }

    return Pool::ComparePlayer (A,
                                B,
                                pool_supervisor->_single_owner,
                                pool_supervisor->_rand_seed,
                                pool_supervisor->GetDataOwner (),
                                policy);
  }

  // --------------------------------------------------------------------------------
  gint Supervisor::CompareCombinedClassification (Player         *A,
                                                  Player         *B,
                                                  Supervisor *pool_supervisor)
  {
    guint policy = Pool::WITH_CALCULUS | Pool::WITH_RANDOM;

    if (pool_supervisor->_allocator->SeedingIsBalanced () == FALSE)
    {
      policy |= Pool::WITH_POOL_NR;
    }

    return Pool::ComparePlayer (A,
                                B,
                                pool_supervisor,
                                pool_supervisor->_rand_seed,
                                pool_supervisor->GetDataOwner (),
                                policy);
  }

  // --------------------------------------------------------------------------------
  gint Supervisor::CompareClassification (GtkTreeModel *model,
                                          GtkTreeIter  *a,
                                          GtkTreeIter  *b,
                                          Supervisor   *pool_supervisor)
  {
    GtkWidget      *w              = pool_supervisor->_glade->GetWidget ("single_radiobutton");
    Classification *classification = pool_supervisor->GetClassification ();
    Player         *A              = classification->GetPlayer (model, a);
    Player         *B              = classification->GetPlayer (model, b);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    {
      return CompareSingleClassification (A,
                                          B,
                                          pool_supervisor);
    }
    else
    {
      return CompareCombinedClassification (A,
                                            B,
                                            pool_supervisor);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnLocked ()
  {
    DisableSensitiveWidgets ();

    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      pool->Lock ();
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnUnLocked ()
  {
    EnableSensitiveWidgets ();

    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      pool->UnLock ();
    }

    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Wipe ()
  {
    if (_allocator)
    {
      for (guint p = 0; p < _allocator->GetNbPools (); p++)
      {
        Pool *pool = _allocator->GetPool (p);

        pool->CleanScores ();
        pool->Wipe ();
      }

      if (_displayed_pool)
      {
        _displayed_pool->UnPlug ();
        _displayed_pool = NULL;
      }

      gtk_list_store_clear (_pool_liststore);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Manage (Pool *pool)
  {
    GtkTreeIter iter;

    gtk_list_store_append (_pool_liststore,
                           &iter);

    gtk_list_store_set (_pool_liststore, &iter,
                        NAME_COLUMN, pool->GetName (),
                        -1);

    {
      Stage      *previous_stage = GetPreviousStage ();
      Supervisor *previous_pool = dynamic_cast <Supervisor *> (previous_stage->GetPreviousStage ());

      pool->SetDataOwner (_single_owner,
                          this,
                          previous_pool);
    }
    pool->CopyPlayersStatus (_allocator);

    pool->SetFilter (_filter);
    pool->SetStatusCbk ((Pool::StatusCbk) OnPoolStatusUpdated,
                        this);

    gtk_list_store_set (_pool_liststore, &iter,
                        STATUS_COLUMN, GTK_STOCK_EXECUTE,
                        -1);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPoolStatusUpdated (Pool       *pool,
                                        Supervisor *ps)
  {
    GtkTreeIter iter;

    if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (ps->_pool_liststore),
                                       &iter,
                                       NULL,
                                       pool->GetNumber () - 1))
    {
      PoolZone *zone = ps->_allocator->GetZone (pool->GetNumber () - 1);

      if (pool->IsOver ())
      {
        zone->FreeReferees ();
        gtk_list_store_set (ps->_pool_liststore, &iter,
                            STATUS_COLUMN, GTK_STOCK_APPLY,
                            -1);
      }
      else if (pool->HasError ())
      {
        zone->BookReferees ();
        gtk_list_store_set (ps->_pool_liststore, &iter,
                            STATUS_COLUMN, GTK_STOCK_DIALOG_WARNING,
                            -1);
      }
      else
      {
        zone->BookReferees ();
        gtk_list_store_set (ps->_pool_liststore, &iter,
                            STATUS_COLUMN, GTK_STOCK_EXECUTE,
                            -1);
      }
    }

    ps->SignalStatusUpdate ();
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::IsOver ()
  {
    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      if (pool->IsOver () == FALSE)
      {
        return FALSE;
      }
    }
    return TRUE;
  }

  // --------------------------------------------------------------------------------
  gchar *Supervisor::GetError ()
  {
    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      if (pool->HasError ())
      {
        return g_strdup_printf (" <span foreground=\"black\" weight=\"800\">%s:</span> \n "
                                " <span foreground=\"black\" style=\"italic\" weight=\"400\">\"%s\" </span>",
                                pool->GetName (), gettext ("Bout without winner!"));
      }
    }
    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPoolSelected (gint index)
  {
    if ((index >= 0) && _allocator)
    {
      OnPoolSelected (_allocator->GetPool (index));

      g_signal_handlers_disconnect_by_func (_glade->GetWidget ("pool_combobox"),
                                            (void *) on_pool_combobox_changed,
                                            (Object *) this);
      gtk_combo_box_set_active (GTK_COMBO_BOX (_glade->GetWidget ("pool_combobox")),
                                index);
      g_signal_connect (_glade->GetWidget ("pool_combobox"), "changed",
                        G_CALLBACK (on_pool_combobox_changed),
                        (Object *) this);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPoolSelected (Pool *pool)
  {
    if (_displayed_pool)
    {
      _displayed_pool->UnPlug ();
      _displayed_pool = NULL;
    }

    if (pool)
    {
      Plug (pool,
            _glade->GetWidget ("main_hook"));

      _displayed_pool = pool;
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_print_pool_toolbutton_clicked (GtkWidget *widget,
                                                                    Object    *owner)
  {
    Supervisor *p = dynamic_cast <Supervisor *> (owner);

    p->OnPrintPoolToolbuttonClicked ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPrintPoolToolbuttonClicked ()
  {
    gchar *title = NULL;

    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))))
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        title = g_strdup_printf ("%s - %s", gettext ("Pools classification"), GetName ());
        classification->Print (title);
      }
    }
    else if (gtk_dialog_run (GTK_DIALOG (_print_dialog)) == GTK_RESPONSE_OK)
    {
      GtkWidget *w = _glade->GetWidget ("all_pool_radiobutton");

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
      {
        _print_all_pool = TRUE;
      }
      else
      {
        _print_all_pool = FALSE;
      }

      title = g_strdup_printf ("%s - %s", gettext ("Pools"), GetName ());
      Print (title);
    }
    g_free (title);
    gtk_widget_hide (_print_dialog);
  }

  // --------------------------------------------------------------------------------
  gchar *Supervisor::GetPrintName ()
  {
    return g_strdup_printf ("%s - %s", gettext ("Pools"), GetName ());
  }

  // --------------------------------------------------------------------------------
  guint Supervisor::PreparePrint (GtkPrintOperation *operation,
                                  GtkPrintContext   *context)
  {
    if (GetStageView (operation) == STAGE_VIEW_CLASSIFICATION)
    {
      return 0;
    }

    if (GetStageView (operation) == STAGE_VIEW_RESULT)
    {
      _print_all_pool = TRUE;
    }

    {
      GtkWidget *w = _glade->GetWidget ("for_referees_radiobutton");

      if (   (GetStageView (operation) == STAGE_VIEW_UNDEFINED)
          && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
      {
        g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) TRUE);
      }
      else
      {
        g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) FALSE);
      }
    }

    if (_displayed_pool)
    {
      _displayed_pool->Wipe ();
    }

    if (_print_all_pool)
    {
      return _allocator->GetNbPools ();
    }
    else
    {
      return 1;
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::DrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr)
  {
    DrawContainerPage (operation,
                       context,
                       page_nr);

    if (   (GetStageView (operation) == STAGE_VIEW_RESULT)
        || (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))) == FALSE))
    {
      Pool *pool;

      if (_print_all_pool)
      {
        pool = _allocator->GetPool (page_nr);
      }
      else
      {
        pool = _displayed_pool;
      }

      pool->DrawPage (operation,
                      context,
                      page_nr);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnEndPrint (GtkPrintOperation *operation,
                               GtkPrintContext   *context)
  {
    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::RetrievePools ()
  {
    if (_allocator)
    {
      for (guint p = 0; p < _allocator->GetNbPools (); p++)
      {
        Manage (_allocator->GetPool (p));
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::ApplyConfig ()
  {
    Stage::ApplyConfig ();

    if (_displayed_pool)
    {
      OnPoolSelected (_displayed_pool);
    }
  }

  // --------------------------------------------------------------------------------
  Stage *Supervisor::GetInputProvider ()
  {
    Stage *previous = GetPreviousStage ();

    if (previous)
    {
      Stage::StageClass *provider_class = previous->GetClass ();

      if (strcmp (provider_class->_xml_name, Allocator::_xml_class_name) == 0)
      {
        return previous;
      }
    }

    return Stage::CreateInstance (Allocator::_xml_class_name);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::SetInputProvider (Stage *input_provider)
  {
    _allocator = dynamic_cast <Allocator *> (input_provider);

    if (_allocator)
    {
      _allocator->Retain ();
      _max_score = _allocator->GetMaxScore ();
    }

    Stage::SetInputProvider (input_provider);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::FillInConfig ()
  {
    Stage::FillInConfig ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnAttrListUpdated ()
  {
    if (_displayed_pool)
    {
      OnPoolSelected (_displayed_pool);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnFilterClicked ()
  {
    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))))
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        classification->SelectAttributes ();
      }
    }
    else
    {
      SelectAttributes ();
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnStuffClicked ()
  {
    for (guint i = 0; i < _allocator->GetNbPools (); i++)
    {
      Pool *pool = _allocator->GetPool (i);

      pool->CleanScores ();
      pool->Stuff ();
    }

    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Load (xmlNode *xml_node)
  {
    LoadAttendees (NULL);

    LoadConfiguration (xml_node);

    {
      guint nb_pools = _allocator->GetNbPools ();

      for (guint p = 0; p < nb_pools; p++)
      {
        Pool *pool = _allocator->GetPool (p);

        OnPoolStatusUpdated (pool,
                             this);
      }
    }
  }

  // --------------------------------------------------------------------------------
  GSList *Supervisor::GetCurrentClassification ()
  {
    GSList *result = NULL;

    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool   *pool           = _allocator->GetPool (p);
      GSList *current_player = pool->GetFencerList ();

      while (current_player)
      {
        Player *player = (Player *) current_player->data;

        result = g_slist_prepend (result,
                                  player);
        current_player = g_slist_next (current_player);
      }
    }

    result = EvaluateClassification (result,
                                     _single_owner,
                                     (GCompareDataFunc) CompareSingleClassification);

    return EvaluateClassification (result,
                                   this,
                                   (GCompareDataFunc) CompareCombinedClassification);
  }

  // --------------------------------------------------------------------------------
  GSList *Supervisor::EvaluateClassification (GSList           *list,
                                              Object           *rank_owner,
                                              GCompareDataFunc  CompareFunction)
  {
    Player::AttributeId *attr_id = new Player::AttributeId ("rank", rank_owner);
    GSList              *result;
    GSList              *current;
    guint                previous_rank   = 0;
    Player              *previous_player = NULL;

    if (CompareFunction)
    {
      result = g_slist_sort_with_data (list,
                                       CompareFunction,
                                       this);
    }
    else
    {
      result = list;
    }

    current = result;
    for (guint i = 1; current; i++)
    {
      Player *player = (Player *) current->data;

      if (   previous_player
          && (Pool::ComparePlayer (player,
                                   previous_player,
                                   rank_owner,
                                   0,
                                   GetDataOwner (),
                                   Pool::WITH_CALCULUS) == 0))
      {
        player->SetAttributeValue (attr_id,
                                   previous_rank);
      }
      else
      {
        player->SetAttributeValue (attr_id,
                                   i);
        previous_rank = i;
      }

      previous_player = player;
      current = g_slist_next (current);
    }
    attr_id->Release ();

    return result;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnToggleSingleClassification (gboolean single_selected)
  {
    Classification *classification = GetClassification ();

    if (single_selected)
    {
      classification->SetDataOwner (_single_owner);
    }
    else
    {
      classification->SetDataOwner (this);
    }

    ToggleClassification (TRUE);
    classification->SortDisplay ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_pool_filter_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Supervisor *p = dynamic_cast <Supervisor *> (owner);

    p->OnFilterClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_pool_combobox_changed (GtkWidget *widget,
                                                            Object    *owner)
  {
    Supervisor *p          = dynamic_cast <Supervisor *> (owner);
    gint            pool_index = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

    p->OnPoolSelected (pool_index);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_pool_classification_toggletoolbutton_toggled (GtkToggleToolButton *widget,
                                                                                   Object              *owner)
  {
    Supervisor *p = dynamic_cast <Supervisor *> (owner);

    p->ToggleClassification (gtk_toggle_tool_button_get_active (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_stuff_toolbutton_clicked (GtkWidget *widget,
                                                               Object    *owner)
  {
    Supervisor *ps = dynamic_cast <Supervisor *> (owner);

    ps->OnStuffClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_smartpoule_toolbutton_clicked (GtkWidget *widget,
                                                                    Object    *owner)
  {
    Supervisor *ps = dynamic_cast <Supervisor *> (owner);

    ps->OnSmartPouleClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_single_radiobutton_toggled (GtkWidget *widget,
                                                                 Object    *owner)
  {
    Supervisor *p = dynamic_cast <Supervisor *> (owner);

    p->OnToggleSingleClassification (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
  }
}
