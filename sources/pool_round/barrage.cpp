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

#include "common/contest.hpp"
#include "common/classification.hpp"

#include "barrage.hpp"

namespace Pool
{
  const gchar *Barrage::_class_name     = N_ ("Barrage");
  const gchar *Barrage::_xml_class_name = "Barrage";

  // --------------------------------------------------------------------------------
  Barrage::Barrage (StageClass *stage_class)
    : Stage (stage_class),
    Module ("barrage.glade")
  {
    _pool = NULL;

    _max_score = new Data ("ScoreMax",
                           5);

    // Sensitive widgets
    {
      AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
      AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

      LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
    }

    {
      GtkWidget *content_area;

      _print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          gettext ("<b><big>Print...</big></b>"));

      gtk_window_set_title (GTK_WINDOW (_print_dialog),
                            gettext ("Barrage printing"));

      content_area = gtk_dialog_get_content_area (GTK_DIALOG (_print_dialog));

      gtk_widget_reparent (_glade->GetWidget ("print_dialog-vbox"),
                           content_area);
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
  }

  // --------------------------------------------------------------------------------
  Barrage::~Barrage ()
  {
    Object::TryToRelease (_pool);
    Object::TryToRelease (_max_score);

    gtk_widget_destroy (_print_dialog);
  }

  // --------------------------------------------------------------------------------
  void Barrage::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);
  }

  // --------------------------------------------------------------------------------
  Stage *Barrage::CreateInstance (StageClass *stage_class)
  {
    return new Barrage (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnLocked ()
  {
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnUnLocked ()
  {
  }

  // --------------------------------------------------------------------------------
  void Barrage::Display ()
  {
    _pool->SetDataOwner (this,
                        this,
                        this);

    _pool->SetFilter (_filter);
    //_pool->SetStatusCbk ((Pool::StatusCbk) OnPoolStatusUpdated,
                         //this);

    Plug (_pool,
          _glade->GetWidget ("main_hook"));

    ToggleClassification (FALSE);
  }

  // --------------------------------------------------------------------------------
  void Barrage::Wipe ()
  {
    if (_pool)
    {
      _pool->CleanScores ();
      _pool->Wipe ();

      _pool->UnPlug ();
      _pool = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnPlugged ()
  {
    Stage *previous_stage = GetPreviousStage ();

    _nb_qualified = previous_stage->_nb_qualified;
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnUnPlugged ()
  {
    _nb_qualified = NULL;
  }

  // --------------------------------------------------------------------------------
  GSList *Barrage::GetCurrentClassification ()
  {
    GSList *result = NULL;

    if (_pool)
    {
      GSList *short_list     = _attendees->GetShortList ();
      guint   exempted_count = g_slist_length (short_list) - _pool->GetNbPlayers ();

      result = _pool->GetCurrentClassification ();

      for (guint i = 0; i < exempted_count; i++)
      {
        result = g_slist_prepend (result, short_list->data);

        short_list = g_slist_next (short_list);
      }
    }

    {
      Player::AttributeId *attr_id = new Player::AttributeId ("rank", GetDataOwner ());
      GSList              *current = result;

      for (guint i = 1;  current != NULL; i++)
      {
        Player *player = (Player *) current->data;

        player->SetAttributeValue (attr_id,
                                   i);
        current = g_slist_next (current);
      }

      attr_id->Release ();
    }

    return result;
  }


  // --------------------------------------------------------------------------------
  void Barrage::Save (xmlTextWriter *xml_writer)
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST _xml_class_name);

    SaveConfiguration (xml_writer);
    SaveAttendees     (xml_writer);

    if (_pool)
    {
      _pool->Save (xml_writer);
    }

    xmlTextWriterEndElement (xml_writer);
  }

  // --------------------------------------------------------------------------------
  void Barrage::Load (xmlNode *xml_node)
  {
    LoadConfiguration (xml_node);

    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (strcmp ((char *) n->name, _xml_class_name) == 0)
        {
        }
        else if (strcmp ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          if (_pool == NULL)
          {
            LoadAttendees (n);
          }
          else
          {
            gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

            if (attr)
            {
              Player *player = GetFencerFromRef (atoi (attr));

              if (player)
              {
                _pool->AddFencer (player,
                                  this);
              }
              xmlFree (attr);
            }
          }
        }
        else if (strcmp ((char *) n->name, "Poule") == 0)
        {
          _pool = new Pool (_max_score,
                            1,
                            _contest->GetWeaponCode (),
                            GetXmlPlayerTag (),
                            _rand_seed);
        }
        else if (strcmp ((char *) n->name, "Arbitre") == 0)
        {
        }
        else if (strcmp ((char *) n->name, "Match") == 0)
        {
          _pool->CreateMatchs (NULL);
          _pool->Load (n,
                       _attendees->GetShortList ());
          return;
        }
        else
        {
          return;
        }
      }
      Load (n->children);
    }
  }

  // --------------------------------------------------------------------------------
  void Barrage::Garnish ()
  {
    _pool = new Pool (_max_score,
                      1,
                      _contest->GetWeaponCode (),
                      GetXmlPlayerTag (),
                      _rand_seed);

    _pool->SetFilter (_filter);

    if (_attendees)
    {
      GSList *barrage_list = GetBarrageList ();
      GSList *current      = barrage_list;

      while (current)
      {
        Player *fencer = (Player *) current->data;

        _pool->AddFencer (fencer,
                          this);

        current = g_slist_next (current);
      }
      g_slist_free (barrage_list);

      _pool->CreateMatchs (NULL);
    }
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnAttrListUpdated ()
  {
    if (_pool)
    {
      _pool->UnPlug ();
    }
    Display ();
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnFilterClicked ()
  {
    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("barrage_classification_toggletoolbutton"))))
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
  void Barrage::OnStuffClicked ()
  {
    if (_pool)
    {
      _pool->CleanScores ();
      _pool->Stuff ();
    }

    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  guint Barrage::PreparePrint (GtkPrintOperation *operation,
                               GtkPrintContext   *context)
  {
    if (GetStageView (operation) == STAGE_VIEW_CLASSIFICATION)
    {
      return 0;
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

    if (_pool)
    {
      _pool->Wipe ();
    }

    return 1;
  }

  // --------------------------------------------------------------------------------
  void Barrage::DrawPage (GtkPrintOperation *operation,
                          GtkPrintContext   *context,
                          gint               page_nr)
  {
    DrawContainerPage (operation,
                       context,
                       page_nr);

    if (   (GetStageView (operation) == STAGE_VIEW_RESULT)
        || (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("barrage_classification_toggletoolbutton"))) == FALSE))
    {
      _pool->DrawPage (operation,
                       context,
                       page_nr);
    }
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnEndPrint (GtkPrintOperation *operation,
                            GtkPrintContext   *context)
  {
    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnPrintPoolToolbuttonClicked ()
  {
    gchar *title          = NULL;
    Stage *previous_stage = GetPreviousStage ();

    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("barrage_classification_toggletoolbutton"))))
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        title = g_strdup_printf ("%s - %s", gettext ("Barrage classification"), previous_stage->GetName ());
        classification->Print (title);
      }
    }
    else if (gtk_dialog_run (GTK_DIALOG (_print_dialog)) == GTK_RESPONSE_OK)
    {
      title = g_strdup_printf ("%s - %s", gettext ("Barrage"), previous_stage->GetName ());
      Print (title);
    }
    g_free (title);
    gtk_widget_hide (_print_dialog);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_classification_toggletoolbutton_toggled (GtkToggleToolButton *widget,
                                                                                      Object              *owner)
  {
    Barrage *b = dynamic_cast <Barrage *> (owner);

    b->ToggleClassification (gtk_toggle_tool_button_get_active (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_filter_toolbutton_clicked (GtkWidget *widget,
                                                                        Object    *owner)
  {
    Barrage *t = dynamic_cast <Barrage *> (owner);

    t->OnFilterClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_stuff_toolbutton_clicked (GtkWidget *widget,
                                                                       Object    *owner)
  {
    Barrage *b = dynamic_cast <Barrage *> (owner);

    b->OnStuffClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_print_toolbutton_clicked (GtkWidget *widget,
                                                                       Object    *owner)
  {
    Barrage *b = dynamic_cast <Barrage *> (owner);

    b->OnPrintPoolToolbuttonClicked ();
  }
}
