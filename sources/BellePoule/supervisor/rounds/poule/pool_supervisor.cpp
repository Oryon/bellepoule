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
#include <gdk/gdkkeysyms.h>

#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/glade.hpp"
#include "util/player.hpp"
#include "network/message.hpp"
#include "../../classification.hpp"

#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"

#define POOL_PAGE_DENSITY (_pool_v_density*_pool_h_density)

namespace Pool
{
  const gchar *Supervisor::_class_name     = N_ ("Pools");
  const gchar *Supervisor::_xml_class_name = "TourDePoules";

  enum class ColumnId
  {
    NAME,
    STATUS
  };

  extern "C" G_MODULE_EXPORT void on_pool_combobox_changed (GtkWidget *widget,
                                                            Object    *owner);

  // --------------------------------------------------------------------------------
  Supervisor::Supervisor (StageClass *stage_class)
    : Object ("Pool::Supervisor"),
      Stage (stage_class),
      Module ("pool_supervisor.glade")
  {
    _allocator      = nullptr;
    _displayed_pool = nullptr;
    _max_score      = nullptr;

    _current_round_owner = new Object ("Pool::Supervisor.owner");

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
                                          "plugin_ID",
#endif
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "score_quest",
                                          "elo",
                                          "HS",
                                          "attending",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          "strip",
                                          "time",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");

      SetFilter (filter);
      filter->Release ();
    }

    // Classifications
    {
      Filter *filter;

      {
        GSList *attr_list;

        AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                            "ref",
                                            "plugin_ID",
#endif
                                            "IP",
                                            "password",
                                            "cyphered_password",
                                            "score_quest",
                                            "elo",
                                            "attending",
                                            "exported",
                                            "final_rank",
                                            "global_status",
                                            "level",
                                            "workload_rate",
                                            "promoted",
                                            "team",
                                            "strip",
                                            "time",
                                            NULL);
        filter = new Filter (GetKlassName (),
                             attr_list);

        filter->ShowAttribute ("rank");
        filter->ShowAttribute ("status");
        filter->ShowAttribute ("name");
        filter->ShowAttribute ("first_name");
        filter->ShowAttribute ("club");
        filter->ShowAttribute ("pool_nr");
#ifdef DEBUG
        filter->ShowAttribute ("victories_count");
        filter->ShowAttribute ("bouts_count");
#endif
        filter->ShowAttribute ("victories_ratio");
        filter->ShowAttribute ("indice");
        filter->ShowAttribute ("HS");

        SetClassificationFilter (filter);
        filter->Release ();
      }

      {
        _current_round_classification = new Classification ();
        _current_round_classification->SetFilter (filter);
        Plug (_current_round_classification,
              _glade->GetWidget ("current_round_classification_hook"));
      }
    }
  }

  // --------------------------------------------------------------------------------
  Supervisor::~Supervisor ()
  {
    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      pool->RegisterStatusListener (nullptr);
    }

    Object::TryToRelease (_allocator);

    _current_round_classification->Release ();

    _current_round_owner->Release ();
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

    gtk_widget_set_sensitive (_glade->GetWidget ("seeding_viewport"),
                              FALSE);

    RetrievePools ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnUnPlugged ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("seeding_viewport"),
                              TRUE);

    if (_allocator)
    {
      for (guint p = 0; p < _allocator->GetNbPools (); p++)
      {
        Pool *pool = _allocator->GetPool (p);

        pool->CleanScores ();
        pool->Wipe ();
        pool->DeleteMatchs ();
      }

      if (_displayed_pool)
      {
        _displayed_pool->UnPlug ();
        _displayed_pool = nullptr;
      }

      gtk_list_store_clear (_pool_liststore);
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Display ()
  {
    OnPoolSelected (0);
    ToggleClassification (FALSE);

    _current_round_classification->SetDataOwner (_current_round_owner);
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::OnMessage (Net::Message *message)
  {
    if (message->Is ("SmartPoule::Score"))
    {
      Pool *pool = _allocator->GetPool (message->GetInteger ("batch")-1);

      if (pool)
      {
        return pool->OnMessage (message);
      }
    }
    else if (message->Is ("BellePoule2D::EndOfBurst"))
    {
      OnPoolSelected (_displayed_pool);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Supervisor::OnHttpPost (const gchar *command,
                                   const gchar **ressource,
                                   const gchar *data)
  {
    if (*ressource)
    {
      guint pool_index = atoi (*ressource);

      if (pool_index > 0)
      {
        pool_index--;

        if (g_strcmp0 (command, "ScoreSheet") == 0)
        {
          OnPoolSelected (pool_index);
        }
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Garnish ()
  {
    _displayed_pool = _allocator->GetPool (0);
  }

  // --------------------------------------------------------------------------------
  gint Supervisor::CompareCurrentRoundClassification (Player     *A,
                                                      Player     *B,
                                                      Supervisor *pool_supervisor)
  {
    guint policy = Pool::WITH_CALCULUS | Pool::WITH_RANDOM;

    if (pool_supervisor->_allocator->SeedingIsBalanced () == FALSE)
    {
      policy |= Pool::WITH_POOL_NR;
    }

    return Pool::ComparePlayer (A,
                                B,
                                pool_supervisor->_current_round_owner,
                                pool_supervisor->_rand_seed,
                                policy);
  }

  // --------------------------------------------------------------------------------
  gint Supervisor::CompareCombinedRoundsClassification (Player     *A,
                                                        Player     *B,
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
                                policy);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnLocked ()
  {
    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      pool->Lock ();
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnUnLocked ()
  {
    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      pool->UnLock ();
    }

    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Manage (Pool *pool)
  {
    GtkTreeIter iter;

    gtk_list_store_append (_pool_liststore,
                           &iter);

    gtk_list_store_set (_pool_liststore, &iter,
                        ColumnId::NAME, pool->GetName (),
                        -1);

    {
      Stage      *previous_stage = GetPreviousStage ();
      Supervisor *previous_pool = dynamic_cast <Supervisor *> (previous_stage->GetPreviousStage ());

      pool->SetDataOwner (_current_round_owner,
                          this,
                          previous_pool);
    }
    pool->CopyPlayersStatus (_allocator);

    pool->SetFilter (_filter);
    pool->RegisterStatusListener (this);
  }

  // --------------------------------------------------------------------------------
  void Supervisor::Save (XmlScheme *xml_scheme)
  {
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPoolStatus (Pool *pool)
  {
    GtkTreeIter iter;

    if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_pool_liststore),
                                       &iter,
                                       nullptr,
                                       pool->GetNumber () - 1))
    {
      GdkPixbuf *pixbuf = pool->GetStatusPixbuf ();

      gtk_list_store_set (_pool_liststore, &iter,
                          ColumnId::STATUS, pixbuf,
                          -1);
      if (pixbuf)
      {
        g_object_unref (pixbuf);
      }
    }

    SignalStatusUpdate ();
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
  Error *Supervisor::GetError ()
  {
    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      if (pool->HasError ())
      {
        return new Error (Error::Level::MAJOR,
                          pool->GetName (),
                          gettext ("Bout without winner!"));
      }
    }
    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Supervisor::OnPoolSelected (gint index)
  {
    if ((index >= 0) && _allocator)
    {
      OnPoolSelected (_allocator->GetPool (index));

      __gcc_extension__ g_signal_handlers_disconnect_by_func (_glade->GetWidget ("pool_combobox"),
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
      _displayed_pool = nullptr;
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
    gchar *title = g_strdup_printf ("%s - %s", gettext ("Pools"), GetName ());

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("watermark_checkbutton")),
                                  Pool::WaterMarkingEnabled ());

    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))))
    {
      Classification *classification;

      if (gtk_notebook_get_current_page (GTK_NOTEBOOK (_glade->GetWidget ("classification_hook"))))
      {
        classification = _current_round_classification;
      }
      else
      {
        classification = GetClassification ();
      }

      if (classification)
      {
        classification->Print (title);
      }
    }
    else
    {
      GtkWidget *print_dialog = _glade->GetWidget ("print_pool_dialog");

      if (RunDialog (GTK_DIALOG (print_dialog)) == GTK_RESPONSE_OK)
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

        Pool::SetWaterMarkingPolicy (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("watermark_checkbutton"))));

        Print (title);
      }
      gtk_widget_hide (print_dialog);
    }

    g_free (title);
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
    if (GetStageView (operation) == StageView::CLASSIFICATION)
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        return classification->PreparePrint (operation,
                                             context);
      }
      return 0;
    }

    if (_displayed_pool)
    {
      _displayed_pool->Wipe ();
    }

    if (GetStageView (operation) == StageView::RESULT)
    {
      _print_all_pool = TRUE;
    }

    _pool_v_density = 1;
    _pool_h_density = 1;

    {
      GtkWidget *w = _glade->GetWidget ("for_referees_radiobutton");

      if (   (GetStageView (operation) == StageView::UNDEFINED)
          && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
      {
        g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) TRUE);
      }
      else
      {
        g_object_set_data (G_OBJECT (operation), "print_for_referees", (void *) FALSE);
        SetPrintPoolDensity ();
      }
    }

    if (_print_all_pool)
    {
      guint page_count = _allocator->GetNbPools () / POOL_PAGE_DENSITY;

      return page_count + ((_allocator->GetNbPools () % POOL_PAGE_DENSITY) != 0);
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

    if (GetStageView (operation) == StageView::CLASSIFICATION)
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        classification->DrawBarePage (operation,
                                      context,
                                      page_nr);
      }
    }
    else if (   (GetStageView (operation) == StageView::RESULT)
             || (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("pool_classification_toggletoolbutton"))) == FALSE))
    {
      GList *pools = nullptr;

      if (_print_all_pool)
      {
        for (guint i = 0; i < POOL_PAGE_DENSITY; i ++)
        {
          Pool *pool = _allocator->GetPool (page_nr*POOL_PAGE_DENSITY + i);

          if (pool == nullptr)
          {
            break;
          }
          pools = g_list_append (pools,
                                 pool);
        }
      }
      else
      {
        pools = g_list_append (pools,
                               _displayed_pool);
      }

      {
        GooCanvas *canvas   = Canvas::CreatePrinterCanvas (context);
        gdouble    offset_v = 0.0;
        GList     *current  = pools;

        for (guint v = 0 ; v < _pool_v_density; v++)
        {
          GooCanvasBounds bounds;
          gdouble         offset_h = 0.0;

          for (guint h = 0 ; h < _pool_h_density; h++)
          {
            GooCanvasItem *root_item;
            Pool          *pool      = (Pool *) current->data;

            root_item = pool->DrawPage (operation,
                                        canvas);
            goo_canvas_item_get_bounds (root_item,
                                        &bounds);

            goo_canvas_item_translate (root_item,
                                       offset_h,
                                       offset_v);

            goo_canvas_convert_to_item_space (canvas,
                                              root_item,
                                              &bounds.x1,
                                              &bounds.y1);
            goo_canvas_convert_to_item_space (canvas,
                                              root_item,
                                              &bounds.x2,
                                              &bounds.y2);
            offset_h += (bounds.x2 - bounds.x1) * 1.0;

            pool->Wipe ();

            current = g_list_next (current);
            if (current == nullptr)
            {
              goto rendering;
            }
          }
          offset_v += (bounds.y2 - bounds.y1) * 1.2;
        }

rendering:
        {
          gdouble scale;
          gdouble canvas_x;
          gdouble canvas_y;
          gdouble canvas_w;
          gdouble canvas_h;
          gdouble paper_w  = gtk_print_context_get_width (context);
          gdouble paper_h  = gtk_print_context_get_height (context);
          gdouble header_h = GetPrintHeaderSize (context, SizeReferential::NORMALIZED);

          {
            GooCanvasBounds bounds;

            goo_canvas_item_get_bounds (goo_canvas_get_root_item (canvas),
                                        &bounds);
            canvas_x = bounds.x1;
            canvas_y = bounds.y1;
            canvas_w = bounds.x2 - bounds.x1;
            canvas_h = bounds.y2 - bounds.y1;
          }

          {
            gdouble canvas_dpi;
            gdouble printer_dpi;

            g_object_get (G_OBJECT (canvas),
                          "resolution-x", &canvas_dpi,
                          NULL);
            printer_dpi = gtk_print_context_get_dpi_x (context);

            scale = printer_dpi/canvas_dpi;
          }

          if (   (canvas_w*scale > paper_w)
              || (canvas_h*scale + header_h*paper_w/100 > paper_h))
          {
            gdouble x_scale = paper_w / canvas_w;
            gdouble y_scale = (paper_h - header_h*paper_w/100) / canvas_h;

            scale = MIN (x_scale, y_scale);
          }

          {
            cairo_t *cr = gtk_print_context_get_cairo_context (context);

            cairo_save (cr);

            cairo_translate (cr,
                             -canvas_x*scale,
                             -canvas_y*scale);
            cairo_scale (cr,
                         scale,
                         scale);

            goo_canvas_render (canvas,
                               cr,
                               nullptr,
                               1.0);
            cairo_restore (cr);
          }
        }
      }

      g_list_free (pools);
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
  void Supervisor::SetPrintPoolDensity ()
  {
    guint size = _allocator->GetBiggestPoolSize ();

    _pool_v_density = MIN (MAX (21 / size, 1), 4);
    _pool_h_density = MIN (MAX (14 / size, 1), 2);
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

      if (g_strcmp0 (provider_class->_xml_name, Allocator::_xml_class_name) == 0)
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
    LoadAttendees (nullptr);

    LoadConfiguration (xml_node);

    {
      guint nb_pools = _allocator->GetNbPools ();

      for (guint p = 0; p < nb_pools; p++)
      {
        Pool *pool = _allocator->GetPool (p);

        OnPoolStatus (pool);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Supervisor::DumpToHTML (FILE *file)
  {
    fprintf (file, "        <div>\n");
    fprintf (file, "          <table>\n");

    {
      guint nb_pools = _allocator->GetNbPools ();

      for (guint p = 0; p < nb_pools; p++)
      {
        Pool *pool = _allocator->GetPool (p);

        pool->DumpToHTML (file, _allocator->GetBiggestPoolSize ());
        fprintf (file, "          <tr class=\"PoolSeparator\">\n");
      }
    }

    fprintf (file, "          </table>\n\n");
    fprintf (file, "        </div>\n");
  }

  // --------------------------------------------------------------------------------
  GSList *Supervisor::GetCurrentClassification ()
  {
    GSList *result = nullptr;

    for (guint p = 0; p < _allocator->GetNbPools (); p++)
    {
      Pool *pool = _allocator->GetPool (p);

      for (GSList *current = pool->GetSortedFencerList (); current; current = g_slist_next (current))
      {
        Player *player = (Player *) current->data;

        result = g_slist_prepend (result,
                                  player);
      }
    }

    // Current round classification
    {
      result = EvaluateClassification (result,
                                       _current_round_owner,
                                       (GCompareDataFunc) CompareCurrentRoundClassification);

      UpdateClassification (_current_round_classification,
                            result);
    }

    // Combined classification
    if (_allocator->SeedingIsBalanced ())
    {
      return EvaluateClassification (result,
                                     this,
                                     (GCompareDataFunc) CompareCombinedRoundsClassification);
    }
    else
    {
      return EvaluateClassification (result,
                                     this,
                                     (GCompareDataFunc) CompareCurrentRoundClassification);
    }
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
    Player              *previous_player = nullptr;

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
  void Supervisor::OnForRefereesClicked (GtkToggleButton *toggle_button)
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("watermark_checkbutton"),
                              gtk_toggle_button_get_active (toggle_button));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_pool_filter_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Supervisor *p = dynamic_cast <Supervisor *> (owner);

    p->OnFilterClicked ("pool_classification_toggletoolbutton");
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_pool_combobox_changed (GtkWidget *widget,
                                                            Object    *owner)
  {
    Supervisor *p          = dynamic_cast <Supervisor *> (owner);
    gint        pool_index = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

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
  extern "C" G_MODULE_EXPORT void on_for_referees_radiobutton_clicked (GtkWidget *widget,
                                                                       Object    *owner)
  {
    Supervisor *ps = dynamic_cast <Supervisor *> (owner);

    ps->OnForRefereesClicked (GTK_TOGGLE_BUTTON (widget));
  }
}
