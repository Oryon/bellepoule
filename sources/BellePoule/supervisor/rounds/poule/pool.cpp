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
#include <libxml/xpath.h>
#include <gdk/gdkkeysyms.h>
#include <goocanvas.h>
#include <cairo.h>
#include <cairo-pdf.h>

#include "util/global.hpp"
#include "util/fie_time.hpp"
#include "util/attribute_desc.hpp"
#include "util/attribute.hpp"
#include "util/flash_code.hpp"
#include "util/data.hpp"
#include "util/player.hpp"
#include "util/xml_scheme.hpp"
#include "network/message.hpp"
#include "dispatcher/dispatcher.hpp"
#include "../../score.hpp"
#include "../../match.hpp"

#include "pool.hpp"

namespace Pool
{
  gboolean Pool::_match_id_watermarked = FALSE;

  // --------------------------------------------------------------------------------
  Pool::Pool (Data        *max_score,
              guint        number,
              const gchar *xml_player_tag,
              guint32      rand_seed,
              guint        stage_id,
              Object      *rank_owner,
              ...)
    : Object ("Pool"),
      CanvasModule ("pool.glade", "canvas_scrolled_window")
  {
      _number                = number;
      _fencer_list           = nullptr;
      _referee_list          = nullptr;
      _strip                 = 0;
      _start_time            = nullptr;
      _duration_sec          = 0;
      _sorted_fencer_list    = nullptr;
      _match_list            = nullptr;
      _is_over               = FALSE;
      _has_error             = FALSE;
      _title_table           = nullptr;
      _status_item           = nullptr;
      _status_pixbuf         = nullptr;
      _locked                = FALSE;
      _max_score             = max_score;
      _display_data          = nullptr;
      _nb_drop               = 0;
      _rand_seed             = rand_seed;
      _xml_player_tag        = xml_player_tag;
      _strength              = 0;
      _strength_contributors = 0;
      _rank_owner            = rank_owner;

      _status_listener       = nullptr;

      _score_collector       = nullptr;

    {
      gchar *text = g_strdup_printf (gettext ("Pool #%02d"), _number);

      _name = GetUndivadableText (text);
      g_free (text);
    }

    _dispatcher = new Dispatcher (_name);

    Disclose ("BellePoule::Job");

    {
      va_list ap;

      va_start (ap, rank_owner);
      while (gchar *key = va_arg (ap, gchar *))
      {
        guint value = va_arg (ap, guint);

        _parcel->Set (key, value);

        if (g_strcmp0 (key, "competition") == 0)
        {
          gchar *ref = g_strdup_printf ("#%x/%x/%d",
                                        value,
                                        stage_id,
                                        _number);

          SetFlashRef (ref);
          g_free (ref);
        }
      }
      va_end (ap);
    }
  }

  // --------------------------------------------------------------------------------
  Pool::~Pool ()
  {
    Wipe ();

    DeleteMatchs ();

    g_free (_name);
    Object::TryToRelease (_start_time);

    RemoveAllReferee ();

    g_slist_free (_fencer_list);
    g_slist_free (_sorted_fencer_list);

    Object::TryToRelease (_score_collector);

    _dispatcher->Release ();
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetPiste ()
  {
    return _strip;
  }

  // --------------------------------------------------------------------------------
  FieTime *Pool::GetStartTime ()
  {
    return _start_time;
  }

  // --------------------------------------------------------------------------------
  void Pool::Wipe ()
  {
    if (_score_collector)
    {
      for (GList *current = _match_list; current; current = g_list_next (current))
      {
        Match *match = (Match *) current->data;

        _score_collector->RemoveCollectingPoints (match);
      }

      _score_collector->Release ();
      _score_collector = nullptr;
    }

    _title_table   = nullptr;
    _status_item   = nullptr;

    if (_status_pixbuf)
    {
      g_object_unref (_status_pixbuf);
      _status_pixbuf = nullptr;
    }

    for (guint i = 0; i < GetNbPlayers (); i++)
    {
      Player *player = (Player *) g_slist_nth_data (_fencer_list, i);

      for (GSList *current = _display_data; current; current = g_slist_next (current))
      {
        player->RemoveData (GetDataOwner (),
                            (gchar *) current->data);
      }
    }

    g_slist_free (_display_data);
    _display_data = nullptr;

    CanvasModule::Wipe ();
  }

  // --------------------------------------------------------------------------------
  void Pool::Stuff ()
  {
    for (GList *current = _match_list; current; current = g_list_next (current))
    {
      Match  *match;
      Player *A;
      Player *B;
      gint    score;

      match = (Match *) current->data;
      A     = match->GetOpponent (0);
      B     = match->GetOpponent (1);
      score = g_random_int_range (0,
                                  _max_score->_value);

      if (g_random_boolean ())
      {
        match->SetScore (A, _max_score->_value, TRUE);
        match->SetScore (B, score, FALSE);
      }
      else
      {
        match->SetScore (A, score, FALSE);
        match->SetScore (B, _max_score->_value, TRUE);
      }
    }

    RefreshScoreData ();
    Timestamp ();
  }

  // --------------------------------------------------------------------------------
  void Pool::SetWaterMarkingPolicy (gboolean enabled)
  {
    _match_id_watermarked = enabled;
  }

  // --------------------------------------------------------------------------------
  gboolean Pool::WaterMarkingEnabled ()
  {
    return _match_id_watermarked;
  }

  // --------------------------------------------------------------------------------
  void Pool::RegisterStatusListener (StatusListener *listener)
  {
    _status_listener = listener;

    RefreshStatus ();
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetNumber ()
  {
    return _number;
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetStrength ()
  {
    return _strength;
  }

  // --------------------------------------------------------------------------------
  void Pool::SetStrengthContributors (gint contributors_count)
  {
    _strength_contributors = contributors_count;
  }

  // --------------------------------------------------------------------------------
  gchar *Pool::GetName ()
  {
    return _name;
  }

  // --------------------------------------------------------------------------------
  void Pool::SetDataOwner (Object *current_round_owner,
                           Object *combined_rounds_owner,
                           Object *previous_combined_round)
  {
    Module::SetDataOwner (current_round_owner);

    _combined_rounds_owner   = combined_rounds_owner;
    _previous_combined_round = previous_combined_round; // To cumulate the current pool round's results with the previous

    _nb_drop = 0;

    {
      Player::AttributeId  current_round_attr_id   ("pool_nr", GetDataOwner ());
      Player::AttributeId  combined_rounds_attr_id ("pool_nr", _combined_rounds_owner);

      for (GSList *current = _fencer_list; current; current = g_slist_next (current))
      {
        Player *player = (Player *) current->data;

        player->SetAttributeValue (&current_round_attr_id,
                                   _number);
        player->SetAttributeValue (&combined_rounds_attr_id,
                                   _number);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::AddReferee (Player *referee)
  {
    if (referee == nullptr)
    {
      return;
    }

    if (   (_referee_list == nullptr)
        || (g_slist_find (_referee_list,
                          referee) == nullptr))
    {
      _referee_list = g_slist_prepend (_referee_list,
                                       referee);

      referee->SetChangeCbk ("connection",
                             (Player::OnChange) OnAttrConnectionChanged,
                             this);
      RefreshParcel ();
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Pool::FencerIsAbsent (Player *fencer)
  {
    Player::AttributeId  incidend_id  ("incident");
    Attribute           *incident  = fencer->GetAttribute (&incidend_id);

    return (incident && incident->GetStrValue ()[0] == 'A');
    }

  // --------------------------------------------------------------------------------
  void Pool::AddFencer (Player *player,
                        gint    position)
  {
    if (player == nullptr)
    {
      return;
    }

    if (   (_fencer_list == nullptr)
        || (g_slist_find (_fencer_list,
                          player) == nullptr))
    {
      {
        Player::AttributeId attr_id ("pool_nr", _rank_owner);

        player->SetAttributeValue (&attr_id,
                                   GetNumber ());
      }

      SetRoadmapTo (player,
                    _strip,
                    _start_time);

      {
        Player::AttributeId attr_id ("stage_start_rank", _rank_owner);

        attr_id.MakeRandomReady (_rand_seed);
        _fencer_list = g_slist_insert_sorted_with_data (_fencer_list,
                                                        player,
                                                        (GCompareDataFunc) Player::Compare,
                                                        &attr_id);

        if (FencerIsAbsent (player) == FALSE)
        {
          // Assumption is done that fencers are loaded sorted.
          if (position >= 0)
          {
            _sorted_fencer_list = g_slist_append (_sorted_fencer_list,
                                                  player);
          }
          else
          {
            SortPlayers ();
          }
        }
      }

      RefreshStrength ();
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RemoveFencer (Player *player)
  {
    if (g_slist_find (_fencer_list,
                      player))
    {
      SetRoadmapTo (player,
                    0,
                    nullptr);

      _fencer_list = g_slist_remove (_fencer_list,
                                     player);
      SortPlayers ();

      RefreshStrength ();
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::DismissFencer (Player *player)
  {
    if (g_slist_find (_sorted_fencer_list,
                      player))
    {
      SetRoadmapTo (player,
                    0,
                    nullptr);

      _sorted_fencer_list = g_slist_remove (_sorted_fencer_list,
                                            player);

      RefreshStrength ();
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::AllowFencer (Player *player)
  {
    if (g_slist_find (_sorted_fencer_list,
                      player) == nullptr)
    {
      _sorted_fencer_list = g_slist_insert_sorted_with_data (_sorted_fencer_list,
                                                             player,
                                                             (GCompareDataFunc) _ComparePlayerWithFullRandom,
                                                             (void *) this);

      SetRoadmapTo (player,
                    _strip,
                    _start_time);

      RefreshStrength ();
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RefreshStrength ()
  {
#ifdef DEBUG
    GSList *current = _fencer_list;

    _strength = 0;
    for (guint i = 0 ; current && (i < _strength_contributors); i++)
    {
      Player              *player = (Player *) current->data;
      Player::AttributeId  attr_id ("stage_start_rank", _rank_owner);
      Attribute           *stage_start_rank = player->GetAttribute (&attr_id);

      if (stage_start_rank)
      {
        _strength += stage_start_rank->GetUIntValue ();
      }

      current = g_slist_next (current);
    }
#endif
  }

  // --------------------------------------------------------------------------------
  void Pool::RemoveReferee (Player *referee)
  {
    if (_referee_list)
    {
      _referee_list = g_slist_remove (_referee_list,
                                      referee);
      referee->RemoveCbkOwner (this);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RemoveAllReferee ()
  {
    while (_referee_list)
    {
      RemoveReferee ((Player *) _referee_list->data);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::CreateMatchs (GSList *affinity_criteria_list)
  {
    if (_match_list == nullptr)
    {
      AttributeDesc *affinity_criteria = nullptr;

      if (affinity_criteria_list)
      {
        affinity_criteria = (AttributeDesc *) affinity_criteria_list->data;
      }

      _dispatcher->SetAffinityCriteria (affinity_criteria,
                                        _sorted_fencer_list);
      _dispatcher->Dump ();

      {
        guint nb_players = GetNbPlayers ();
        guint nb_matchs  = (nb_players*nb_players - nb_players) / 2;

        for (guint i = 0; i < nb_matchs; i++)
        {
          guint    a_id;
          guint    b_id;
          gboolean rest_error;

          if (_dispatcher->GetPlayerPair (i,
                                          &a_id,
                                          &b_id,
                                          &rest_error))
          {
            Player *a     = (Player *) g_slist_nth_data (_sorted_fencer_list, a_id-1);
            Player *b     = (Player *) g_slist_nth_data (_sorted_fencer_list, b_id-1);
            Match  *match = new Match (a,
                                       b,
                                       _max_score);
            match->SetNameSpace ("M");
            match->SetNumber (i+1);
            match->SetData (this, "rest_error", (void *) rest_error);

            _match_list = g_list_append (_match_list,
                                         match);
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::CopyPlayersStatus (Object *from)
  {
    for (GSList *current = _sorted_fencer_list; current; current = g_slist_next (current))
    {
      Player::AttributeId  attr_id ("status", from);
      Player              *player      = (Player *) current->data;
      Attribute           *status_attr = player->GetAttribute (&attr_id);
      gchar               *status      = status_attr->GetStrValue ();

      RestorePlayer (player);

      if (   (status[0] == 'A')
          || (status[0] == 'F')
          || (status[0] == 'E'))
      {
        DropPlayer (player,
                    status);
      }
    }

    RefreshScoreData ();
    RefreshDashBoard ();
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetNbPlayers ()
  {
    return g_slist_length (_sorted_fencer_list);
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetNbMatchs (Player *of)
  {
    guint nb_matchs = GetNbPlayers () - 1;

    for (GSList *current = _fencer_list; current; current = g_slist_next (current))
    {
      Player::AttributeId attr_id ("status", GetDataOwner ());
      Player              *player      = (Player *) current->data;
      Attribute           *status_attr = player->GetAttribute (&attr_id);

      if (status_attr)
      {
        gchar *status = status_attr->GetStrValue ();

        if (   (status[0] == 'A')
            || (status[0] == 'F')
            || (status[0] == 'E')
            || FencerIsAbsent (player))
        {
          nb_matchs--;

          if (player == of)
          {
            return 0;
          }
        }
      }
    }

    return nb_matchs;
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetNbMatchs ()
  {
    guint nb_players = g_slist_length (_sorted_fencer_list);

    return ((nb_players*nb_players - nb_players) / 2);
  }

  // --------------------------------------------------------------------------------
  GSList *Pool::GetFencerList ()
  {
    return _fencer_list;
  }

  // --------------------------------------------------------------------------------
  GSList *Pool::GetSortedFencerList ()
  {
    return _sorted_fencer_list;
  }

  // --------------------------------------------------------------------------------
  GSList *Pool::GetRefereeList ()
  {
    return _referee_list;
  }

  // --------------------------------------------------------------------------------
  Player *Pool::GetPlayer (guint   i,
                           GSList *in_list)
  {
    return (Player *) g_slist_nth_data (in_list,
                                        i);
  }

  // --------------------------------------------------------------------------------
  void Pool::OnAttrConnectionChanged (Player    *player,
                                      Attribute *attr,
                                      Object    *owner,
                                      guint      step)
  {
    Pool *pool = dynamic_cast <Pool *> (owner);

    if (pool->_locked == FALSE)
    {
      pool->RefreshStatus ();
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::Timestamp ()
  {
    if (_start_time)
    {
      _duration_sec = Match::GetDuration (_start_time->GetGDateTime ());
      Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::OnNewScore (ScoreCollector *score_collector,
                         Match          *match,
                         Player         *player)
  {
    RefreshScoreData ();
    RefreshDashBoard ();

    if (_is_over)
    {
      Timestamp ();
    }
  }

  // --------------------------------------------------------------------------------
  GooCanvasItem *Pool::Draw (GooCanvas *on_canvas,
                             gboolean   print_for_referees,
                             gboolean   print_matchs)
  {
    GooCanvasItem *canvas_root = goo_canvas_get_root_item (on_canvas);
    GooCanvasItem *root_item   = goo_canvas_group_new (canvas_root, NULL);

    _score_collector = new ScoreCollector (this);

    {
      const guint    cell_w      = 45;
      const guint    cell_h      = 45;
      guint          nb_players  = GetNbPlayers ();
      GooCanvasItem *title_group = goo_canvas_group_new (root_item, NULL);
      GooCanvasItem *grid_group  = goo_canvas_group_new (root_item, NULL);
      GooCanvasItem *grid_header = goo_canvas_group_new (grid_group, NULL);
      GooCanvasItem *grid;

      // Name
      {
        GooCanvasItem *text_item;
        GdkPixbuf     *pixbuf = _flash_code->GetPixbuf (2);

        _title_table = goo_canvas_table_new (title_group, NULL);
        text_item = Canvas::PutTextInTable (_title_table,
                                            _name,
                                            0,
                                            1);
        g_object_set (G_OBJECT (text_item),
                      "font", BP_FONT "bold 18px",
                      NULL);

        Canvas::PutPixbufInTable (_title_table,
                                  pixbuf,
                                  1,
                                  1);
        g_object_unref (pixbuf);
      }

      // Referee / Strip
      {
        GooCanvasItem *referee_group = goo_canvas_group_new (title_group, NULL);
        GooCanvasItem *piste_group   = goo_canvas_group_new (title_group, NULL);

        goo_canvas_rect_new (referee_group,
                             0.0,
                             0.0,
                             cell_w*5,
                             cell_h,
                             "stroke-color", "Grey",
                             "line-width", 2.0,
                             NULL);
        goo_canvas_text_new (referee_group,
                             gettext ("Referee"),
                             0.0,
                             0.0,
                             -1,
                             GTK_ANCHOR_SW,
                             "fill-color", "Grey",
                             "font", BP_FONT "bold 30.0px",
                             NULL);
        if (_referee_list)
        {
          Player *referee = (Player *) _referee_list->data;

          {
            gchar *name = referee->GetName ();

            goo_canvas_text_new (referee_group,
                                 name,
                                 5.0,
                                 cell_h/3.0,
                                 -1,
                                 GTK_ANCHOR_W,
                                 "fill-color", "DarkGreen",
                                 "font", BP_FONT "bold 25.0px",
                                 NULL);
            g_free (name);
          }

          {
            Player::AttributeId  first_name_attr_id ("first_name");
            Attribute *attribute = referee->GetAttribute (&first_name_attr_id);

            if (attribute)
            {
              gchar *first_name = attribute->GetUserImage (AttributeDesc::LONG_TEXT);

              goo_canvas_text_new (referee_group,
                                   first_name,
                                   20.0,
                                   2.4*cell_h/3.0,
                                   -1,
                                   GTK_ANCHOR_W,
                                   "fill-color", "DarkGreen",
                                   "font", BP_FONT "bold 15.0px",
                                   NULL);
              g_free (first_name);
            }
          }
        }

        Canvas::HAlign (referee_group,
                        Canvas::Alignment::START,
                        _title_table,
                        Canvas::Alignment::START);
        Canvas::Anchor (referee_group,
                        nullptr,
                        _title_table,
                        cell_w*5);

        goo_canvas_rect_new (piste_group,
                             0.0,
                             0.0,
                             cell_w*5,
                             cell_h,
                             "stroke-color", "Grey",
                             "line-width", 2.0,
                             NULL);
        goo_canvas_text_new (piste_group,
                             gettext ("Piste"),
                             0.0,
                             0.0,
                             -1,
                             GTK_ANCHOR_SW,
                             "fill-color", "Grey",
                             "font", BP_FONT "bold 30.0px",
                             NULL);

        if (_strip)
        {
          gchar *piste = g_strdup_printf ("%02d%c%c@%c%c%s",
                                          _strip,
                                          0xC2, 0xA0, // non breaking space
                                          0xC2, 0xA0, // non breaking space
                                          _start_time->GetImage ());

          goo_canvas_text_new (piste_group,
                               piste,
                               5.0,
                               cell_h/2.0,
                               -1,
                               GTK_ANCHOR_W,
                               "fill-color", "DarkGreen",
                               "font", BP_FONT "bold 25.0px",
                               NULL);
          g_free (piste);
        }

        Canvas::HAlign (piste_group,
                        Canvas::Alignment::START,
                        referee_group,
                        Canvas::Alignment::START);
        Canvas::Anchor (piste_group,
                        nullptr,
                        referee_group,
                        cell_w/2);
      }

      // Grid
      {
        // Border
        grid = goo_canvas_rect_new (grid_group,
                                    0, 0,
                                    nb_players * cell_w, nb_players * cell_h,
                                    "line-width", 5.0,
                                    NULL);

        // Cells
        {
          GooCanvasItem *previous_goo_rect = nullptr;

          for (guint i = 0; i < nb_players; i++)
          {
            Player *A = GetPlayer (i, _sorted_fencer_list);

            for (guint j = 0; j < nb_players; j++)
            {
              GooCanvasItem *goo_rect;
              GooCanvasItem *score_text;
              gint           x, y;

              x = j * cell_w;
              y = i * cell_h;

              goo_rect = goo_canvas_rect_new (grid_group,
                                              x, y,
                                              cell_w, cell_h,
                                              "line-width", 2.0,
                                              "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                              NULL);

              if (i != j)
              {
                Player *B     = GetPlayer (j, _sorted_fencer_list);
                Match  *match = GetMatch (A, B);

                if (match)
                {
                  if (match->IsDropped ())
                  {
                    g_object_set (goo_rect, "fill-color", "grey", NULL);
                  }

                  // Text
                  {
                    gchar *score_image;

                    if (print_for_referees)
                    {
                      score_image = g_strdup (match->GetName ());
                    }
                    else
                    {
                      Score *score = match->GetScore (A);

                      score_image = score->GetImage ();
                    }
                    score_text = goo_canvas_text_new (grid_group,
                                                      score_image,
                                                      x + cell_w / 2,
                                                      y + cell_h / 2,
                                                      -1,
                                                      GTK_ANCHOR_CENTER,
                                                      "font", BP_FONT "bold 18px",
                                                      NULL);
                    if (print_for_referees)
                    {
                      if (_match_id_watermarked)
                      {
                        g_object_set (score_text,
                                      "fill-color", "Grey",
                                      NULL);
                      }
                      else
                      {
                        g_object_set (score_text,
                                      "fill-color", "White",
                                      NULL);
                      }
                    }
                    g_free (score_image);
                  }

                  if (   (print_for_referees == FALSE)
                      && (_locked == FALSE))
                  {
                    _score_collector->AddCollectingPoint (goo_rect,
                                                          score_text,
                                                          match,
                                                          A);

                    if (previous_goo_rect)
                    {
                      _score_collector->SetNextCollectingPoint (previous_goo_rect,
                                                                goo_rect);
                    }
                  }

                  previous_goo_rect = goo_rect;
                }
                else
                {
                  g_object_set (goo_rect, "fill-color", "grey", NULL);
                }
              }
              else
              {
                g_object_set (goo_rect, "fill-color", "grey", NULL);
              }
            }
          }
        }

        // Players (vertically)
        for (guint i = 0; i < nb_players; i++)
        {
          gint           y;
          GooCanvasItem *image;
          GooCanvasItem *player_table = goo_canvas_table_new (grid_group, NULL);

          y = cell_h / 2 + i * cell_h;
          image = GetPlayerImage (player_table,
                                  "font_desc=\"" BP_FONT "14.0px\"",
                                  GetPlayer (i, _sorted_fencer_list),
                                  nullptr,
                                  "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                                  "first_name", "foreground=\"darkblue\"",
                                  "club",       "style=\"italic\" foreground=\"dimgrey\"",
                                  "league",     "style=\"italic\" foreground=\"dimgrey\"",
                                  "region",     "style=\"italic\" foreground=\"dimgrey\"",
                                  "country",    "style=\"italic\" foreground=\"dimgrey\"",
                                  NULL);
          Canvas::PutInTable (player_table,
                              image,
                              0,
                              0);

          {
            gchar         *index = g_strdup_printf ("%d\302\240\302\240", i+1);
            GooCanvasItem *text_item;

            text_item = Canvas::PutTextInTable (player_table,
                                                index,
                                                0,
                                                1);
            g_object_set (G_OBJECT (text_item),
                          "wrap",      PANGO_WRAP_CHAR,
                          NULL);
            g_free (index);
          }

          goo_canvas_item_translate (player_table,
                                     0,
                                     y);
          Canvas::VAlign (player_table,
                          Canvas::Alignment::END,
                          grid,
                          Canvas::Alignment::START);
        }

        // Players (horizontally)
        for (guint i = 0; i < nb_players; i++)
        {
          gint   x, y;
          gchar *text;

          x = cell_w / 2 + i * cell_w;;
          y = - 13;
          text = g_strdup_printf ("%d", i+1);

          goo_canvas_text_new (grid_header,
                               text,
                               x, y, -1,
                               GTK_ANCHOR_WEST,
                               "font", BP_FONT "18px",
                               NULL);
          g_free (text);
        }
      }

      // Dashboard
      {
        GooCanvasItem *dashboard_group  = goo_canvas_group_new (root_item, NULL);
        GooCanvasItem *dashboard_body   = goo_canvas_group_new (dashboard_group, NULL);

        if (print_for_referees)
        {
          for (guint i = 0; i < nb_players; i++)
          {
            goo_canvas_rect_new (dashboard_body,
                                 0.0,
                                 i*cell_h,
                                 cell_w*5,
                                 cell_h,
                                 "fill-color",   "Grey85",
                                 "stroke-color", "White",
                                 "line-width", 2.0,
                                 NULL);
            goo_canvas_text_new (dashboard_body,
                                 gettext ("Signature"),
                                 0.0,
                                 i*cell_h,
                                 -1,
                                 GTK_ANCHOR_NW,
                                 "fill-color",   "Grey95",
                                 "font", BP_FONT "bold 30.0px",
                                 NULL);
          }
        }
        else
        {
          GooCanvasItem *dashboard_header = goo_canvas_group_new (dashboard_group, NULL);
          GooCanvasItem *goo_text;
          gint           x, y;

          x = cell_w/2;
          y = -10;

          if (GetCanvas () == on_canvas)
          {
            goo_text = goo_canvas_text_new (dashboard_header,
                                            gettext ("Status"),
                                            0.0, y, -1,
                                            GTK_ANCHOR_WEST,
                                            "font", BP_FONT "18px",
                                            NULL);
            goo_canvas_item_rotate (goo_text, 315, 0.0, y);
            x += cell_w;
          }

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Victories"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", BP_FONT "18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Vict./Matchs"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", BP_FONT "18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("H. scored"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", BP_FONT "18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("H. received"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", BP_FONT "18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Index"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", BP_FONT "18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Place"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", BP_FONT "18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          for (guint i = 0; i < nb_players; i++)
          {
            GooCanvasItem *goo_item;
            Player        *player   = GetPlayer (i, _sorted_fencer_list);
            gint           x_player = cell_w/2;
            gint           y_player = cell_h/2 + i *cell_h;

            if (GetCanvas () == on_canvas)
            {
              GtkWidget       *w    = gtk_combo_box_new_with_model (GetStatusModel ());
              GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();

              gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (w), cell, FALSE);
              gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (w),
                                              cell, "pixbuf", AttributeDesc::DiscreteColumnId::ICON_pix,
                                              NULL);

              g_object_set_data (G_OBJECT (w), "player",  player);
              player->SetData (this, "combo_box", w);
              goo_item = goo_canvas_widget_new (dashboard_body,
                                                w,
                                                x_player-cell_w/2,
                                                y_player,
                                                cell_w*3/2,
                                                cell_h*2/3,
                                                "anchor", GTK_ANCHOR_CENTER,
                                                NULL);
              SetDisplayData (player, on_canvas, "StatusItem", w);
              x_player += cell_w*3/2;
              gtk_widget_show (w);
            }

            goo_item = goo_canvas_text_new (dashboard_body,
                                            ".",
                                            x_player, y_player, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", BP_FONT "18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "VictoriesItem", goo_item);
            x_player += cell_w;

            goo_item = goo_canvas_text_new (dashboard_body,
                                            ".",
                                            x_player, y_player, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", BP_FONT "18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "VictoriesRatioItem", goo_item);
            x_player += cell_w;

            goo_item = goo_canvas_text_new (dashboard_body,
                                            ".",
                                            x_player, y_player, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", BP_FONT "18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "HSItem", goo_item);
            x_player += cell_w;

            goo_item = goo_canvas_text_new (dashboard_body,
                                            ".",
                                            x_player, y_player, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", BP_FONT "18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "HRItem", goo_item);
            x_player += cell_w;

            goo_item = goo_canvas_text_new (dashboard_body,
                                            ".",
                                            x_player, y_player, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", BP_FONT "18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "IndiceItem", goo_item);
            x_player += cell_w;

            goo_item = goo_canvas_text_new (dashboard_body,
                                            ".",
                                            x_player, y_player, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", BP_FONT "18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "RankItem", goo_item);
            x_player += cell_w;
          }
        }

        Canvas::VAlign (grid_group,
                        Canvas::Alignment::START,
                        title_group,
                        Canvas::Alignment::START,
                        cell_h/2);

        Canvas::Anchor (dashboard_group,
                        title_group,
                        nullptr,
                        cell_h/2);

        Canvas::Anchor (dashboard_group,
                        nullptr,
                        grid_group,
                        cell_w/2);

        if (print_for_referees)
        {
          Canvas::HAlign (grid_group,
                          Canvas::Alignment::START,
                          dashboard_body,
                          Canvas::Alignment::START,
                          -((gdouble) cell_h)/2.0);
        }
        else
        {
          GooCanvasBounds grid_header_bounds;

          goo_canvas_item_get_bounds (grid_header,
                                      &grid_header_bounds);
          Canvas::HAlign (grid_group,
                          Canvas::Alignment::START,
                          dashboard_body,
                          Canvas::Alignment::START,
                          -((gdouble) cell_h)/4.0 - (grid_header_bounds.y2 - grid_header_bounds.y1));
        }
      }

      // Matchs
      if (print_matchs)
      {
        GooCanvasItem *match_main_table;
        GooCanvasItem *text_item;
        guint          nb_column = 1;

        match_main_table = goo_canvas_table_new (root_item,
                                                 "row-spacing",    (gdouble) cell_h/2.0,
                                                 "column-spacing", (gdouble) cell_w/4.0,
                                                 NULL);

        for (guint m = 0; m < g_list_length (_match_list); m++)
        {
          Match         *match;
          GooCanvasItem *match_table;

          match_table = goo_canvas_table_new (match_main_table,
                                              "column-spacing", 1.5,
                                              NULL);

          match = GetMatch (m);

          {
            GooCanvasItem *name_table = goo_canvas_table_new (match_main_table,
                                                              "column-spacing", 1.0,
                                                              NULL);

            text_item = Canvas::PutTextInTable (name_table,
                                                match->GetName (),
                                                0,
                                                0);
            g_object_set (G_OBJECT (text_item),
                          "font", BP_FONT "bold 18px",
                          NULL);

            if (match->GetUIntData (this, "rest_error"))
            {
              gchar *png = g_build_filename (Global::_share_dir, "resources/glade/images/clock.png", NULL);

              GooCanvasItem *icon = Canvas::PutIconInTable (name_table,
                                                            png,
                                                            0,
                                                            1);
              Canvas::SetTableItemAttribute (icon, "y-align", 0.5);

              g_free (png);
            }

            Canvas::SetTableItemAttribute (text_item, "y-align", 0.5);

            Canvas::PutInTable (match_main_table,
                                name_table,
                                m/nb_column,
                                m%nb_column + 2*(m%nb_column));
            Canvas::SetTableItemAttribute (name_table, "y-align", 0.5);
          }

          {
            Player *player              = match->GetOpponent (0);
            GooCanvasItem *player_table = goo_canvas_table_new (match_table, NULL);
            GooCanvasItem *image = GetPlayerImage (player_table,
                                                   "font_desc=\"" BP_FONT "14.0px\"",
                                                   player,
                                                   nullptr,
                                                   "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                                                   "first_name", "foreground=\"darkblue\"",
                                                   "club",       "style=\"italic\" foreground=\"dimgrey\"",
                                                   "league",     "style=\"italic\" foreground=\"dimgrey\"",
                                                   "region",     "style=\"italic\" foreground=\"dimgrey\"",
                                                   "country",    "style=\"italic\" foreground=\"dimgrey\"",
                                                   NULL);

            {
              gchar *position = g_strdup_printf ("<span font_weight=\"bold\">%d</span>\302\240\302\240",
                                                 g_slist_index (_sorted_fencer_list, player) + 1);

              text_item = Canvas::PutTextInTable (player_table,
                                                  position,
                                                  0,
                                                  0);
              g_object_set (G_OBJECT (text_item),
                            "use-markup", TRUE,
                            "wrap",       PANGO_WRAP_CHAR,
                            NULL);
              g_free (position);
            }

            Canvas::PutInTable (match_table,
                                player_table,
                                0,
                                1);
            Canvas::PutInTable (player_table,
                                image,
                                0,
                                1);

            {
              GooCanvasItem *score_table = match->GetScoreTable (match_table,
                                                                 10);

              Canvas::PutInTable (match_table,
                                  score_table,
                                  0,
                                  2);
              Canvas::SetTableItemAttribute (score_table, "x-align", 1.0);
              Canvas::SetTableItemAttribute (score_table, "x-expand", 1u);
              Canvas::SetTableItemAttribute (score_table, "y-align", 0.5);
            }
          }

          {
            Player        *player = match->GetOpponent (1);
            GooCanvasItem *player_table = goo_canvas_table_new (match_table, NULL);
            GooCanvasItem *image        = GetPlayerImage (player_table,
                                                          "font_desc=\"" BP_FONT "14.0px\"",
                                                          player,
                                                          nullptr,
                                                          "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                                                          "first_name", "foreground=\"darkblue\"",
                                                          "club",       "style=\"italic\" foreground=\"dimgrey\"",
                                                          "league",     "style=\"italic\" foreground=\"dimgrey\"",
                                                          "region",     "style=\"italic\" foreground=\"dimgrey\"",
                                                          "country",    "style=\"italic\" foreground=\"dimgrey\"",
                                                          NULL);

            {
              gchar *position = g_strdup_printf ("<span font_weight=\"bold\">%d</span>\302\240\302\240",
                                                 g_slist_index (_sorted_fencer_list, player) + 1);

              text_item = Canvas::PutTextInTable (player_table,
                                                  position,
                                                  0,
                                                  0);
              g_object_set (G_OBJECT (text_item),
                            "use-markup", TRUE,
                            "wrap",       PANGO_WRAP_CHAR,
                            NULL);
              g_free (position);
            }

            Canvas::PutInTable (match_table,
                                player_table,
                                1,
                                1);
            Canvas::PutInTable (player_table,
                                image,
                                0,
                                1);

            {
              GooCanvasItem *score_table = match->GetScoreTable (match_table,
                                                                 10);

              if (score_table)
              {
                Canvas::PutInTable (match_table,
                                    score_table,
                                    1,
                                    2);
                Canvas::SetTableItemAttribute (score_table, "x-align", 1.0);
                Canvas::SetTableItemAttribute (score_table, "x-expand", 1u);
                Canvas::SetTableItemAttribute (score_table, "y-align", 0.5);
              }
            }
          }

          Canvas::PutInTable (match_main_table,
                              match_table,
                              m/nb_column,
                              m%nb_column + 2*(m%nb_column) + 1);

          Canvas::SetTableItemAttribute (match_table, "x-fill", 1U);

          // Adjust the number of columns according to
          // the grid width
          if (m/nb_column == 0)
          {
            GooCanvasBounds grid_bounds;
            GooCanvasBounds main_table_bounds;

            goo_canvas_item_get_bounds (grid_group,
                                        &grid_bounds);
            goo_canvas_item_get_bounds (match_main_table,
                                        &main_table_bounds);

            if ((main_table_bounds.x2 - main_table_bounds.x1) < (grid_bounds.x2 - grid_bounds.x1))
            {
              nb_column++;
            }
          }
        }

        Canvas::Anchor (match_main_table,
                        grid_group,
                        nullptr,
                        cell_w/2);
        Canvas::VAlign (match_main_table,
                        Canvas::Alignment::START,
                        grid_group,
                        Canvas::Alignment::START);
      }
    }

    if (print_for_referees == FALSE)
    {
      RefreshScoreData ();
      RefreshDashBoard ();
    }

    if (print_for_referees == FALSE)
    {
      guint nb_players = GetNbPlayers ();

      for (guint i = 0; i < nb_players; i++)
      {
        Player    *player;
        GtkWidget *w;

        player = GetPlayer (i, _sorted_fencer_list);
        w      = GTK_WIDGET (player->GetPtrData (this, "combo_box"));

        if (w)
        {
          if (_locked)
          {
            gtk_widget_set_sensitive (w,
                                      FALSE);
          }
          else
          {
            g_signal_connect (w, "changed",
                              G_CALLBACK (on_status_changed), this);
            g_signal_connect (w, "scroll-event",
                              G_CALLBACK (on_status_scrolled), this);
          }
          player->RemoveData (this, "combo_box");
        }
      }
    }

    return root_item;
  }

  // --------------------------------------------------------------------------------
  void Pool::SetDisplayData (Player      *player,
                             GooCanvas   *on_canvas,
                             const gchar *name,
                             void        *value)
  {
    player->SetData (GetDataOwner (), name, value);

    _display_data = g_slist_prepend (_display_data, (void *) name);
  }

  // --------------------------------------------------------------------------------
  GooCanvasItem *Pool::DrawPage (GtkPrintOperation *operation,
                                 GooCanvas         *canvas)
  {
    gboolean for_referees = (gboolean) GPOINTER_TO_INT(g_object_get_data (G_OBJECT (operation), "print_for_referees"));

    return Draw (canvas,
                 for_referees,
                 for_referees);
  }

  // --------------------------------------------------------------------------------
  void Pool::OnPlugged ()
  {
    CanvasModule::OnPlugged ();
    Draw (GetCanvas (),
          FALSE,
          TRUE);
  }

  // --------------------------------------------------------------------------------
  void Pool::OnUnPlugged ()
  {
    Wipe ();
    CanvasModule::OnUnPlugged ();
  }

  // --------------------------------------------------------------------------------
  gint Pool::_ComparePlayer (Player *A,
                             Player *B,
                             Pool   *pool)
  {
    return ComparePlayer (A,
                          B,
                          pool->GetDataOwner (),
                          pool->_rand_seed,
                          WITH_CALCULUS | WITH_RANDOM);
  }

  // --------------------------------------------------------------------------------
  gint Pool::CompareStatus (gchar A,
                            gchar B)
  {
    static char status_value[] = {'Q', 'N', 'A', 'E', 'F', 0};
    gint a_value;
    gint b_value;

    for (a_value = 0; status_value[a_value] != 0 && status_value[a_value] != A; a_value++);
    for (b_value = 0; status_value[b_value] != 0 && status_value[b_value] != B; b_value++);

    if (a_value < b_value)
    {
      return -1;
    }
    else
    {
      return 1;
    }
  }

  // --------------------------------------------------------------------------------
  gint Pool::ComparePlayer (Player   *A,
                            Player   *B,
                            Object   *data_owner,
                            guint32   rand_seed,
                            guint     comparison_policy)
  {
    if (B == nullptr)
    {
      return 1;
    }
    else if (A == nullptr)
    {
      return -1;
    }
    else if (comparison_policy & WITH_CALCULUS)
    {
      guint        pool_nr_A;
      guint        pool_nr_B;
      guint        ratio_A;
      guint        ratio_B;
      gint         average_A;
      gint         average_B;
      guint        HS_A;
      guint        HS_B;
      const gchar *status_A  = "Q";
      const gchar *status_B  = "Q";
      Player::AttributeId attr_id ("", data_owner);

      attr_id._name = (gchar *) "pool_nr";
      pool_nr_A = A->GetAttribute (&attr_id)->GetUIntValue ();
      pool_nr_B = B->GetAttribute (&attr_id)->GetUIntValue ();

      attr_id._name = (gchar *) "victories_ratio";
      ratio_A = A->GetAttribute (&attr_id)->GetUIntValue ();
      ratio_B = B->GetAttribute (&attr_id)->GetUIntValue ();

      attr_id._name = (gchar *) "indice";
      average_A = A->GetAttribute (&attr_id)->GetIntValue ();
      average_B = B->GetAttribute (&attr_id)->GetIntValue ();

      attr_id._name = (gchar *) "HS";
      HS_A = A->GetAttribute (&attr_id)->GetUIntValue ();
      HS_B = B->GetAttribute (&attr_id)->GetUIntValue ();

      attr_id._name = (gchar *) "status";
      if (A->GetAttribute (&attr_id))
      {
        status_A = A->GetAttribute (&attr_id)->GetStrValue ();
      }
      if (B->GetAttribute (&attr_id))
      {
        status_B = B->GetAttribute (&attr_id)->GetStrValue ();
      }

      if (status_A[0] != status_B[0])
      {
        if (status_A[0] == 'E')
        {
          return 1;
        }
        if (status_B[0] == 'E')
        {
          return -1;
        }
      }

      if (comparison_policy & WITH_POOL_NR)
      {
        if (pool_nr_B != pool_nr_A)
        {
          return pool_nr_A - pool_nr_B;
        }
      }

      if (status_A[0] != status_B[0])
      {
        return CompareStatus (status_A[0],
                              status_B[0]);
      }

      if (ratio_B != ratio_A)
      {
        return ratio_B - ratio_A;
      }
      if (average_B != average_A)
      {
        return average_B - average_A;
      }
      if (HS_B != HS_A)
      {
        return HS_B - HS_A;
      }
    }

    if (comparison_policy & WITH_RANDOM)
    {
      return Player::RandomCompare (A,
                                    B,
                                    rand_seed);
    }

    return 0;
  }

  // --------------------------------------------------------------------------------
  void Pool::Lock ()
  {
    _locked = TRUE;

    RefreshScoreData ();
    if (_score_collector)
    {
      _score_collector->Lock ();
    }

    if (_title_table)
    {
      Wipe ();
      Draw (GetCanvas (),
            FALSE,
            TRUE);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::UnLock ()
  {
    _locked = FALSE;

    if (_score_collector)
    {
      _score_collector->UnLock ();
    }

    if (_title_table)
    {
      Wipe ();
      Draw (GetCanvas (),
            FALSE,
            TRUE);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RefreshStatus ()
  {
    if (_status_item)
    {
      goo_canvas_item_remove (_status_item);
      _status_item = nullptr;
    }

    if (_status_pixbuf)
    {
      g_object_unref (_status_pixbuf);
      _status_pixbuf = nullptr;
    }

    if (_is_over)
    {
      _status_pixbuf = GetPixbuf (GTK_STOCK_APPLY);
    }
    else if (_has_error)
    {
      _status_pixbuf = GetPixbuf (GTK_STOCK_DIALOG_WARNING);
    }
    else
    {
      for (GSList *current = _referee_list; current; current = g_slist_next (current))
      {
        Player              *referee = (Player *) current->data;
        Player::AttributeId  connection_attr_id  ("connection");
        Attribute           *connection_attr = referee->GetAttribute (&connection_attr_id);

        if (connection_attr)
        {
          const gchar *connection_status = connection_attr->GetStrValue ();

          if (   (g_strcmp0 (connection_status, "Broken") == 0)
              || (g_strcmp0 (connection_status, "Waiting") == 0))
          {
            _status_pixbuf = connection_attr->GetPixbuf ();
            break;
          }
        }
      }

      if (_status_pixbuf == nullptr)
      {
        _status_pixbuf = GetPixbuf (GTK_STOCK_EXECUTE);
      }
    }

    if (_title_table && _status_pixbuf)
    {
      _status_item = Canvas::PutPixbufInTable (_title_table,
                                               _status_pixbuf,
                                               0, 0);
    }

    if (_status_listener)
    {
      _status_listener->OnPoolStatus (this);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RefreshScoreData ()
  {
    GSList *ranking    = nullptr;
    guint   nb_players = GetNbPlayers ();

    _is_over   = TRUE;
    _has_error = FALSE;

    // Evaluate attributes (current round & combined rounds)
    for (guint a = 0; a < nb_players; a++)
    {
      Player *player_a;
      guint   victories     = 0;
      guint   hits_scored   = 0;
      gint    hits_received = 0;

      player_a = GetPlayer (a, _sorted_fencer_list);

      for (guint b = 0; b < nb_players; b++)
      {
        if (a != b)
        {
          Player *player_b = GetPlayer (b, _sorted_fencer_list);
          Match  *match    = GetMatch (player_a, player_b);

          if (match)
          {
            Score  *score_a  = match->GetScore (player_a);
            Score  *score_b  = match->GetScore (player_b);

            if (score_a->IsKnown ())
            {
              hits_scored += score_a->Get ();
            }

            if (score_b->IsKnown ())
            {
              hits_received -= score_b->Get ();
            }

            if (   score_a->IsKnown ()
                && score_b->IsKnown ())
            {
              if (score_a->IsTheBest ())
              {
                victories++;
              }
            }

            if (match->IsOver () == FALSE)
            {
              _is_over = FALSE;
            }
            if (match->HasError ())
            {
              _has_error = TRUE;
            }
          }
        }
      }

      player_a->SetData (GetDataOwner (), "Victories", (void *) victories);
      player_a->SetData (GetDataOwner (), "HR", (void *) hits_received);

      RefreshAttribute (player_a,
                        "victories_count",
                        victories,
                        CombinedOperation::SUM);

      RefreshAttribute (player_a,
                        "bouts_count",
                        GetNbPlayers () - _nb_drop -1,
                        CombinedOperation::SUM);

      RefreshAttribute (player_a,
                        "indice",
                        hits_scored+hits_received,
                        CombinedOperation::SUM);

      RefreshAttribute (player_a,
                        "HS",
                        hits_scored,
                        CombinedOperation::SUM);

      // Ratio
      {
        guint current_round_ratio = 0;
        guint combined_ratio;

        // current
        if ((GetNbPlayers () - _nb_drop) > 1)
        {
          current_round_ratio = victories*1000 / (GetNbPlayers () - _nb_drop -1);
        }

        // combined
        combined_ratio = current_round_ratio;
        if (_previous_combined_round)
        {
          Player::AttributeId  previous_victories_attr_id ("victories_count", _previous_combined_round);
          Player::AttributeId  previous_bouts_attr_id     ("bouts_count",     _previous_combined_round);
          Attribute           *victories_attr = player_a->GetAttribute (&previous_victories_attr_id);
          Attribute           *bouts_attr     = player_a->GetAttribute (&previous_bouts_attr_id);

          if (victories_attr && bouts_attr)
          {
            guint total_victories = victories_attr->GetUIntValue () + victories;
            guint total_bouts     = bouts_attr->GetUIntValue ()     + GetNbPlayers () - _nb_drop -1;

            if (total_bouts)
            {
              combined_ratio = total_victories*1000 / total_bouts;
            }
          }
        }

        RefreshAttribute (player_a,
                          "victories_ratio",
                          current_round_ratio,
                          CombinedOperation::NONE,
                          combined_ratio);
      }

      ranking = g_slist_append (ranking,
                                player_a);
    }

    ranking = g_slist_sort_with_data (ranking,
                                      (GCompareDataFunc) _ComparePlayer,
                                      (void *) this);

    // Give players a rank for the current pool
    {
      Player::AttributeId  victories_ratio_id ("victories_ratio", GetDataOwner ());
      Player::AttributeId  indice_id          ("indice",          GetDataOwner ());
      Player::AttributeId  HS_id              ("HS",              GetDataOwner ());
      GSList *current_player  = ranking;
      Player *previous_player = nullptr;
      guint   previous_rank   = 0;

      for (guint p = 1; current_player != nullptr; p++)
      {
        Player *player;

        player = (Player *) current_player->data;
        if (   previous_player
            && (Player::Compare (previous_player, player, &victories_ratio_id) == 0)
            && (Player::Compare (previous_player, player, &indice_id) == 0)
            && (Player::Compare (previous_player, player, &HS_id) == 0))
        {
          player->SetData (this, "Rank", (void *) (previous_rank));
        }
        else
        {
          player->SetData (this, "Rank", (void *) (p));
          previous_rank = p;
        }

        previous_player = player;
        current_player  = g_slist_next (current_player);
      }
    }

    g_slist_free (ranking);

    RefreshStatus ();

    MakeDirty ();
  }

  // --------------------------------------------------------------------------------
  GdkPixbuf *Pool::GetStatusPixbuf ()
  {
    if (_status_pixbuf)
    {
      g_object_ref (_status_pixbuf);
    }

    return _status_pixbuf;
  }

  // --------------------------------------------------------------------------------
  void Pool::RefreshAttribute (Player            *player,
                               const gchar       *name,
                               guint              value,
                               CombinedOperation  operation,
                               guint              combined_value)
  {
    // Current round
    {
      Player::AttributeId current_round_attr_id  (name, GetDataOwner ());

      player->SetAttributeValue (&current_round_attr_id,
                                 value);
    }

    // Combined rounds
    {
      Player::AttributeId combined_rounds_attr_id (name, _combined_rounds_owner);

      if (_previous_combined_round == nullptr)
      {
        player->SetAttributeValue (&combined_rounds_attr_id,
                                   value);
      }
      else
      {
        Player::AttributeId  source_attr_id (name, _previous_combined_round);
        Attribute           *source_attr = player->GetAttribute (&source_attr_id);

        if (source_attr)
        {
          if (operation == CombinedOperation::AVERAGE)
          {
            player->SetAttributeValue (&combined_rounds_attr_id,
                                       (source_attr->GetUIntValue () + value) / 2);
          }
          else if (operation == CombinedOperation::SUM)
          {
            player->SetAttributeValue (&combined_rounds_attr_id,
                                       source_attr->GetUIntValue () + value);
          }
          else if (operation == CombinedOperation::NONE)
          {
            player->SetAttributeValue (&combined_rounds_attr_id,
                                       combined_value);
          }
        }
        else
        {
          g_print (RED "source_attr == NULL\n" ESC);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RefreshAttribute (Player      *player,
                               const gchar *name,
                               gchar       *value)
  {
    // Current round
    {
      Player::AttributeId current_round_attr_id  (name, GetDataOwner ());

      player->SetAttributeValue (&current_round_attr_id,
                                 value);
    }

    // Combined rounds
    {
      Player::AttributeId combined_rounds_attr_id (name, _combined_rounds_owner);

      player->SetAttributeValue (&combined_rounds_attr_id,
                                 value);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Pool::IsOver ()
  {
    return _is_over;
  }

  // --------------------------------------------------------------------------------
  gboolean Pool::HasError ()
  {
    return _has_error;
  }

  // --------------------------------------------------------------------------------
  void Pool::RefreshDashBoard ()
  {
    if (_title_table)
    {
      guint               nb_players = GetNbPlayers ();
      Player::AttributeId attr_id ("", GetDataOwner ());

      for (guint p = 0; p < nb_players; p++)
      {
        Player        *player;
        GooCanvasItem *goo_item;
        gchar         *text;
        Attribute     *attr;
        gint           value;
        void          *data;

        player = GetPlayer (p, _sorted_fencer_list);

        {
          Player::AttributeId status_attr_id ("status", GetDataOwner ());

          attr = player->GetAttribute (&status_attr_id);
          data = player->GetPtrData (GetDataOwner (), "StatusItem");
          if (attr && data)
          {
            GtkTreeIter  iter;
            gboolean     iter_is_valid;
            gchar       *code;

            text = attr->GetStrValue ();

            iter_is_valid = gtk_tree_model_get_iter_first (GetStatusModel (),
                                                           &iter);
            for (guint i = 0; iter_is_valid; i++)
            {
              gtk_tree_model_get (GetStatusModel (),
                                  &iter,
                                  AttributeDesc::DiscreteColumnId::XML_IMAGE_str, &code,
                                  -1);
              if (g_strcmp0 (text, code) == 0)
              {
                gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data),
                                               &iter);
                g_free (code);
                break;
              }

              g_free (code);
              iter_is_valid = gtk_tree_model_iter_next (GetStatusModel (),
                                                        &iter);
            }
          }
        }

        attr_id._name = (gchar *) "victories_ratio";
        attr = player->GetAttribute (&attr_id);
        data = player->GetPtrData (GetDataOwner (), "VictoriesRatioItem");
        if (data)
        {
          goo_item = GOO_CANVAS_ITEM (data);
          text = g_strdup_printf ("%0.3f", (gdouble) (attr->GetUIntValue ()) / 1000.0);
          g_object_set (goo_item,
                        "text",
                        text, NULL);
          g_free (text);
        }

        data = player->GetPtrData (GetDataOwner (), "VictoriesItem");
        if (data)
        {
          value = player->GetIntData (GetDataOwner (), "Victories");
          goo_item = GOO_CANVAS_ITEM (data);
          text = g_strdup_printf ("%d", value);
          g_object_set (goo_item,
                        "text",
                        text, NULL);
          g_free (text);
        }

        attr_id._name = (gchar *) "HS";
        attr = player->GetAttribute (&attr_id);
        data = player->GetPtrData (GetDataOwner (), "HSItem");
        if (data)
        {
          goo_item = GOO_CANVAS_ITEM (data);
          text = g_strdup_printf ("%d", attr->GetUIntValue ());
          g_object_set (goo_item,
                        "text",
                        text, NULL);
          g_free (text);
        }

        data = player->GetPtrData (GetDataOwner (), "HRItem");
        if (data)
        {
          value = player->GetIntData (GetDataOwner (), "HR");
          goo_item = GOO_CANVAS_ITEM (data);
          text = g_strdup_printf ("%d", -value);
          g_object_set (goo_item,
                        "text",
                        text, NULL);
          g_free (text);
        }

        attr_id._name = (gchar *) "indice";
        attr = player->GetAttribute (&attr_id);
        data = player->GetPtrData (GetDataOwner (), "IndiceItem");
        if (data)
        {
          goo_item = GOO_CANVAS_ITEM (data);
          text = g_strdup_printf ("%+d", attr->GetIntValue ());
          g_object_set (goo_item,
                        "text",
                        text, NULL);
          g_free (text);
        }

        data = player->GetPtrData (GetDataOwner (), "RankItem");
        if (data)
        {
          value = player->GetIntData (this, "Rank");
          goo_item = GOO_CANVAS_ITEM (data);
          text = g_strdup_printf ("%d", value);
          g_object_set (goo_item,
                        "text",
                        text, NULL);
          g_free (text);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  Match *Pool::GetMatch (Player *A,
                         Player *B)
  {
    for (GList *current = _match_list; current; current = g_list_next (current))
    {
      Match *match = (Match *) current->data;

      if (   match->HasFencer (A)
          && match->HasFencer (B))
      {
        return match;
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Pool::SetRoadmapTo (Player  *fencer,
                           guint    strip,
                           FieTime *start_time)
  {
    Player::AttributeId strip_attr_id ("strip", _rank_owner);
    Player::AttributeId time_attr_id  ("time",  _rank_owner);

    fencer->SetAttributeValue (&strip_attr_id,
                               strip);
    if (start_time)
    {
      fencer->SetAttributeValue (&time_attr_id,
                                 start_time->GetImage ());
    }
    else
    {
      fencer->SetAttributeValue (&time_attr_id,
                                 "");
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::SetRoadmap (guint    strip,
                         FieTime *start_time)
  {
    _strip = strip;

    Object::TryToRelease (_start_time);
    _start_time = start_time;

    for (GSList *current = _sorted_fencer_list; current; current = g_slist_next (current))
    {
      Player *fencer = (Player *) current->data;

      SetRoadmapTo (fencer,
                    _strip,
                    _start_time);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Pool::OnMessage (Net::Message *message)
  {
    if (message->Is ("BellePoule2D::Roadmap"))
    {
      if (message->GetInteger ("source") == _parcel->GetNetID ())
      {
        SetRoadmap (0,
                    nullptr);

        if (message->GetFitness () > 0)
        {
          gchar *start_date = message->GetString  ("start_date");
          gchar *start_time = message->GetString  ("start_time");

          SetRoadmap (message->GetInteger ("piste"),
                      new FieTime (start_date,
                                   start_time));

          g_free (start_date);
          g_free (start_time);
        }

        RefreshParcel ();

        return TRUE;
      }
    }
    else if (message->Is ("SmartPoule::Score"))
    {
      guint match_number = message->GetInteger ("bout");

      if (match_number > 0)
      {
        Match *match = GetMatch (match_number-1);

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
                for (guint i = 0; i < 2; i++)
                {
                  match->Load (xml_nodeset->nodeTab[i],
                               match->GetOpponent (i));
                }
                _score_collector->Refresh (match);
                RefreshScoreData ();
                RefreshDashBoard ();
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
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  Match *Pool::GetMatch (guint i)
  {
    guint    a_id;
    guint    b_id;
    gboolean rest_error;

    if (_dispatcher->GetPlayerPair (i,
                                    &a_id,
                                    &b_id,
                                    &rest_error))
    {
      Player *a     = (Player *) g_slist_nth_data (_sorted_fencer_list, a_id-1);
      Player *b     = (Player *) g_slist_nth_data (_sorted_fencer_list, b_id-1);
      Match  *match = GetMatch (a, b);

      match->SetData (this, "rest_error", (void *) rest_error);
      return match;
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gint Pool::CompareMatch (Match *a,
                           Match *b,
                           Pool  *pool)
  {
    return g_slist_index (pool->_sorted_fencer_list, a->GetOpponent (0)) - g_slist_index (pool->_sorted_fencer_list, b->GetOpponent (0));
  }

  // --------------------------------------------------------------------------------
  void Pool::FeedParcel (Net::Message *parcel)
  {
    xmlBuffer *xml_buffer = xmlBufferCreate ();

    {
      XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

      Save (xml_scheme);
      xml_scheme->Release ();
    }

    parcel->Set ("xml", (const gchar *) xml_buffer->content);
    xmlBufferFree (xml_buffer);

    parcel->Set ("display_position",
                 _number);

    parcel->Set ("duration_sec",
                 _duration_sec);

    parcel->Set ("workload_units",
                 _max_score->_value * GetNbMatchs ());
  }

  // --------------------------------------------------------------------------------
  void Pool::Save (XmlScheme *xml_scheme)
  {
    xml_scheme->StartElement ("Poule");

    xml_scheme->WriteFormatAttribute ("ID",
                                      "%d", _number);

    xml_scheme->WriteFormatAttribute ("NetID",
                                      "%x", _parcel->GetNetID ());

    if (_strip)
    {
      xml_scheme->WriteFormatAttribute ("Piste",
                                        "%d", _strip);
    }

    if (_start_time)
    {
      xml_scheme->WriteFormatAttribute ("Date",
                                        "%s", _start_time->GetXmlDate ());
      xml_scheme->WriteFormatAttribute ("Heure",
                                        "%s", _start_time->GetXmlTime ());
    }

    if (_duration_sec > 0)
    {
      xml_scheme->WriteFormatAttribute ("Duree",
                                        "%d", _duration_sec);
    }

    {
      GSList *fencers = g_slist_copy (_sorted_fencer_list);

      for (GSList *current = _fencer_list; current; current = g_slist_next (current))
      {
        if (g_slist_find (_sorted_fencer_list,
                          current->data) == nullptr)
        {
          fencers = g_slist_append (fencers,
                                    current->data);
        }
      }

      {
        Player::AttributeId  attr_id ("", GetDataOwner ());
        GSList              *current = fencers;

        for (guint i = 1; current != nullptr; i++)
        {
          Attribute *attr;
          Player    *player = (Player *) current->data;

          xml_scheme->StartElement (player->GetXmlTag ());

          xml_scheme->WriteFormatAttribute ("REF",
                                            "%d", player->GetRef ());

          xml_scheme->WriteFormatAttribute ("NoDansLaPoule",
                                            "%d", i);

          xml_scheme->WriteFormatAttribute ("NbVictoires",
                                            "%d", player->GetIntData (GetDataOwner (),
                                                                      "Victories"));
          xml_scheme->WriteFormatAttribute ("NbMatches",
                                            "%d", GetNbMatchs (player));

          attr_id._name = (gchar *) "HS";
          attr = player->GetAttribute (&attr_id);
          if (attr)
          {
            gint  indice;
            guint HS;

            HS = player->GetAttribute (&attr_id)->GetUIntValue ();
            xml_scheme->WriteFormatAttribute ("TD",
                                              "%d", HS);
            attr_id._name = (gchar *) "indice";
            indice = player->GetAttribute (&attr_id)->GetUIntValue ();
            xml_scheme->WriteFormatAttribute ("TR",
                                              "%d", HS - indice);
          }
          xml_scheme->WriteFormatAttribute ("RangPoule",
                                            "%d", player->GetIntData (this,
                                                                      "Rank"));
          xml_scheme->EndElement ();
          current = g_slist_next (current);
        }
      }

      g_slist_free (fencers);
    }

    for (GSList *current = _referee_list; current; current = g_slist_next (current))
    {
      Player *referee = (Player *) current->data;

      xml_scheme->StartElement (referee->GetXmlTag ());
      xml_scheme->WriteFormatAttribute ("REF",
                                        "%d", referee->GetRef ());
      xml_scheme->EndElement ();
    }

    for (GList *current = _match_list; current; current = g_list_next (current))
    {
      Match *match = (Match *) current->data;

      match->Save (xml_scheme);
    }

    xml_scheme->EndElement ();
  }

  // --------------------------------------------------------------------------------
  void Pool::Load (xmlNode *xml_node)
  {
    gchar *attr;

    attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "NetID");
    if (attr)
    {
      _parcel->SetNetID (g_ascii_strtoull (attr,
                                           nullptr,
                                           16));
      xmlFree (attr);
    }

    attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Piste");
    if (attr)
    {
      _strip = atoi (attr);
      xmlFree (attr);
    }

    {
      gchar *date = (gchar *) xmlGetProp (xml_node, BAD_CAST "Date");
      gchar *time = (gchar *) xmlGetProp (xml_node, BAD_CAST "Heure");

      if (date && time)
      {
        Object::TryToRelease (_start_time);
        _start_time = new FieTime (date, time);
      }

      xmlFree (date);
      xmlFree (time);
    }

    attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Duree");
    if (attr)
    {
      _duration_sec = atoi (attr);
      xmlFree (attr);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::Load (xmlNode *xml_node,
                   GSList  *player_list)
  {
    if (xml_node == nullptr)
    {
      return;
    }

    _is_over = TRUE;

    for (xmlNode *n = xml_node; n != nullptr; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        static xmlNode *A = nullptr;
        static xmlNode *B = nullptr;

        if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          A = nullptr;
          B = nullptr;
        }
        else if (g_strcmp0 ((char *) n->name, _xml_player_tag) == 0)
        {
          if (A == nullptr)
          {
            A = n;
          }
          else
          {
            gchar  *attr;
            Player *player_A = nullptr;
            Player *player_B = nullptr;
            Match  *match;

            B = n;
            {
              GSList *node;

              attr = (gchar *) xmlGetProp (A, BAD_CAST "REF");
              node = g_slist_find_custom (player_list,
                                          (void *) strtol (attr, nullptr, 10),
                                          (GCompareFunc) Player::CompareWithRef);
              if (node)
              {
                player_A = (Player *) node->data;
              }
              xmlFree (attr);

              attr = (gchar *) xmlGetProp (B, BAD_CAST "REF");
              node = g_slist_find_custom (player_list,
                                          (void *) strtol (attr, nullptr, 10),
                                          (GCompareFunc) Player::CompareWithRef);
              if (node)
              {
                player_B = (Player *) node->data;
              }
              xmlFree (attr);
            }

            match = GetMatch (player_A, player_B);
            if (match)
            {
              // XML files may have been generated from another software
              // where the matchs are potentially stored in a different order
              // Consequently, at the loading the initial position has to be recovered.
              if (player_A != match->GetOpponent (0))
              {
                Player  *p    = player_B;
                xmlNode *temp = B;

                player_B = player_A;
                player_A = p;

                B = A;
                A = temp;
              }

              {
                match->Load (A, player_A,
                             B, player_B);

                if (match->IsOver () == FALSE)
                {
                  _is_over = FALSE;
                }
                if (match->HasError ())
                {
                  _has_error = TRUE;
                }
              }
            }
            return;
          }
        }

        Load (n->children,
              player_list);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::DumpToHTML (FILE                *file,
                         Player              *fencer,
                         const gchar         *attr_name,
                         AttributeDesc::Look  look)

  {
    AttributeDesc       *desc = AttributeDesc::GetDescFromCodeName (attr_name);
    Player::AttributeId *attr_id;
    Attribute           *attr;

    if (desc->_scope == AttributeDesc::Scope::LOCAL)
    {
      attr_id = new Player::AttributeId (desc->_code_name,
                                         _owner);
    }
    else
    {
      attr_id = new Player::AttributeId (desc->_code_name);
    }

    attr = fencer->GetAttribute (attr_id);
    attr_id->Release ();

    if (attr)
    {
      gchar *attr_image = attr->GetUserImage (look);

      fprintf (file, "            <td>\n");
      fprintf (file, "              <span class=\"%s\">%s </span>", attr_name, attr_image);
      fprintf (file, "            </td>\n");
      g_free (attr_image);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::DumpToHTML (FILE  *file,
                         guint  grid_size)
  {
    guint nb_players  = GetNbPlayers ();

    // Header
    {
      fprintf (file, "          <tr class=\"PoolName\">\n");
      fprintf (file, "            <td>%s</td>\n", GetName ());

      fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      fprintf (file, "            <td class=\"GridSeparator\"></td>\n");

      for (guint i = 0; i < nb_players; i++)
      {
        fprintf (file, "            <td class=\"GridSeparator\" style=\"text-align: center\";>%d</td>\n", i+1);
      }

      fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      if (nb_players < grid_size)
      {
        fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      }

      {
        fprintf (file, "            <td class=\"GridSeparator\" style=\"text-align: center\";>#</td>\n");
        fprintf (file, "            <td class=\"GridSeparator\" style=\"text-align: center\";>V</td>\n");
        fprintf (file, "            <td class=\"GridSeparator\" style=\"text-align: center\";>+/-</td>\n");
        fprintf (file, "            <td class=\"GridSeparator\" style=\"text-align: center\";>+</td>\n");
      }

      fprintf (file, "          </tr>\n");
    }

    for (guint i = 0; i < nb_players; i++)
    {
      Player *A = GetPlayer (i, _sorted_fencer_list);

      fprintf (file, "          <tr class=\"EvenRow\">\n");

      DumpToHTML (file,
                  A,
                  "name",
                  AttributeDesc::LONG_TEXT);
      DumpToHTML (file,
                  A,
                  "first_name",
                  AttributeDesc::LONG_TEXT);
      DumpToHTML (file,
                  A,
                  "country",
                  AttributeDesc::SHORT_TEXT);

      fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      fprintf (file, "            <td class=\"GridSeparator\">%d</td>\n", i+1);

      for (guint j = 0; j < nb_players; j++)
      {
        if (i != j)
        {
          Player *B     = GetPlayer (j, _sorted_fencer_list);
          Match  *match = GetMatch (A, B);

          if (match)
          {
            if (match->IsDropped ())
            {
              fprintf (file, "            <td class=\"NoScoreCell\"></td>\n");
            }
            else
            {
              Score *score       = match->GetScore (A);
              gchar *score_image = score->GetImage ();

              fprintf (file, "            <td class=\"PoolCell\">%s</td>\n", score_image);
              g_free (score_image);
            }
          }
          else
          {
            fprintf (file, "            <td class=\"NoScoreCell\"></td>\n");
          }
        }
        else
        {
          fprintf (file, "            <td class=\"NoScoreCell\"></td>\n");
        }
      }

      fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      if (nb_players < grid_size)
      {
        fprintf (file, "            <td class=\"GridSeparator\"></td>\n");
      }

      {
        Player::AttributeId attr_id ("", GetDataOwner ());

        fprintf (file, "            <td class=\"DashBoardCell\">%d</td>\n", A->GetIntData (this, "Rank"));

        attr_id._name = (gchar *) "victories_ratio";
        fprintf (file, "            <td class=\"DashBoardCell\">%1.3f</td>\n", (float) (A->GetAttribute (&attr_id)->GetUIntValue ()) / 1000.0);

        attr_id._name = (gchar *) "indice";
        fprintf (file, "            <td class=\"DashBoardCell\">%d</td>\n", A->GetAttribute (&attr_id)->GetIntValue ());

        attr_id._name = (gchar *) "HS";
        fprintf (file, "            <td class=\"DashBoardCell\">%d</td>\n", A->GetAttribute (&attr_id)->GetUIntValue ());
      }

      fprintf (file, "          </tr>\n");
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::CleanScores ()
  {
    for (GList *current = _match_list; current; current = g_list_next (current))
    {
      Match *match = (Match *) current->data;

      match->CleanScore ();
    }

    if (_match_list)
    {
      RefreshScoreData ();
    }

    _is_over = FALSE;
  }

  // --------------------------------------------------------------------------------
  gint Pool::_ComparePlayerWithFullRandom (Player *A,
                                           Player *B,
                                           Pool   *pool)
  {
    return ComparePlayer (A,
                          B,
                          nullptr,
                          pool->_rand_seed,
                          WITH_RANDOM);
  }

  // --------------------------------------------------------------------------------
  void Pool::SortPlayers ()
  {
    g_slist_free (_sorted_fencer_list);
    _sorted_fencer_list = nullptr;

    for (GSList *current = _fencer_list; current; current = g_slist_next (current))
    {
      Player *fencer = (Player *) current->data;

      if (FencerIsAbsent (fencer) == FALSE)
      {
        _sorted_fencer_list = g_slist_insert_sorted_with_data (_sorted_fencer_list,
                                                               fencer,
                                                               (GCompareDataFunc) _ComparePlayerWithFullRandom,
                                                               (void *) this);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::DeleteMatchs ()
  {
    _duration_sec = 0;

    CleanScores ();

    FreeFullGList (Match, _match_list);

    for (GSList *current = _fencer_list; current; current = g_slist_next (current))
    {
      Player              *player         = (Player *) current->data;
      Player::AttributeId  status_attr_id = Player::AttributeId ("status", GetDataOwner ());

      player->RemoveAttribute (&status_attr_id);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::DropPlayer (Player *player,
                         gchar  *reason)
  {
    Player::AttributeId  status_attr_id = Player::AttributeId ("status", GetDataOwner ());
    Attribute           *status_attr    = player->GetAttribute (&status_attr_id);

    if (status_attr)
    {
      gchar *status = status_attr->GetStrValue ();

      if (status && (*status == 'Q'))
      {
        _nb_drop++;
      }
    }

    RefreshAttribute (player,
                      "status",
                      reason);

    for (guint i = 0; i < GetNbPlayers (); i++)
    {
      Player *opponent = GetPlayer (i, _sorted_fencer_list);

      if (opponent != player)
      {
        Match *match = GetMatch (opponent, player);

        if (match)
        {
          match->DropFencer (player,
                             reason);
        }
      }
    }

    if (_title_table)
    {
      Wipe ();
      Draw (GetCanvas (),
            FALSE,
            TRUE);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RestorePlayer (Player *player)
  {
    Player::AttributeId status_attr_id = Player::AttributeId ("status", GetDataOwner ());
    Attribute           *status_attr   = player->GetAttribute (&status_attr_id);
    gboolean             dropped       = FALSE;

    if (status_attr)
    {
      gchar *status = status_attr->GetStrValue ();

      if (status && (*status != 'Q') && (*status != 'N'))
      {
        dropped = TRUE;
      }
    }

    RefreshAttribute (player,
                      "status",
                      (gchar *) "Q");

    if (dropped)
    {
      _nb_drop--;

      for (guint i = 0; i < GetNbPlayers (); i++)
      {
        Player *opponent = GetPlayer (i, _sorted_fencer_list);

        if (opponent != player)
        {
          Match *match = GetMatch (opponent, player);

          if (match)
          {
            match->RestoreFencer (player);
          }
        }
      }
    }

    if (_title_table)
    {
      Wipe ();
      Draw (GetCanvas (),
            FALSE,
            TRUE);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::OnStatusChanged (GtkComboBox *combo_box)
  {
    Player      *player = (Player *) g_object_get_data (G_OBJECT (combo_box), "player");
    GtkTreeIter  iter;
    gchar       *code;

    gtk_combo_box_get_active_iter (combo_box,
                                   &iter);
    gtk_tree_model_get (GetStatusModel (),
                        &iter,
                        AttributeDesc::DiscreteColumnId::XML_IMAGE_str, &code,
                        -1);

    if (code && *code !='Q')
    {
      DropPlayer (player,
                  code);
    }
    else
    {
      RestorePlayer (player);
    }

    g_free (code);
  }

  // --------------------------------------------------------------------------------
  void Pool::on_status_changed (GtkComboBox *combo_box,
                                Pool        *pool)
  {
    pool->OnStatusChanged (combo_box);
  }

  // --------------------------------------------------------------------------------
  gboolean Pool::on_status_scrolled (GtkWidget *widget,
                                     GdkEvent  *event,
                                     gpointer   user_data)
  {
    gtk_widget_event (gtk_widget_get_parent (widget),
                      event);
    return TRUE;
  }
}
