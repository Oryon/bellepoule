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

#include <gdk/gdkkeysyms.h>
#include "util/data.hpp"
#include "util/glade.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "util/xml_scheme.hpp"
#include "util/global.hpp"
#include "network/message.hpp"
#include "network/ring.hpp"
#include "../../match.hpp"
#include "../../contest.hpp"
#include "../../score.hpp"

#include "hall.hpp"
#include "wheel_of_fortune.hpp"
#include "round.hpp"

namespace Swiss
{
  const gchar *Round::_class_name     = N_ ("Pool");
  const gchar *Round::_xml_class_name = "RondeSuisse";

  GdkPixbuf     *Round  ::_moved_pixbuf = nullptr;
  const gdouble  Round  ::_score_rect_w = 45.0;
  const gdouble  Round  ::_score_rect_h = 30.0;

  // --------------------------------------------------------------------------------
  Round::Round (StageClass *stage_class)
    : Object ("Swiss::Round"),
      Stage (stage_class),
      CanvasModule ("swiss_supervisor.glade")
  {
    _matches         = nullptr;
    _score_collector = nullptr;

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

    _hall = new Hall ();

    _piste_count = new Data ("NbPistes",
                             4);
    _hall->SetPisteCount (_piste_count->_value);

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
  }

  // --------------------------------------------------------------------------------
  Round::~Round ()
  {
    Reset ();

    _max_score->Release          ();
    _matches_per_fencer->Release ();
    _piste_count->Release        ();

    Object::TryToRelease (_score_collector);
  }

  // --------------------------------------------------------------------------------
  void Round::Declare ()
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
  void Round::Reset ()
  {
    _hall->Clear ();

    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      for (guint i = 0; i < 2; i++)
      {
        Player *fencer  = match->GetOpponent (i);
        GList  *matches = (GList *) fencer->GetPtrData (this, "Round::matches");

        g_list_free (matches);
        fencer->RemoveData (this, "Round::matches");
      }

      match->RemoveData (this, "Round::displayed");
    }

    FreeFullGList (Match,
                   _matches);
    _matches = nullptr;
  }

  // --------------------------------------------------------------------------------
  Stage *Round::CreateInstance (StageClass *stage_class)
  {
    return new Round (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Round::Load (xmlNode *xml_node)
  {
    LoadConfiguration (xml_node);
    Garnish ();
    LoadMatches (xml_node);
  }

  // --------------------------------------------------------------------------------
  void Round::LoadMatches (xmlNode *xml_node)
  {
    _hall->Clear ();

    for (xmlNode *n = xml_node; n != nullptr; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (g_strcmp0 ((char *) n->name, _xml_class_name) == 0)
        {
        }
        else if (g_strcmp0 ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          LoadAttendees (n);
        }
        else if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          gchar *number = nullptr;

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

                    if (match->IsOver () == FALSE)
                    {
                      _hall->BookPiste (match->GetPiste ());
                    }
                  }
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
  void Round::SaveConfiguration (XmlScheme *xml_scheme)
  {
    Stage::SaveConfiguration (xml_scheme);

    _matches_per_fencer->Save (xml_scheme);
    _piste_count->Save        (xml_scheme);
  }

  // --------------------------------------------------------------------------------
  void Round::LoadConfiguration (xmlNode *xml_node)
  {
    Stage::LoadConfiguration (xml_node);

    _matches_per_fencer->Load (xml_node);

    _piste_count->Load (xml_node);
    _hall->SetPisteCount (_piste_count->_value);
  }

  // --------------------------------------------------------------------------------
  void Round::SaveHeader (XmlScheme *xml_scheme)
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
  void Round::Save (XmlScheme *xml_scheme)
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
  void Round::FillInConfig ()
  {
    Stage::FillInConfig ();

    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("matches_per_fencer_entry"));

      if (w)
      {
        gchar *text = g_strdup_printf ("%d", _matches_per_fencer->_value);

        gtk_entry_set_text (w,
                            text);
        g_free (text);
      }
    }

    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("piste_entry"));

      if (w)
      {
        gchar *text = g_strdup_printf ("%d", _piste_count->_value);

        gtk_entry_set_text (w,
                            text);
        g_free (text);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Round::ApplyConfig ()
  {
    Stage::ApplyConfig ();

    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("matches_per_fencer_entry"));

      if (w)
      {
        gchar *str = (gchar *) gtk_entry_get_text (w);

        if (str)
        {
          _matches_per_fencer->_value = atoi (str);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Round::Garnish ()
  {
    GSList *fencers            = g_slist_copy (GetShortList ());
    guint   fencer_count       = g_slist_length (fencers);
    guint   matches_per_fencer = MIN (_matches_per_fencer->_value, fencer_count-1);

    for (guint i = 0; i < matches_per_fencer; i++)
    {
      TossMatches (fencers,
                   i+1);
      fencers = g_slist_reverse (fencers);
    }

    g_slist_free (fencers);
  }

  // --------------------------------------------------------------------------------
  void Round::TossMatches (GSList *fencers,
                           guint   matches_per_fencer)
  {
    WheelOfFortune *wheel = new WheelOfFortune (fencers);

    for (GSList *f = fencers; f; f = g_slist_next (f))
    {
      Player *fencer  = (Player *) f->data;
      GList  *matches = (GList *) fencer->GetPtrData (this, "Round::matches");

      for (guint match_count = g_list_length (matches);
                 match_count < matches_per_fencer;
                 match_count++)
      {

        for (void *o = wheel->Turn (); o; o = wheel->TryAgain ())
        {
          Player *opponent         = (Player *) o;
          GList  *opponent_matches = (GList *) opponent->GetPtrData (this, "Round::matches");

          if (   (opponent != fencer)
              && (FencerHasMatch (opponent, matches) == FALSE)
              && (g_list_length (opponent_matches) < matches_per_fencer))
          {
            Match *match = new Match (fencer,
                                      opponent,
                                      _max_score);

            matches = g_list_prepend (matches,
                                      match);

            opponent_matches = g_list_prepend (opponent_matches,
                                               match);
            opponent->SetData (this, "Round::matches",
                               opponent_matches);

            _matches = g_list_append (_matches,
                                      match);
            match->SetNumber (g_list_length (_matches));

            break;
          }
        }
      }
      fencer->SetData (this, "Round::matches",
                       matches);
    }

    wheel->Release ();
  }

  // --------------------------------------------------------------------------------
  void Round::Dump ()
  {
    GSList *fencers = GetShortList ();

    for (GSList *f = fencers; f; f = g_slist_next (f))
    {
      Player *fencer  = (Player *) f->data;
      GList  *matches = (GList *) fencer->GetPtrData (this, "Round::matches");

      printf ("%s (%d) ==>> ", fencer->GetName (), g_list_length (matches));

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
  void Round::Display ()
  {
    GooCanvasItem *root = GetRootItem ();

    if (root)
    {
      guint ongoings = 0;

      _table = goo_canvas_table_new (root,
                                     "column-spacing", 10.0,
                                     "row-spacing",    10.0,
                                     NULL);

      {
        Object::TryToRelease (_score_collector);

        _score_collector = new ScoreCollector (this,
                                               FALSE);

        _score_collector->SetConsistentColors ("LightGrey",
                                               "SkyBlue");
      }

      _rows = 0;
      for (GList *m = _matches; m; m = g_list_next (m))
      {
        Match *match = (Match *) m->data;

        if (match->IsOver () == FALSE)
        {
          ongoings++;

          if (ongoings > _piste_count->_value)
          {
            break;
          }
        }

        match->SetPiste (_hall->BookPiste ());
        DisplayMatch (match);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Round::DisplayMatch (Match *match)
  {
    GooCanvasItem *item;
    GooCanvasItem *collecting_point_A;
    GooCanvasItem *collecting_point_B;
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
    }

    // Fencers
    for (guint i = 0; i < 2; i++)
    {
      Player *fencer = match->GetOpponent (i);

      if (i%2 != 0)
      {
        collecting_point_A = DisplayScore (_table,
                                           _rows,
                                           column++,
                                           match,
                                           fencer);
      }

      item = GetPlayerImage (_table,
                             "font_desc=\"" BP_FONT "14.0px\"",
                             fencer,
                             nullptr,
                             "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                             "first_name", "foreground=\"darkblue\"",
                             "club",       "style=\"italic\" foreground=\"dimgrey\"",
                             "league",     "style=\"italic\" foreground=\"dimgrey\"",
                             "region",     "style=\"italic\" foreground=\"dimgrey\"",
                             "country",    "style=\"italic\" foreground=\"dimgrey\"",
                             NULL);

      Canvas::PutInTable (_table,
                          item,
                          _rows, column++);
      Canvas::SetTableItemAttribute (item, "y-align", 0.5);

      if (i%2 == 0)
      {
        collecting_point_B = DisplayScore (_table,
                                           _rows,
                                           column++,
                                           match,
                                           fencer);
      }
    }

    _score_collector->SetNextCollectingPoint (collecting_point_A,
                                              collecting_point_B);
    _score_collector->SetNextCollectingPoint (collecting_point_B,
                                              collecting_point_A);

    // Piste
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
    }

    _rows++;
    match->SetData (this, "Round::displayed",
                    (void *) TRUE);
  }

  // --------------------------------------------------------------------------------
  GooCanvasItem * Round::DisplayScore (GooCanvasItem *table,
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
      GooCanvasItem *item = Canvas::PutPixbufInTable (score_table,
                                                      nullptr,
                                                      0,
                                                      0);

      Canvas::SetTableItemAttribute (item, "x-align", 0.5);
      Canvas::SetTableItemAttribute (item, "y-align", 0.5);
    }

    return score_rect;
  }

  // --------------------------------------------------------------------------------
  gboolean Round::OnStatusArrowPress (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event,
                                      Round          *round)
  {
    if (   (event->button == 1)
        && (event->type   == GDK_BUTTON_PRESS))
    {
      GtkWidget *combo;
      Match     *match  = (Match *)  g_object_get_data (G_OBJECT (item), "Arrow::match");
      Player    *fencer = (Player *) g_object_get_data (G_OBJECT (item), "Arrow::fencer");

      {
        GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();

        combo = gtk_combo_box_new_with_model (round->GetStatusModel ());

        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
                                        cell, "pixbuf", AttributeDesc::DiscreteColumnId::ICON_pix,
                                        NULL);

        goo_canvas_widget_new (round->GetRootItem (),
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

        iter_is_valid = gtk_tree_model_get_iter_first (round->GetStatusModel (),
                                                       &iter);
        while (iter_is_valid)
        {
          gtk_tree_model_get (round->GetStatusModel (),
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
          iter_is_valid = gtk_tree_model_iter_next (round->GetStatusModel (),
                                                    &iter);
        }
      }

      {
        g_object_set_data (G_OBJECT (combo), "player_for_status", (void *) fencer);
        g_object_set_data (G_OBJECT (combo), "match_for_status",  (void *) match);
        g_signal_connect (combo, "changed",
                          G_CALLBACK (OnStatusChanged), round);
        g_signal_connect (combo, "key-press-event",
                          G_CALLBACK (OnStatusKeyPressEvent), round);
      }
    }

    return FALSE;
  }


  // --------------------------------------------------------------------------------
  void Round::OnStatusChanged (GtkComboBox *combo_box,
                               Round       *round)
  {
    {
      GtkTreeIter  iter;
      gchar       *code;
      Match       *match  = (Match *) g_object_get_data (G_OBJECT (combo_box), "match_for_status");
      Player      *player = (Player *) g_object_get_data (G_OBJECT (combo_box), "player_for_status");

      gtk_combo_box_get_active_iter (combo_box,
                                     &iter);
      gtk_tree_model_get (round->GetStatusModel (),
                          &iter,
                          AttributeDesc::DiscreteColumnId::XML_IMAGE_str, &code,
                          -1);

      if (code && *code !='Q')
      {
        match->DropFencer (player,
                           code);
      }
      else
      {
        match->RestoreFencer (player);
      }

      {
        Player::AttributeId status_attr_id ("status", round->GetDataOwner ());

        player->SetAttributeValue (&status_attr_id,
                                   code);
      }

      g_free (code);

      round->OnNewScore (nullptr,
                         match,
                         player);
    }

    gtk_widget_destroy (GTK_WIDGET (combo_box));
  }

  // --------------------------------------------------------------------------------
  gboolean Round::OnStatusKeyPressEvent (GtkWidget   *widget,
                                         GdkEventKey *event,
                                         Round       *round)
  {
    if (event->keyval == GDK_KEY_Escape)
    {
      gtk_widget_destroy (GTK_WIDGET (widget));
      return TRUE;
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Round::FencerHasMatch (Player *fencer,
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
  void Round::OnAttrListUpdated ()
  {
    Wipe ();

    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      _score_collector->RemoveCollectingPoints (match);
    }

    Display ();
  }

  // --------------------------------------------------------------------------------
  void Round::OnNewScore (ScoreCollector *score_collector,
                          Match          *match,
                          Player         *player)
  {
    match->Timestamp ();

    _score_collector->Refresh (match);

    if (match->IsOver ())
    {
      _hall->Free (match->GetPiste ());

      for (GList *m = _matches; m; m = g_list_next (m))
      {
        Match *current_match = (Match *) m->data;

        if (current_match->IsOver () == FALSE)
        {
          if (current_match->GetPtrData (this, "Round::displayed") == FALSE)
          {
            guint piste = _hall->BookPiste ();

            if (piste)
            {
              current_match->SetPiste (piste);
              DisplayMatch (current_match);
            }
            break;
          }
        }
      }
    }

    MakeDirty ();
    //OnRoundOver
  }

  // --------------------------------------------------------------------------------
  void Round::OnPisteCountChanged (GtkEditable *editable)
  {
    gchar *str = (gchar *) gtk_entry_get_text (GTK_ENTRY (editable));

    if (str)
    {
      _piste_count->_value = atoi (str);
      _hall->SetPisteCount (_piste_count->_value);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Round::OnMessage (Net::Message *message)
  {
    if (message->Is ("SmartPoule::JobListCall"))
    {
      {
        Net::Message *response = new Net::Message ("BellePoule::JobList");

        {
          xmlBuffer *xml_buffer = xmlBufferCreate ();

          {
            XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

            _contest->SaveHeader (xml_scheme);
            SaveHeader (xml_scheme);

            for (GList *m = _matches; m; m = g_list_next (m))
            {
              Match *match = (Match *) m->data;

              if (match->GetPtrData (this, "Round::displayed"))
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
          response->Set ("batch",       1);
          response->Set ("xml",         (const gchar *) xml_buffer->content);

          xmlBufferFree (xml_buffer);
        }

        Net::Ring::_broker->SendBackResponse (message,
                                              response);

        response->Release ();

        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_swiss_filter_toolbutton_clicked (GtkWidget *widget,
                                                                      Object    *owner)
  {
    Round *r = dynamic_cast <Round *> (owner);

    r->SelectAttributes ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_print_swiss_toolbutton_clicked (GtkWidget *widget,
                                                                     Object    *owner)
  {
    Round *r = dynamic_cast <Round *> (owner);

    r->Print (gettext (Round::_class_name));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_piste_entry_changed (GtkEditable *editable,
                                                          Object      *owner)
  {
    Round *r = dynamic_cast <Round *> (owner);

    r->OnPisteCountChanged (editable);
  }
}