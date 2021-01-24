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

#include <math.h>
#include <gdk/gdkkeysyms.h>
#include "util/data.hpp"
#include "util/glade.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "util/xml_scheme.hpp"
#include "util/global.hpp"
#include "util/attribute.hpp"
#include "network/message.hpp"
#include "network/ring.hpp"
#include "../../match.hpp"
#include "../../contest.hpp"
#include "../../score.hpp"

#include "wheel_of_fortune.hpp"
#include "point_system.hpp"
#include "poule.hpp"

namespace Quest
{
  const gchar *Poule::_class_name     = N_ ("Pool");
  const gchar *Poule::_xml_class_name = "RondeSuisse";

  GdkPixbuf     *Poule::_moved_pixbuf = nullptr;
  const gdouble  Poule::_score_rect_w = 45.0;
  const gdouble  Poule::_score_rect_h = 30.0;

  // --------------------------------------------------------------------------------
  Poule::Poule (StageClass *stage_class)
    : Object ("Quest::Poule"),
    Stage (stage_class),
    CanvasModule ("quest_supervisor.glade")
  {
    {
      Net::Message *parcel = Disclose ("BellePoule::JobList");

      parcel->Set ("channel", (guint) Net::Ring::Channel::WEB_SOCKET);
    }

    SetZoomer (GTK_RANGE (_glade->GetWidget ("zoom_scale")));
    ZoomTo (1.0);

    {
      _piste_entry      = GTK_WIDGET (_glade->GetWidget ("piste_entry"));
      _per_fencer_entry = GTK_WIDGET (_glade->GetWidget ("per_fencer_entry"));
      _duration_entry   = GTK_WIDGET (_glade->GetWidget ("duration_entry"));

      _per_fencer_entry_handler = g_signal_connect (G_OBJECT (_per_fencer_entry), "changed",
                                                    G_CALLBACK (on_per_fencer_entry_changed),
                                                    (Object *) this);
      _duration_entry_handler = g_signal_connect (G_OBJECT (_duration_entry), "changed",
                                                  G_CALLBACK (on_duration_entry_changed),
                                                  (Object *) this);
    }

    _muted = FALSE;

    _point_system = new PointSystem (this);

     _matches = nullptr;

     {
       _score_collector = new ScoreCollector (this,
                                              FALSE);

       _score_collector->SetConsistentColors ("LightGrey",
                                              "SkyBlue");
     }

    _max_score = new Data ("ScoreMax",
                           15);

    {
      _nb_qualified->Release ();
      _nb_qualified = new Data ("NbQualifiesParIndice",
                                8);
      ActivateNbQualified ();
    }

    _matches_per_fencer = new Data ("MatchsParTireur",
                                    7);

    _available_time = new Data ("TempsDisponible",
                                4);
    _available_time->Deactivate ();

    _hall = new Hall (this);

    _piste_count = new Data ("NbPistes",
                             4);
    _hall->SetPisteCount (_piste_count->GetValue ());

    Plug (_hall,
          _glade->GetWidget ("hall_hook"));

    {
      AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
      AddSensitiveWidget (_glade->GetWidget ("qualified_viewport"));
      AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));
      AddSensitiveWidget (_glade->GetWidget ("control_panel"));

      LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
      LockOnClassification (_glade->GetWidget ("control_panel"));
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
                                          "incident",
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "score_quest",
                                          "nb_matchs",
                                          "elo",
                                          "initial_elo",
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
      filter->ShowAttribute ("club");

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
                                            "incident",
                                            "IP",
                                            "password",
                                            "cyphered_password",
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
                                            "status",
                                            "team",
                                            "bouts_count",
                                            "victories_ratio",
                                            "strip",
                                            "time",
                                            NULL);
        filter = new Filter (GetKlassName (),
                             attr_list);

        filter->ShowAttribute ("rank");
        filter->ShowAttribute ("name");
        filter->ShowAttribute ("first_name");
        filter->ShowAttribute ("club");
        filter->ShowAttribute ("victories_count");
        filter->ShowAttribute ("score_quest");
        filter->ShowAttribute ("nb_matchs");
        filter->ShowAttribute ("elo");

        SetClassificationFilter (filter);
        filter->Release ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  Poule::~Poule ()
  {
    Reset ();

    _max_score->Release          ();
    _matches_per_fencer->Release ();
    _available_time->Release     ();
    _piste_count->Release        ();
    _hall->Release               ();
    _point_system->Release       ();

    Object::TryToRelease (_score_collector);
  }

  // --------------------------------------------------------------------------------
  void Poule::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);

    {
      GtkWidget *image;

      image = gtk_image_new ();
      g_object_ref_sink (image);
      _moved_pixbuf = gtk_widget_render_icon (image,
                                              GTK_STOCK_REFRESH,
                                              GTK_ICON_SIZE_BUTTON,
                                              nullptr);
      g_object_unref (image);
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::Reset ()
  {
    _hall->Clear ();

    _point_system->Reset ();

    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      for (guint i = 0; i < 2; i++)
      {
        Player *fencer  = match->GetOpponent (i);
        GList  *matches = (GList *) fencer->GetPtrData (this, "Poule::matches");

        g_list_free (matches);
        fencer->RemoveData (this, "Poule::matches");
      }

      match->RemoveData (this, "Poule::match_gooitem");
    }

    FreeFullGList (Match,
                   _matches);
    _matches = nullptr;

    Recall ();
  }

  // --------------------------------------------------------------------------------
  Stage *Poule::CreateInstance (StageClass *stage_class)
  {
    return new Poule (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Poule::Load (xmlNode *xml_node)
  {
    _muted = TRUE;

    _hall->Clear ();
    LoadConfiguration (xml_node);
    Garnish ();
    LoadMatches (xml_node);
    _point_system->Rehash ();
    RefreshClassification ();
  }

  // --------------------------------------------------------------------------------
  void Poule::LoadMatches (xmlNode *xml_node)
  {
    static xmlNode  *match_node = nullptr;
    static xmlNode  *stop_node  = nullptr;

    for (xmlNode *n = xml_node; n != nullptr; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (n->parent == stop_node)
        {
          match_node = nullptr;
          stop_node  = nullptr;
          return;
        }
        else if (g_strcmp0 ((char *) n->name, _xml_class_name) == 0)
        {
          stop_node = n->parent;
        }
        else if (g_strcmp0 ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          if (match_node == nullptr)
          {
            LoadAttendees (n);
          }
        }
        else if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          gchar *number = nullptr;

          match_node = n;

          {
            gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");

            if (attr)
            {
              number = g_strdup (attr);

              xmlFree (attr);
            }
          }

          if (number)
          {
            for (GList *m = _matches; m; m = g_list_next (m))
            {
              Match       *match = (Match *) m->data;
              const gchar *id    = match->GetName ();

              if (id)
              {
                if (g_strcmp0 (id, number) == 0)
                {
                  for (guint i = 0; i < 2; i++)
                  {
                    LoadMatch (n,
                               match);
                  }

                  if (   match->GetPiste ()
                      || (MatchIsFinished (match) == FALSE))
                  {
                    _hall->BookPiste (match);
                  }

                  _point_system->RateMatch (match);

                  break;
                }
              }
            }

            g_free (number);
          }
        }
        else
        {
          return;
        }
      }
      LoadMatches (n->children);
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::SaveConfiguration (XmlScheme *xml_scheme)
  {
    Stage::SaveConfiguration (xml_scheme);

    _matches_per_fencer->Save (xml_scheme);
    _available_time->Save     (xml_scheme);
    _piste_count->Save        (xml_scheme);
  }

  // --------------------------------------------------------------------------------
  void Poule::LoadConfiguration (xmlNode *xml_node)
  {
    Stage::LoadConfiguration (xml_node);

    if (_matches_per_fencer->Load (xml_node) == FALSE)
    {
      _matches_per_fencer->Deactivate ();
    }

    if (_available_time->Load (xml_node) == FALSE)
    {
      _available_time->Deactivate ();
    }

    _piste_count->Load (xml_node);
    _hall->SetPisteCount (_piste_count->GetValue ());
  }

  // --------------------------------------------------------------------------------
  void Poule::SaveHeader (XmlScheme *xml_scheme)
  {
    xml_scheme->StartElement (_xml_class_name);

    SaveConfiguration (xml_scheme);

    if (_drop_zones)
    {
      xml_scheme->WriteFormatAttribute ("NbDePoules",
                                        "%d", g_slist_length (_drop_zones));
    }
    xml_scheme->WriteFormatAttribute ("PhaseSuivanteDesQualifies",
                                      "%d", GetId ()+2);
  }

  // --------------------------------------------------------------------------------
  void Poule::Save (XmlScheme *xml_scheme)
  {
    SaveHeader (xml_scheme);

    Stage::SaveAttendees (xml_scheme);

    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      match->Save (xml_scheme);
    }

    xml_scheme->EndElement ();
  }

  // --------------------------------------------------------------------------------
  void Poule::FillInConfig ()
  {
    Stage::FillInConfig ();

    if (_matches_per_fencer->IsValid ())
    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("per_fencer_entry"));

      if (w)
      {
        gchar *text = g_strdup_printf ("%d", _matches_per_fencer->GetValue ());

        gtk_entry_set_text (w,
                            text);
        g_free (text);
      }
    }

    if (_available_time->IsValid ())
    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("duration_entry"));

      if (w)
      {
        gchar *text = g_strdup_printf ("%d", _available_time->GetValue ());

        gtk_entry_set_text (w,
                            text);
        g_free (text);
      }
    }

    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("piste_entry"));

      if (w)
      {
        gchar *text = g_strdup_printf ("%d", _piste_count->GetValue ());

        gtk_entry_set_text (w,
                            text);
        g_free (text);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::Garnish ()
  {
    GSList *fencers            = g_slist_copy (GetShortList ());
    guint   fencer_count       = g_slist_length (fencers);
    guint   matches_per_fencer = MIN (_matches_per_fencer->GetValue (), fencer_count-1);

    for (guint i = 0; i < matches_per_fencer; i++)
    {
      TossMatches (fencers,
                   i+1);
      fencers = g_slist_reverse (fencers);
    }

    g_slist_free (fencers);
  }

  // --------------------------------------------------------------------------------
  void Poule::TossMatches (GSList *fencers,
                           guint   matches_per_fencer)
  {
    WheelOfFortune *wheel = new WheelOfFortune (fencers,
                                                GetRandSeed ());

    for (GSList *f = fencers; f; f = g_slist_next (f))
    {
      Player *fencer  = (Player *) f->data;
      GList  *matches = (GList *) fencer->GetPtrData (this, "Poule::matches");

      for (guint match_count = g_list_length (matches);
           match_count < matches_per_fencer;
           match_count++)
      {
        for (void *o = wheel->Turn (); o; o = wheel->TryAgain ())
        {
          Player *opponent         = (Player *) o;
          GList  *opponent_matches = (GList *) opponent->GetPtrData (this, "Poule::matches");

          if (   (opponent != fencer)
              && (FencerHasMatch (opponent, matches) == FALSE)
              && (g_list_length (opponent_matches) < matches_per_fencer))
          {
            Match *match = new Match (fencer,
                                      opponent,
                                      _max_score,
                                      TRUE);

            matches = g_list_prepend (matches,
                                      match);

            opponent_matches = g_list_prepend (opponent_matches,
                                               match);
            opponent->SetData (this, "Poule::matches",
                               opponent_matches);

            _matches = g_list_append (_matches,
                                      match);
            match->SetNumber (g_list_length (_matches));

            break;
          }
        }
      }
      fencer->SetData (this, "Poule::matches",
                       matches);
    }

    wheel->Release ();
  }

  // --------------------------------------------------------------------------------
  void Poule::Dump ()
  {
    GSList *fencers = GetShortList ();

    for (GSList *f = fencers; f; f = g_slist_next (f))
    {
      Player *fencer  = (Player *) f->data;
      GList  *matches = (GList *) fencer->GetPtrData (this, "Poule::matches");

      for (GList *m = matches; m; m = g_list_next (m))
      {
        Match *match = (Match *) m->data;

        if (match->GetOpponent (0) != fencer)
        {
          printf ("%s, ", match->GetOpponent (0)->GetName ());
        }
        else
        {
          printf ("%s, ", match->GetOpponent (1)->GetName ());
        }
      }
      printf ("\n");
    }
    printf ("\n");
  }

  // --------------------------------------------------------------------------------
  void Poule::OnMatchSelected (Match *match)
  {
    GooCanvasItem *item = (GooCanvasItem *) match->GetPtrData (this, "Poule::match_gooitem");

    if (item)
    {
      GooCanvasBounds bounds;

      goo_canvas_item_get_bounds (item,
                                  &bounds);
      Swipe (0,
             bounds.y1);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Poule::MatchIsFinished (Match *match)
  {
    return match->IsOver () || MatchIsCancelled (match);
  }

  // --------------------------------------------------------------------------------
  void Poule::Wipe ()
  {
    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      for (guint i = 0; i < 2; i++)
      {
        Player *fencer = match->GetOpponent (i);

        fencer->RemoveData (match, "Poule::highlight");
      }

      _score_collector->RemoveCollectingPoints (match);
    }

    CanvasModule::Wipe ();
  }

  // --------------------------------------------------------------------------------
  void Poule::Display ()
  {
    GooCanvasItem *root = GetRootItem ();

    Wipe ();

    if (root)
    {
      guint ongoings = 0;

      _table = goo_canvas_table_new (root,
                                     "column-spacing", 10.0,
                                     "row-spacing",    10.0,
                                     NULL);

      _rows = 0;
      for (GList *m = _matches; m; m = g_list_next (m))
      {
        Match *match = (Match *) m->data;

        if (   match->GetPiste ()
            && match->HasError ())
        {
          ongoings++;
          _hall->BookPiste (match);
        }
      }

      printf ("=====>> %d\n", g_list_length (_matches));
      for (GList *m = _matches; m; m = g_list_next (m))
      {
        Match *match = (Match *) m->data;

        if (match->HasError () == FALSE)
        {
          if (   match->GetPiste ()
              || (MatchIsFinished (match) == FALSE))
          {
            ongoings++;
            if (ongoings > _piste_count->GetValue ())
            {
              continue;
            }
          }

          if (MatchIsFinished (match) == FALSE)
          {
            _hall->BookPiste (match);
          }
        }

        DisplayMatch (match);
        RefreshMatch (match);
      }

      if (_muted == FALSE)
      {
        Spread ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Poule::MatchIsCancelled (Match *match)
  {
    if (match->IsOver () == FALSE)
    {
      Player::AttributeId status_attr_id ("status", GetDataOwner ());

      for (guint i = 0; i < 2; i++)
      {
        Player    *fencer      = match->GetOpponent (i);
        Attribute *status_attr = fencer->GetAttribute (&status_attr_id);

        if (status_attr)
        {
          gchar *status = status_attr->GetStrValue ();

          if (   (status && status[0] == 'A')
              || (status && status[0] == 'E'))
          {
            return TRUE;
          }
        }
      }
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Poule::DisplayMatch (Match *match)
  {
    GooCanvasItem *item;
    GooCanvasItem *collecting_point_A;
    GooCanvasItem *collecting_point_B;
    gboolean       cancelled          = MatchIsCancelled (match);
    guint          column             = 0;

    // Falsification
    {
      if (match->IsFalsified ())
      {
        item = Canvas::PutPixbufInTable (_table,
                                         _moved_pixbuf,
                                         _rows, column);
        Canvas::SetTableItemAttribute (item, "y-align", 0.5);
      }
      column++;
    }

    // Number
    {
      item = Canvas::PutTextInTable (_table,
                                     match->GetName (),
                                     _rows, column++);
      g_object_set (G_OBJECT (item),
                    "font", BP_FONT "18px",
                    NULL);
      Canvas::SetTableItemAttribute (item, "y-align", 0.5);

      match->SetData (this, "Poule::match_gooitem",
                      item);
    }

    // Fencers
    {
      GString *common_markup = g_string_new ("font_desc=\"" BP_FONT "14.0px\"");

      if (cancelled)
      {
        common_markup = g_string_append (common_markup,
                                         " strikethrough=\"yes\"");
      }

      for (guint i = 0; i < 2; i++)
      {
        Player *fencer = match->GetOpponent (i);

        if (i%2 != 0)
        {
          if (cancelled == FALSE)
          {
            collecting_point_A = DisplayScore (_table,
                                               _rows,
                                               column,
                                               match,
                                               fencer);
          }
          column++;
        }

        item = GetPlayerImage (_table,
                               common_markup->str,
                               fencer,
                               nullptr,
                               "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                               "first_name", "foreground=\"darkblue\"",
                               "club",       "style=\"italic\" foreground=\"dimgrey\"",
                               "league",     "style=\"italic\" foreground=\"dimgrey\"",
                               "region",     "style=\"italic\" foreground=\"dimgrey\"",
                               "country",    "style=\"italic\" foreground=\"dimgrey\"",
                               NULL);

        fencer->SetData (match, "Poule::highlight",
                         item);

        Canvas::PutInTable (_table,
                            item,
                            _rows, column++);
        Canvas::SetTableItemAttribute (item, "y-align", 0.5);

        if (i%2 == 0)
        {
          if (cancelled == FALSE)
          {
            collecting_point_B = DisplayScore (_table,
                                               _rows,
                                               column,
                                               match,
                                               fencer);
          }
          column++;
        }
      }
      g_string_free (common_markup,
                     TRUE);
    }

    if (cancelled == FALSE)
    {
      _score_collector->SetNextCollectingPoint (collecting_point_A,
                                                collecting_point_B);
      _score_collector->SetNextCollectingPoint (collecting_point_B,
                                                collecting_point_A);
    }

    // Piste
    if (cancelled == FALSE)
    {
      gchar *piste = g_strdup_printf ("%s %d", gettext ("Piste"), match->GetPiste ());

      item = Canvas::PutTextInTable (_table,
                                     piste,
                                     _rows, column++);
      g_object_set (G_OBJECT (item),
                    "font", BP_FONT "bold 18px",
                    NULL);
      Canvas::SetTableItemAttribute (item, "y-align", 0.5);

      g_free (piste);

      match->SetData (this,
                      "Poule::piste",
                      item);
    }

    // Button
    {
      GtkWidget *button = gtk_button_new_with_label (gettext ("Validate"));
      GtkWidget *image  = gtk_image_new_from_stock (GTK_STOCK_APPLY,
                                                    GTK_ICON_SIZE_BUTTON);

      gtk_button_set_image (GTK_BUTTON (button),
                            image);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (OnMatchValidated), this);

      g_object_set_data (G_OBJECT (button), "Button::match", match);

      item = goo_canvas_widget_new (_table,
                                    button,
                                    0,
                                    0,
                                    -1,
                                    -1,
                                    "anchor", GTK_ANCHOR_CENTER,
                                    NULL);

      Canvas::PutInTable (_table,
                          item,
                          _rows, column++);

      match->SetData (this,
                      "Poule::button",
                      item);
    }

    _rows++;
  }

  // --------------------------------------------------------------------------------
  GooCanvasItem *Poule::DisplayScore (GooCanvasItem *table,
                                      guint          row,
                                      guint          column,
                                      Match         *match,
                                      Player        *fencer)
  {
    GooCanvasItem *score_table = goo_canvas_table_new (table, NULL);
    GooCanvasItem *score_rect;

    Canvas::PutInTable (table,
                        score_table,
                        row,
                        column);
    Canvas::SetTableItemAttribute (score_table, "x-align", 1.0);
    Canvas::SetTableItemAttribute (score_table, "x-expand", 1u);
    Canvas::SetTableItemAttribute (score_table, "y-align", 0.5);

    // score_rect
    {
      score_rect = goo_canvas_rect_new (score_table,
                                        0, 0,
                                        _score_rect_w, _score_rect_h,
                                        "stroke-pattern", NULL,
                                        "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                        NULL);

      Canvas::PutInTable (score_table,
                          score_rect,
                          0,
                          0);
    }

    // arrow_icon
    {
      GooCanvasItem *item;
      static gchar  *arrow_icon = nullptr;

      if (arrow_icon == nullptr)
      {
        arrow_icon = g_build_filename (Global::_share_dir, "resources/glade/images/arrow.png", NULL);
      }
      item = Canvas::PutIconInTable (score_table,
                                     arrow_icon,
                                     0,
                                     0);
      Canvas::SetTableItemAttribute (item, "x-align", 1.0);
      Canvas::SetTableItemAttribute (item, "y-align", 0.0);

      g_object_set_data (G_OBJECT (item), "Arrow::match", match);
      g_object_set_data (G_OBJECT (item), "Arrow::fencer", fencer);
      g_signal_connect (item, "button_press_event",
                        G_CALLBACK (OnStatusArrowPress), this);
    }

    // score_text
    {
      GooCanvasItem *item = goo_canvas_text_new (score_table,
                                                 "",
                                                 0, 0,
                                                 -1,
                                                 GTK_ANCHOR_CENTER,
                                                 "font", BP_FONT "bold 20px",
                                                 NULL);

      Canvas::PutInTable (score_table,
                          item,
                          0,
                          0);
      Canvas::SetTableItemAttribute (item, "x-align", 0.5);
      Canvas::SetTableItemAttribute (item, "y-align", 1.0);

      _score_collector->AddCollectingPoint (score_rect,
                                            item,
                                            match,
                                            fencer);
    }

    // score_image (status icon)
    {
      Score         *score = match->GetScore (fencer);
      GooCanvasItem *item  = Canvas::PutPixbufInTable (score_table,
                                                       nullptr,
                                                       0,
                                                       0);

      Canvas::SetTableItemAttribute (item, "x-align", 0.5);
      Canvas::SetTableItemAttribute (item, "y-align", 0.5);

      score->SetData (this,
                      "Poule::score_status",
                      item);
    }

    return score_rect;
  }

  // --------------------------------------------------------------------------------
  gboolean Poule::OnStatusArrowPress (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event,
                                      Poule          *poule)
  {
    if (   (event->button == 1)
        && (event->type   == GDK_BUTTON_PRESS))
    {
      GtkWidget *combo;
      Match     *match  = (Match *)  g_object_get_data (G_OBJECT (item), "Arrow::match");
      Player    *fencer = (Player *) g_object_get_data (G_OBJECT (item), "Arrow::fencer");

      {
        GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();

        combo = gtk_combo_box_new_with_model (poule->GetStatusModel ());

        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
                                        cell, "pixbuf", AttributeDesc::DiscreteColumnId::ICON_pix,
                                        NULL);

        goo_canvas_widget_new (poule->GetRootItem (),
                               combo,
                               event->x_root,
                               event->y_root,
                               -1,
                               -1,
                               NULL);
        gtk_widget_grab_focus (combo);
      }

      {
        GtkTreeIter  iter;
        gboolean     iter_is_valid;
        gchar       *code;
        gchar        current_status = 'Q';

        if (match->IsDropped ())
        {
          Score *score = match->GetScore (fencer);

          if (score && score->IsOut ())
          {
            current_status = score->GetDropReason ();
          }
        }

        iter_is_valid = gtk_tree_model_get_iter_first (poule->GetStatusModel (),
                                                       &iter);
        while (iter_is_valid)
        {
          gtk_tree_model_get (poule->GetStatusModel (),
                              &iter,
                              AttributeDesc::DiscreteColumnId::XML_IMAGE_str, &code,
                              -1);
          if (current_status == code[0])
          {
            gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo),
                                           &iter);

            g_free (code);
            break;
          }

          g_free (code);
          iter_is_valid = gtk_tree_model_iter_next (poule->GetStatusModel (),
                                                    &iter);
        }
      }

      {
        g_object_set_data (G_OBJECT (combo), "player_for_status", fencer);
        g_object_set_data (G_OBJECT (combo), "match_for_status",  match);
        g_signal_connect (combo, "changed",
                          G_CALLBACK (OnStatusChanged), poule);
        g_signal_connect (combo, "key-press-event",
                          G_CALLBACK (OnStatusKeyPressEvent), poule);
      }
    }

    return FALSE;
  }


  // --------------------------------------------------------------------------------
  void Poule::OnStatusChanged (GtkComboBox *combo_box,
                               Poule       *poule)
  {
    {
      Match  *match  = (Match *) g_object_get_data (G_OBJECT (combo_box), "match_for_status");
      Player *fencer = (Player *) g_object_get_data (G_OBJECT (combo_box), "player_for_status");

      {
        GtkTreeIter  iter;
        gchar       *code;

        gtk_combo_box_get_active_iter (combo_box,
                                       &iter);
        gtk_tree_model_get (poule->GetStatusModel (),
                            &iter,
                            AttributeDesc::DiscreteColumnId::XML_IMAGE_str, &code,
                            -1);

        if (code && *code !='Q')
        {
          match->DropFencer (fencer,
                             code);
        }
        else
        {
          match->RestoreFencer (fencer);
        }

        {
          Player::AttributeId status_attr_id ("status", poule->GetDataOwner ());

          fencer->SetAttributeValue (&status_attr_id,
                                     code);
        }

        g_free (code);
      }

      poule->OnNewScore (nullptr,
                         match,
                         fencer);
      poule->_point_system->Rehash ();
      poule->RefreshClassification ();
    }

    gtk_widget_destroy (GTK_WIDGET (combo_box));
  }

  // --------------------------------------------------------------------------------
  gboolean Poule::OnStatusKeyPressEvent (GtkWidget   *widget,
                                         GdkEventKey *event,
                                         Poule       *poule)
  {
    if (event->keyval == GDK_KEY_Escape)
    {
      gtk_widget_destroy (GTK_WIDGET (widget));
      return TRUE;
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Poule::FencerHasMatch (Player *fencer,
                                  GList  *matches)
  {
    for (GList *m = matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      for (guint i = 0; i < 2; i++)
      {
        if (match->GetOpponent (i) == fencer)
        {
          return TRUE;
        }
      }
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gint Poule::ComparePlayer (Player *A,
                             Player *B,
                             Poule  *poule)
  {
    return poule->_point_system->Compare (A,
                                          B);
  }

  // --------------------------------------------------------------------------------
  void Poule::OnPlugged ()
  {
    CanvasModule::OnPlugged ();
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("quest_classification_toggletoolbutton")),
                                       FALSE);

    SynchronizeConfiguration (GTK_EDITABLE (_piste_entry));
  }

  // --------------------------------------------------------------------------------
  void Poule::OnAttrListUpdated ()
  {
    _muted = TRUE;
    Display ();
    _muted = FALSE;
  }

  // --------------------------------------------------------------------------------
  GSList *Poule::GetCurrentClassification ()
  {
    GSList *result = g_slist_copy (GetShortList ());

    result = g_slist_sort_with_data (result,
                                     (GCompareDataFunc) ComparePlayer,
                                     this);

    {
      Player::AttributeId  rank_attr_id  ("rank", this);
      guint                rank = 1;

      for (GSList *f = result; f; f = g_slist_next (f))
      {
        Player *fencer = (Player *) f->data;

        fencer->SetAttributeValue (&rank_attr_id,
                                   rank++);
      }
    }

    return result;
  }

  // --------------------------------------------------------------------------------
  void Poule::OnNewScore (ScoreCollector *score_collector,
                          Match          *match,
                          Player         *player)
  {
    match->Timestamp ();

    _score_collector->Refresh (match);
    RefreshMatch (match);

    _point_system->RateMatch (match);

    if (score_collector)
    {
      _point_system->Rehash ();
      RefreshClassification ();
    }

    gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("per_fencer_entry")),
                              FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("duration_entry")),
                              FALSE);

    MakeDirty ();
    //OnRoundOver

    if (_muted == FALSE)
    {
      Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::RefreshMatch (Match *match)
  {
    for (guint i = 0; i < 2; i++)
    {
      Player *fencer = match->GetOpponent (i);
      Score  *score  = match->GetScore (fencer);

      if (score)
      {
        GooCanvasItem *status_item = (GooCanvasItem *) score->GetPtrData (this, "Poule::score_status");

        if (status_item)
        {
          AttributeDesc *attr_desc = AttributeDesc::GetDescFromCodeName ("status");
          GdkPixbuf     *pixbuf    = attr_desc->GetDiscretePixbuf (score->GetDropReason ());

          g_object_set (G_OBJECT (status_item),
                        "visibility", GOO_CANVAS_ITEM_HIDDEN,
                        NULL);
          if (pixbuf)
          {
            g_object_set (G_OBJECT (score->GetPtrData (this, "Poule::score_status")),
                          "pixbuf",     pixbuf,
                          "visibility", GOO_CANVAS_ITEM_VISIBLE,
                          NULL);

            g_object_unref (pixbuf);
          }
        }
      }
    }

    g_object_set (G_OBJECT (match->GetPtrData (this, "Poule::piste")),
                  "visibility", GOO_CANVAS_ITEM_HIDDEN,
                  nullptr);
    g_object_set (G_OBJECT (match->GetPtrData (this, "Poule::button")),
                  "visibility", GOO_CANVAS_ITEM_HIDDEN,
                  nullptr);
    _hall->SetStatusIcon (match,
                          nullptr);

    if (MatchIsFinished (match))
    {
      g_object_set (G_OBJECT (match->GetPtrData (this, "Poule::piste")),
                    "font",       BP_FONT "18px",
                    "fill-color", "grey",
                    nullptr);

      if (match->GetPiste ())
      {
        g_object_set (G_OBJECT (match->GetPtrData (this, "Poule::piste")),
                      "visibility", GOO_CANVAS_ITEM_VISIBLE,
                      nullptr);
        g_object_set (G_OBJECT (match->GetPtrData (this, "Poule::button")),
                      "visibility", GOO_CANVAS_ITEM_VISIBLE,
                      nullptr);
        _hall->SetStatusIcon (match,
                              GTK_STOCK_APPLY);
      }
    }
    else
    {
      g_object_set (G_OBJECT (match->GetPtrData (this, "Poule::piste")),
                    "font",       BP_FONT "bold 18px",
                    "fill-color", "dark",
                    "visibility", GOO_CANVAS_ITEM_VISIBLE,
                    nullptr);
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::OnPisteCountChanged (GtkEditable *editable)
  {
    gchar *str = (gchar *) gtk_entry_get_text (GTK_ENTRY (editable));

    if (str)
    {
      _piste_count->SetValue (atoi (str));
      _hall->SetPisteCount (_piste_count->GetValue ());

      SynchronizeConfiguration (editable);
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::OnMatchesPerFencerChanged (GtkEditable *editable)
  {
    gchar *str = (gchar *) gtk_entry_get_text (GTK_ENTRY (editable));

    if (str)
    {
      _matches_per_fencer->SetValue (atoi (str));
      _matches_per_fencer->Activate ();
      _available_time->Deactivate   ();

      SynchronizeConfiguration (editable);
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::OnDurationChanged (GtkEditable *editable)
  {
    gchar *str = (gchar *) gtk_entry_get_text (GTK_ENTRY (editable));

    if (str)
    {
      _available_time->SetValue (atoi (str));
      _matches_per_fencer->Deactivate ();
      _available_time->Activate       ();

      SynchronizeConfiguration (editable);
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::SynchronizeConfiguration (GtkEditable *editable)
  {
    GtkWidget *entry_to_block   = _duration_entry;
    gulong     handler_to_block = _duration_entry_handler;

    if (GTK_WIDGET (editable) == _piste_entry)
    {
      if (_available_time->IsValid ())
      {
        entry_to_block   = _per_fencer_entry;
        handler_to_block = _per_fencer_entry_handler;
      }
    }
    else if (GTK_WIDGET (editable) == _duration_entry)
    {
      entry_to_block   = _per_fencer_entry;
      handler_to_block = _per_fencer_entry_handler;
    }

    {
      gboolean round_started = FALSE;

      for (GList *m = _matches; m; m = g_list_next (m))
      {
        Match *match = (Match *) m->data;

        if (match->IsStarted ())
        {
          round_started = TRUE;
        }
        break;
      }

      if (round_started)
      {
        gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("per_fencer_entry")),
                                  FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("duration_entry")),
                                  FALSE);
      }
      else
      {
        gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("per_fencer_entry")),
                                  TRUE);
        gtk_widget_set_sensitive (GTK_WIDGET (_glade->GetWidget ("duration_entry")),
                                  TRUE);
        Reset ();
        Garnish ();
      }
    }

    if (_piste_count->GetValue ())
    {
      guint fencers = g_slist_length (GetShortList ());

      g_signal_handler_block (entry_to_block,
                              handler_to_block);

      if (entry_to_block == _duration_entry)
      {
        gchar *text;

        if (_matches_per_fencer->GetValue () > fencers-1)
        {
          text = g_strdup_printf ("%d", fencers-1);

          gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("per_fencer_entry")),
                              text);
        }
        else
        {
          guint matches    = (fencers * _matches_per_fencer->GetValue ()) / 2;
          guint time_slots = matches / _piste_count->GetValue ();
          guint duration   = (time_slots*15) / 60;

          if (duration == 0)
          {
            text = g_strdup ("<1");
          }
          else
          {
            text = g_strdup_printf ("%d", duration);
          }
          gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("duration_entry")),
                              text);
        }
        g_free (text);
      }
      else
      {
        guint  matches    = _available_time->GetValue () * 4 * _piste_count->GetValue ();
        guint  grid_size  = (1 + sqrt (1 + 8*matches)) / 2;
        guint  per_fencer = MIN (fencers-1, grid_size-1);
        gchar *text       = g_strdup_printf ("%d", per_fencer);

        gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("per_fencer_entry")),
                            text);
        g_free (text);
      }

      g_signal_handler_unblock (entry_to_block,
                                handler_to_block);

      Display ();
    }

    MakeDirty ();
  }

  // --------------------------------------------------------------------------------
  void Poule::OnHighlightChanged (GtkEditable *editable)
  {
    gchar *str      = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
    gchar *stripped = g_strstrip (str);
    gchar *color    = (gchar *) "yellow";

    if ((str == nullptr) || (str[0] == '\0'))
    {
      color = (gchar *) "white";
    }

    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      if (match->GetPtrData (this, "Poule::match_gooitem"))
      {
        for (guint i = 0; i < 2; i++)
        {
          Player        *fencer = match->GetOpponent (i);
          gchar         *name   = fencer->GetName ();
          GooCanvasItem *item   = (GooCanvasItem *) fencer->GetPtrData (match, "Poule::highlight");

          if (g_str_match_string (str,
                                  name,
                                  TRUE))
          {
            g_object_set (G_OBJECT (item),
                          "fill-color", color,
                          nullptr);
          }
          else
          {
            g_object_set (G_OBJECT (item),
                          "fill-color", "white",
                          nullptr);
          }

          g_free (name);
        }
      }
    }
    g_free (stripped);
  }

  // --------------------------------------------------------------------------------
  void Poule::OnStuffClicked ()
  {
    _muted = TRUE;
    {
      Reset ();
      Garnish ();
      Display ();

      for (GList *m = _matches; m; m = g_list_next (m))
      {
        Match  *match;
        Player *A;
        Player *B;
        gint    score;

        match = (Match *) m->data;
        A     = match->GetOpponent (0);
        B     = match->GetOpponent (1);
        score = g_random_int_range (0,
                                    _max_score->GetValue ());

        if (g_random_boolean ())
        {
          match->SetScore (A, _max_score->GetValue (), TRUE);
          match->SetScore (B, score, FALSE);
        }
        else
        {
          match->SetScore (A, score, FALSE);
          match->SetScore (B, _max_score->GetValue (), TRUE);
        }

        OnNewScore (nullptr,
                    match,
                    nullptr);
      }

      _point_system->Rehash ();
      RefreshClassification ();
    }
    _muted = FALSE;

    Spread ();
  }

  // --------------------------------------------------------------------------------
  void Poule::OnLoadingCompleted ()
  {
    _muted = FALSE;

    if (Locked () == FALSE)
    {
      Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  void Poule::FeedParcel (Net::Message *parcel)
  {
    parcel->Set ("competition", _contest->GetNetID ());
    parcel->Set ("stage",       GetNetID ());
    parcel->Set ("name",        GetId ());
    parcel->Set ("done",        Locked ());

    parcel->Set ("client", 0);

    {
      xmlBuffer *xml_buffer = xmlBufferCreate ();

      {
        XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

        _contest->SaveHeader (xml_scheme);
        SaveHeader (xml_scheme);

        for (GList *m = _matches; m; m = g_list_next (m))
        {
          Match *match = (Match *) m->data;

          if (   match->GetPtrData (this, "Poule::match_gooitem")
              && (MatchIsCancelled (match) == FALSE))
          {
            match->Save (xml_scheme);
          }
        }

        xml_scheme->EndElement ();
        xml_scheme->EndElement ();

        xml_scheme->Release ();
      }

      parcel->Set ("xml", (const gchar *) xml_buffer->content);

      xmlBufferFree (xml_buffer);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Poule::OnMessage (Net::Message *message)
  {
    if (message->Is ("SmartPoule::ScoreSheetCall"))
    {
      Net::Message *response = new Net::Message ("BellePoule::ScoreSheet");

      {
        xmlBuffer *xml_buffer = xmlBufferCreate ();
        guint      channel    = message->GetInteger ("channel");
        guint      bout       = message->GetInteger ("bout")-1;

        {
          XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

          _contest->SaveHeader (xml_scheme);
          SaveHeader (xml_scheme);

          {
            Match *match = (Match *) g_list_nth_data (_matches,
                                                      bout);

            if (match)
            {
              match->Save (xml_scheme);
            }
          }

          xml_scheme->EndElement ();
          xml_scheme->EndElement ();

          xml_scheme->Release ();
        }

        response->Set ("competition", _contest->GetNetID ());
        response->Set ("stage",       GetNetID ());
        response->Set ("bout",        bout);
        response->Set ("xml",         (const gchar *) xml_buffer->content);

        response->Set ("client",  message->GetInteger ("client"));
        response->Set ("channel", channel);

        xmlBufferFree (xml_buffer);
      }

      response->Spread ();
      response->Release ();

      return TRUE;
    }
    else if (message->Is ("SmartPoule::Score"))
    {
      guint  bout  = message->GetInteger ("bout");
      Match *match = (Match *) g_list_nth_data (_matches,
                                                bout);

      if (match)
      {
        gchar *xml_data = message->GetString ("xml");

        xmlDoc *doc = xmlReadMemory (xml_data,
                                     strlen (xml_data),
                                     "noname.xml",
                                     nullptr,
                                     0);

        if (doc)
        {
          xmlXPathInit ();

          {
            xmlXPathContext *xml_context = xmlXPathNewContext (doc);
            xmlXPathObject  *xml_object;
            xmlNodeSet      *xml_nodeset;

            xml_object = xmlXPathEval (BAD_CAST "/Match/*", xml_context);
            xml_nodeset = xml_object->nodesetval;

            if (xml_nodeset->nodeNr == 2)
            {
              Player::AttributeId status_attr_id ("status", GetDataOwner ());

              match->CleanScore ();

              for (guint i = 0; i < 2; i++)
              {
                Player *fencer = match->GetOpponent (i);
                Score  *score;

                match->Load (xml_nodeset->nodeTab[i],
                             fencer);
                score = match->GetScore (i);

                fencer->SetAttributeValue (&status_attr_id,
                                           score->GetStatusImage ());
              }

              OnNewScore (nullptr,
                          match,
                          nullptr);

              _point_system->Rehash ();
              RefreshClassification ();
              //RefreshScoreData ();
              //RefreshDashBoard ();
            }

            xmlXPathFreeObject  (xml_object);
            xmlXPathFreeContext (xml_context);
          }
          xmlFreeDoc (doc);
        }
        g_free (xml_data);

        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Poule::on_per_fencer_entry_changed (GtkEditable *editable,
                                           Object      *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->OnMatchesPerFencerChanged (editable);
  }

  // --------------------------------------------------------------------------------
  void Poule::on_duration_entry_changed (GtkEditable *editable,
                                         Object      *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->OnDurationChanged (editable);
  }

  // --------------------------------------------------------------------------------
  void Poule::OnMatchValidated (GtkButton *button,
                                Poule     *poule)
  {
    Match *match = (Match *) g_object_get_data (G_OBJECT (button), "Button::match");

    poule->_hall->FreePiste (match);
    match->SetPiste (0);

    for (GList *m = poule->_matches; m; m = g_list_next (m))
    {
      Match *current_match = (Match *) m->data;

      if (poule->MatchIsCancelled (current_match))
      {
        poule->_hall->FreePiste (current_match);
      }
    }

    poule->Display ();
    poule->MakeDirty ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_quest_filter_toolbutton_clicked (GtkWidget *widget,
                                                                      Object    *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->OnFilterClicked ("quest_classification_toggletoolbutton");
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_print_quest_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->Print (gettext (Poule::_class_name));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_piste_entry_changed (GtkEditable *editable,
                                                          Object      *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->OnPisteCountChanged (editable);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_quest_classification_toggletoolbutton_toggled (GtkToggleToolButton *widget,
                                                                                    Object              *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->ToggleClassification (gtk_toggle_tool_button_get_active (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_quest_stuff_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->OnStuffClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_highlight_entry_changed (GtkEditable *editable,
                                                              Object      *owner)
  {
    Poule *p = dynamic_cast <Poule *> (owner);

    p->OnHighlightChanged (editable);
  }
}
