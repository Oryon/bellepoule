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
#include <goocanvas.h>
#include <cairo.h>
#include <cairo-pdf.h>

#include "common/match.hpp"

#include "pool_match_order.hpp"

#include "pool.hpp"

namespace Pool
{
  // --------------------------------------------------------------------------------
  Pool::Pool (Data        *max_score,
              guint        number,
              gchar        weapon_code,
              const gchar *xml_player_tag,
              guint32      rand_seed)
    : CanvasModule ("pool.glade",
                    "canvas_scrolled_window")
  {
    _number             = number;
    _fencer_list        = NULL;
    _referee_list       = NULL;
    _sorted_fencer_list = NULL;
    _match_list         = NULL;
    _is_over            = FALSE;
    _has_error          = FALSE;
    _title_table        = NULL;
    _status_item        = NULL;
    _status_pixbuf      = NULL;
    _locked             = FALSE;
    _max_score          = max_score;
    _display_data       = NULL;
    _nb_drop            = 0;
    _rand_seed          = rand_seed;
    _xml_player_tag     = xml_player_tag;

    _match_order = new MatchOrder (weapon_code);

    _status_cbk_data = NULL;
    _status_cbk      = NULL;

    _score_collector = NULL;

    {
      gchar *text = g_strdup_printf (gettext ("Pool #%02d"), _number);

      _name = GetUndivadableText (text);
      g_free (text);
    }
  }

  // --------------------------------------------------------------------------------
  Pool::~Pool ()
  {
    Wipe ();

    DeleteMatchs ();

    g_free (_name);

    while (_referee_list)
    {
      RemoveReferee ((Player *) _referee_list->data);
    }

    g_slist_free (_fencer_list);
    g_slist_free (_sorted_fencer_list);

    Object::TryToRelease (_score_collector);

    _match_order->Release ();
  }

  // --------------------------------------------------------------------------------
  void Pool::Wipe ()
  {
    _title_table   = NULL;
    _status_item   = NULL;

    if (_status_pixbuf)
    {
      g_object_unref (_status_pixbuf);
      _status_pixbuf = NULL;
    }

    for (guint i = 0; i < GetNbPlayers (); i++)
    {
      Player *player;
      GSList *current;

      player  = (Player *) g_slist_nth_data (_fencer_list, i);

      current = _display_data;
      while (current)
      {
        player->RemoveData (GetDataOwner (),
                            (gchar *) current->data);

        current = g_slist_next (current);
      }
    }

    g_slist_free (_display_data);
    _display_data = NULL;

    CanvasModule::Wipe ();
  }

  // --------------------------------------------------------------------------------
  void Pool::Stuff ()
  {
    GSList *current = _match_list;

    while (current)
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
      current = g_slist_next (current);
    }

    RefreshScoreData ();
  }

  // --------------------------------------------------------------------------------
  void Pool::SetStatusCbk (StatusCbk  cbk,
                           void      *data)
  {
    _status_cbk_data = data;
    _status_cbk      = cbk;

    RefreshStatus ();
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetNumber ()
  {
    return _number;
  }

  // --------------------------------------------------------------------------------
  gchar *Pool::GetName ()
  {
    return _name;
  }

  // --------------------------------------------------------------------------------
  void Pool::SetDataOwner (Object *single_owner,
                           Object *combined_owner,
                           Object *combined_source_owner)
  {
    Module::SetDataOwner (combined_owner);

    _single_owner          = single_owner;
    _combined_source_owner = combined_source_owner;

    _nb_drop = 0;

    {
      Player::AttributeId  single_attr_id  ("pool_nr", _single_owner);
      Player::AttributeId  combined_attr_id ("pool_nr", GetDataOwner ());
      GSList              *current = _fencer_list;

      while (current)
      {
        Player *player = (Player *) current->data;

        player->SetAttributeValue (&single_attr_id,
                                   _number);
        player->SetAttributeValue (&combined_attr_id,
                                   _number);
        current = g_slist_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::AddReferee (Player *referee)
  {
    if (referee == NULL)
    {
      return;
    }

    if (   (_referee_list == NULL)
        || (g_slist_find (_referee_list,
                          referee) == NULL))
    {
      _referee_list = g_slist_prepend (_referee_list,
                                       referee);

      referee->SetChangeCbk ("connection",
                             (Player::OnChange) OnAttrConnectionChanged,
                             this);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::AddFencer (Player *player,
                        Object *rank_owner)
  {
    if (player == NULL)
    {
      return;
    }

    if (   (_fencer_list == NULL)
        || (g_slist_find (_fencer_list,
                          player) == NULL))
    {
      {
        Player::AttributeId attr_id ("pool_nr", rank_owner);

        player->SetAttributeValue (&attr_id,
                                   GetNumber ());
      }

      {
        Player::AttributeId attr_id ("previous_stage_rank", rank_owner);

        attr_id.MakeRandomReady (_rand_seed);
        _fencer_list = g_slist_insert_sorted_with_data (_fencer_list,
                                                        player,
                                                        (GCompareDataFunc) Player::Compare,
                                                        &attr_id);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RemoveFencer (Player *player)
  {
    if (g_slist_find (_fencer_list,
                      player))
    {
      _fencer_list = g_slist_remove (_fencer_list,
                                     player);
    }
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
  void Pool::CreateMatchs (AttributeDesc *affinity_criteria)
  {
    SortPlayers ();

    if (_match_list == NULL)
    {
      _match_order->SetAffinityCriteria (affinity_criteria,
                                         _sorted_fencer_list);

      {
        guint nb_players = GetNbPlayers ();
        guint nb_matchs  = (nb_players*nb_players - nb_players) / 2;

        for (guint i = 0; i < nb_matchs; i++)
        {
          guint a_id;
          guint b_id;

          if (_match_order->GetPlayerPair (i,
                                           &a_id,
                                           &b_id))
          {
            Player *a     = (Player *) g_slist_nth_data (_sorted_fencer_list, a_id-1);
            Player *b     = (Player *) g_slist_nth_data (_sorted_fencer_list, b_id-1);
            Match  *match = new Match (a,
                                       b,
                                       _max_score);
            match->SetNameSpace ("M");
            match->SetNumber (i+1);

            _match_list = g_slist_append (_match_list,
                                          match);
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::CopyPlayersStatus (Object *from)
  {
    GSList *current = _fencer_list;

    while (current)
    {
      Player::AttributeId  attr_id ("status", from);
      Attribute           *status_attr;
      Player              *player;
      gchar               *status;

      player = (Player *) current->data;
      RestorePlayer (player);

      status_attr = player->GetAttribute (&attr_id);
      status      = status_attr->GetStrValue ();

      if (   (status[0] == 'A')
          || (status[0] == 'F')
          || (status[0] == 'E'))
      {
        DropPlayer (player,
                    status);
      }
      current = g_slist_next (current);
    }

    RefreshScoreData ();
    RefreshDashBoard ();
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetNbPlayers ()
  {
    return g_slist_length (_fencer_list);
  }

  // --------------------------------------------------------------------------------
  guint Pool::GetNbMatchs ()
  {
    guint nb_players = g_slist_length (_fencer_list);

    return ((nb_players*nb_players - nb_players) / 2);
  }

  // --------------------------------------------------------------------------------
  GSList *Pool::GetFencerList ()
  {
    return _fencer_list;
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
  void Pool::OnNewScore (ScoreCollector *score_collector,
                         CanvasModule   *client,
                         Match          *match,
                         Player         *player)
  {
    Pool *pool = dynamic_cast <Pool *> (client);

    pool->RefreshScoreData ();
    pool->RefreshDashBoard ();
  }

  // --------------------------------------------------------------------------------
  void Pool::Draw (GooCanvas *on_canvas,
                   gboolean   print_for_referees,
                   gboolean   print_matchs)
  {
    if (_score_collector)
    {
      GSList *current = _match_list;

      while (current)
      {
        Match *match = (Match *) current->data;

        _score_collector->RemoveCollectingPoints (match);
        current = g_slist_next (current);
      }

      _score_collector->Release ();
    }

    _score_collector = new ScoreCollector (this,
                                           (ScoreCollector::OnNewScore_cbk) &Pool::OnNewScore);

    {
      const guint    cell_w      = 45;
      const guint    cell_h      = 45;
      guint          nb_players  = GetNbPlayers ();
      GooCanvasItem *root_item   = goo_canvas_get_root_item (on_canvas);
      GooCanvasItem *title_group = goo_canvas_group_new (root_item, NULL);
      GooCanvasItem *grid_group  = goo_canvas_group_new (root_item, NULL);
      GooCanvasItem *grid_header = goo_canvas_group_new (grid_group, NULL);
      GooCanvasItem *grid;

      // Name
      {
        GooCanvasItem *text_item;

        _title_table = goo_canvas_table_new (title_group, NULL);
        text_item = Canvas::PutTextInTable (_title_table,
                                            _name,
                                            0,
                                            1);
        g_object_set (G_OBJECT (text_item),
                      "font", "Sans bold 18px",
                      NULL);
      }

      // Referee / Track
      {
        GooCanvasItem *referee_group = goo_canvas_group_new (title_group, NULL);
        GooCanvasItem *track_group   = goo_canvas_group_new (title_group, NULL);

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
                             "font", "Sans bold 30.0px",
                             NULL);
        if (_referee_list)
        {
          Player *referee = (Player *) _referee_list->data;

          goo_canvas_text_new (referee_group,
                               referee->GetName (),
                               5.0,
                               cell_h/2.0,
                               -1,
                               GTK_ANCHOR_W,
                               "fill-color", "Black",
                               "font", "Sans bold 25.0px",
                               NULL);
        }

        Canvas::HAlign (referee_group,
                        Canvas::START,
                        _title_table,
                        Canvas::START);
        Canvas::Anchor (referee_group,
                        NULL,
                        _title_table,
                        cell_w*5);

        goo_canvas_rect_new (track_group,
                             0.0,
                             0.0,
                             cell_w*5,
                             cell_h,
                             "stroke-color", "Grey",
                             "line-width", 2.0,
                             NULL);
        goo_canvas_text_new (track_group,
                             gettext ("Piste"),
                             0.0,
                             0.0,
                             -1,
                             GTK_ANCHOR_SW,
                             "fill-color", "Grey",
                             "font", "Sans bold 30.0px",
                             NULL);

        Canvas::HAlign (track_group,
                        Canvas::START,
                        referee_group,
                        Canvas::START);
        Canvas::Anchor (track_group,
                        NULL,
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
          GooCanvasItem *previous_goo_rect = NULL;

          for (guint i = 0; i < nb_players; i++)
          {
            Player *A;

            A = GetPlayer (i, _sorted_fencer_list);

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
                                                    "font", "Sans bold 18px",
                                                    NULL);
                  if (print_for_referees)
                  {
                    g_object_set (score_text,
                                  "fill-color", "Grey",
                                  NULL);
                  }
                  g_free (score_image);
                }

                if (   (print_for_referees == FALSE)
                    && (_locked == FALSE)
                    && (match->IsDropped () == FALSE))
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
                                  "font_desc=\"Sans 14.0px\"",
                                  GetPlayer (i, _sorted_fencer_list),
                                  "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                                  "first_name", "foreground=\"darkblue\"",
                                  "club",       "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
                                  "league",     "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
                                  "country",    "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
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
                          "ellipsize", PANGO_ELLIPSIZE_NONE,
                          "wrap",      PANGO_WRAP_CHAR,
                          NULL);
            g_free (index);
          }

          goo_canvas_item_translate (player_table,
                                     0,
                                     y);
          Canvas::VAlign (player_table,
                          Canvas::END,
                          grid,
                          Canvas::START);
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
                               "font", "Sans 18px",
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
                                 "fill-color", "Grey85",
                                 "stroke-color", "White",
                                 "line-width", 2.0,
                                 NULL);
            goo_canvas_text_new (dashboard_body,
                                 gettext ("Signature"),
                                 0.0,
                                 cell_h * (i + (1.0/2.0)),
                                 -1,
                                 GTK_ANCHOR_W,
                                 "fill-color", "Grey95",
                                 "font", "Sans bold 30.0px",
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
                                            "font", "Sans 18px",
                                            NULL);
            goo_canvas_item_rotate (goo_text, 315, 0.0, y);
            x += cell_w;
          }

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Victories"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", "Sans 18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Vict./Matchs"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", "Sans 18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("H. scored"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", "Sans 18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("H. received"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", "Sans 18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Index"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", "Sans 18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          goo_text = goo_canvas_text_new (dashboard_header,
                                          gettext ("Place"),
                                          x, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", "Sans 18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, x, y);
          x += cell_w;

          for (guint i = 0; i < nb_players; i++)
          {
            Player *player;

            player = GetPlayer (i, _sorted_fencer_list);

            {
              GooCanvasItem *goo_item;
              gint           x, y;

              x = cell_w/2;
              y = cell_h/2 + i*cell_h;

              {
                if (GetCanvas () == on_canvas)
                {
                  GtkWidget       *w    = gtk_combo_box_new_with_model (GetStatusModel ());
                  GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();

                  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (w), cell, FALSE);
                  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (w),
                                                  cell, "pixbuf", AttributeDesc::DISCRETE_ICON_pix,
                                                  NULL);

                  g_object_set_data (G_OBJECT (w), "player",  player);
                  player->SetData (this, "combo_box", w);
                  goo_item = goo_canvas_widget_new (dashboard_body,
                                                    w,
                                                    x-cell_w/2,
                                                    y,
                                                    cell_w*3/2,
                                                    cell_h*2/3,
                                                    "anchor", GTK_ANCHOR_CENTER,
                                                    NULL);
                  SetDisplayData (player, on_canvas, "StatusItem", w);
                  x += cell_w*3/2;
                  gtk_widget_show (w);
                }

                goo_item = goo_canvas_text_new (dashboard_body,
                                                ".",
                                                x, y, -1,
                                                GTK_ANCHOR_CENTER,
                                                "font", "Sans 18px",
                                                NULL);
                SetDisplayData (player, on_canvas, "VictoriesItem", goo_item);
                x += cell_w;

                goo_item = goo_canvas_text_new (dashboard_body,
                                                ".",
                                                x, y, -1,
                                                GTK_ANCHOR_CENTER,
                                                "font", "Sans 18px",
                                                NULL);
                SetDisplayData (player, on_canvas, "VictoriesRatioItem", goo_item);
                x += cell_w;

                goo_item = goo_canvas_text_new (dashboard_body,
                                                ".",
                                                x, y, -1,
                                                GTK_ANCHOR_CENTER,
                                                "font", "Sans 18px",
                                                NULL);
                SetDisplayData (player, on_canvas, "HSItem", goo_item);
                x += cell_w;

                goo_item = goo_canvas_text_new (dashboard_body,
                                                ".",
                                                x, y, -1,
                                                GTK_ANCHOR_CENTER,
                                                "font", "Sans 18px",
                                                NULL);
                SetDisplayData (player, on_canvas, "HRItem", goo_item);
                x += cell_w;

                goo_item = goo_canvas_text_new (dashboard_body,
                                                ".",
                                                x, y, -1,
                                                GTK_ANCHOR_CENTER,
                                                "font", "Sans 18px",
                                                NULL);
                SetDisplayData (player, on_canvas, "IndiceItem", goo_item);
                x += cell_w;

                goo_item = goo_canvas_text_new (dashboard_body,
                                                ".",
                                                x, y, -1,
                                                GTK_ANCHOR_CENTER,
                                                "font", "Sans 18px",
                                                NULL);
                SetDisplayData (player, on_canvas, "RankItem", goo_item);
                x += cell_w;
              }
            }
          }
        }

        Canvas::VAlign (grid_group,
                        Canvas::START,
                        title_group,
                        Canvas::START,
                        cell_h/2);

        Canvas::Anchor (dashboard_group,
                        title_group,
                        NULL,
                        cell_h/2);

        Canvas::Anchor (dashboard_group,
                        NULL,
                        grid_group,
                        cell_w/2);

        if (print_for_referees)
        {
          Canvas::HAlign (grid_group,
                          Canvas::START,
                          dashboard_body,
                          Canvas::START,
                          -((gdouble) cell_h)/2.0);
        }
        else
        {
          GooCanvasBounds grid_header_bounds;

          goo_canvas_item_get_bounds (grid_header,
                                      &grid_header_bounds);
          Canvas::HAlign (grid_group,
                          Canvas::START,
                          dashboard_body,
                          Canvas::START,
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

        for (guint m = 0; m < g_slist_length (_match_list); m++)
        {
          Match         *match;
          GooCanvasItem *match_table;

          match_table = goo_canvas_table_new (match_main_table,
                                              "column-spacing", 1.5,
                                              NULL);

          match = GetMatch (m);

          {
            text_item = Canvas::PutTextInTable (match_main_table,
                                                match->GetName (),
                                                m/nb_column,
                                                m%nb_column + 2*(m%nb_column));
            g_object_set (G_OBJECT (text_item),
                          "font", "Sans bold 18px",
                          NULL);
            Canvas::SetTableItemAttribute (text_item, "y-align", 0.5);
          }

          {
            Player *player              = match->GetOpponent (0);
            GooCanvasItem *player_table = goo_canvas_table_new (match_table, NULL);
            GooCanvasItem *image = GetPlayerImage (player_table,
                                                   "font_desc=\"Sans 14.0px\"",
                                                   player,
                                                   "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                                                   "first_name", "foreground=\"darkblue\"",
                                                   "club",       "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
                                                   "league",     "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
                                                   "country",    "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
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
                            "ellipsize",  PANGO_ELLIPSIZE_NONE,
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
                                                          "font_desc=\"Sans 14.0px\"",
                                                          player,
                                                          "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                                                          "first_name", "foreground=\"darkblue\"",
                                                          "club",       "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
                                                          "league",     "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
                                                          "country",    "style=\"italic\" size=\"x-small\" foreground=\"dimgrey\"",
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
                            "ellipsize",  PANGO_ELLIPSIZE_NONE,
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
                        NULL,
                        cell_w/2);
        Canvas::VAlign (match_main_table,
                        Canvas::START,
                        grid_group,
                        Canvas::START);
      }
    }

    if (print_for_referees == FALSE)
    {
      RefreshScoreData ();
      RefreshDashBoard ();
    }

    if (print_for_referees == FALSE)
    {
      for (guint i = 0; i < GetNbPlayers (); i++)
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
  void Pool::DrawPage (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr)
  {
    GooCanvas *canvas = CreateCanvas ();

    {
      gboolean for_referees = (gboolean) GPOINTER_TO_INT(g_object_get_data (G_OBJECT (operation), "print_for_referees"));

      Draw (canvas,
            for_referees,
            for_referees);
    }

    g_object_set_data (G_OBJECT (operation), "operation_canvas", (void *) canvas);
    CanvasModule::DrawPage (operation,
                            context,
                            page_nr);
    Wipe ();

    gtk_widget_destroy (GTK_WIDGET (canvas));
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
                          pool->_single_owner,
                          pool->_rand_seed,
                          pool->GetDataOwner (),
                          WITH_CALCULUS | WITH_RANDOM);
  }

  // --------------------------------------------------------------------------------
  gint Pool::ComparePlayer (Player   *A,
                            Player   *B,
                            Object   *data_owner,
                            guint32   rand_seed,
                            Object   *main_data_owner,
                            guint     comparison_policy)
  {
    if (B == NULL)
    {
      return 1;
    }
    else if (A == NULL)
    {
      return -1;
    }
    else if (comparison_policy & WITH_CALCULUS)
    {
      guint   pool_nr_A;
      guint   pool_nr_B;
      guint   ratio_A;
      guint   ratio_B;
      gint    average_A;
      gint    average_B;
      guint   HS_A;
      guint   HS_B;
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
      attr_id._owner = main_data_owner;
      if (A->GetAttribute (&attr_id) && B->GetAttribute (&attr_id))
      {
        gchar *status_A = A->GetAttribute (&attr_id)->GetStrValue ();
        gchar *status_B = B->GetAttribute (&attr_id)->GetStrValue ();

        if ((status_A[0] == 'Q') && (status_A[0] != status_B[0]))
        {
          return -1;
        }
        if ((status_B[0] == 'Q') && (status_B[0] != status_A[0]))
        {
          return 1;
        }

        if ((status_A[0] == 'E') && (status_A[0] != status_B[0]))
        {
          return 1;
        }
        if ((status_B[0] == 'E') && (status_B[0] != status_A[0]))
        {
          return -1;
        }

        if ((status_A[0] == 'A') && (status_A[0] != status_B[0]))
        {
          return 1;
        }
        if ((status_B[0] == 'A') && (status_B[0] != status_A[0]))
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
      _status_item = NULL;
    }

    if (_status_pixbuf)
    {
      g_object_unref (_status_pixbuf);
      _status_pixbuf = NULL;
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
      GSList *current = _referee_list;

      while (current)
      {
        Player              *referee = (Player *) current->data;
        Player::AttributeId  connection_attr_id  ("connection");
        Attribute           *connection_attr = referee->GetAttribute (&connection_attr_id);

        if (connection_attr)
        {
          const gchar *connection_status = connection_attr->GetStrValue ();

          if (   (strcmp (connection_status, "Broken") == 0)
              || (strcmp (connection_status, "Waiting") == 0))
          {
            _status_pixbuf = connection_attr->GetPixbuf ();
            break;
          }
        }

        current = g_slist_next (current);
      }

      if (_status_pixbuf == NULL)
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

    if (_status_cbk)
    {
      _status_cbk (this,
                   _status_cbk_data);
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::RefreshScoreData ()
  {
    GSList *ranking    = NULL;
    guint   nb_players = GetNbPlayers ();

    _is_over   = TRUE;
    _has_error = FALSE;

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

      player_a->SetData (GetDataOwner (), "Victories", (void *) victories);
      player_a->SetData (GetDataOwner (), "HR", (void *) hits_received);

      if ((GetNbPlayers () - _nb_drop) > 1)
      {
        RefreshAttribute (player_a,
                          "victories_ratio",
                          (victories*1000 / (GetNbPlayers () - _nb_drop -1)),
                          AVERAGE);
      }
      else
      {
        RefreshAttribute (player_a,
                          "victories_ratio",
                          0,
                          AVERAGE);
      }

      RefreshAttribute (player_a,
                        "indice",
                        hits_scored+hits_received,
                        SUM);

      RefreshAttribute (player_a,
                        "HS",
                        hits_scored,
                        SUM);

      ranking = g_slist_append (ranking,
                                player_a);
    }

    ranking = g_slist_sort_with_data (ranking,
                                      (GCompareDataFunc) _ComparePlayer,
                                      (void *) this);

    {
      Player::AttributeId  victories_ratio_id ("victories_ratio", _single_owner);
      Player::AttributeId  indice_id          ("indice", _single_owner);
      Player::AttributeId  HS_id              ("HS", _single_owner);
      GSList *current_player  = ranking;
      Player *previous_player = NULL;
      guint   previous_rank   = 0;

      for (guint p = 1; current_player != NULL; p++)
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
  GSList *Pool::GetCurrentClassification ()
  {
    GSList *result = g_slist_copy (_sorted_fencer_list);

    result = g_slist_sort_with_data (result,
                                     (GCompareDataFunc) _ComparePlayer,
                                     (void *) this);
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
                               CombinedOperation  operation)
  {
    Player::AttributeId  single_attr_id  (name, _single_owner);
    Player::AttributeId  combined_attr_id (name, GetDataOwner ());

    player->SetAttributeValue (&single_attr_id,
                               value);

    if (_combined_source_owner == NULL)
    {
      player->SetAttributeValue (&combined_attr_id,
                                 value);
    }
    else
    {
      Player::AttributeId  source_attr_id (name, _combined_source_owner);
      Attribute           *source_attr = player->GetAttribute (&source_attr_id);

      if (source_attr)
      {
        if (operation == AVERAGE)
        {
          player->SetAttributeValue (&combined_attr_id,
                                     (source_attr->GetUIntValue () + value) / 2);
        }
        else
        {
          player->SetAttributeValue (&combined_attr_id,
                                     source_attr->GetUIntValue () + value);
        }
      }
      else
      {
        g_print (RED "source_attr == NULL\n" ESC);
      }
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
      Player::AttributeId attr_id ("", _single_owner);

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
          Player::AttributeId attr_id ("status", GetDataOwner ());

          attr = player->GetAttribute (&attr_id);
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
                                  AttributeDesc::DISCRETE_XML_IMAGE_str, &code,
                                  -1);
              if (strcmp (text, code) == 0)
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
    GSList *current = _match_list;

    while (current)
    {
      Match *match = (Match *) current->data;

      if (   match->HasFencer (A)
          && match->HasFencer (B))
      {
        return match;
      }
      current = g_slist_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean Pool::OnHttpPost (const gchar **url,
                             const gchar *data)
  {
    if (*url && data)
    {
      guint match_index = atoi  (*url);

      if (match_index > 0)
      {
        Match *match = GetMatch (match_index-1);

        if (match)
        {
          xmlDoc *doc = xmlReadMemory (data,
                                       strlen (data),
                                       "noname.xml",
                                       NULL,
                                       0);

          if (doc)
          {
            xmlXPathInit ();

            {
              xmlXPathContext *xml_context = xmlXPathNewContext (doc);
              xmlXPathObject  *xml_object;
              xmlNodeSet      *xml_nodeset;

              xml_object = xmlXPathEval (BAD_CAST "/Arbitre/Match/*", xml_context);
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
          return TRUE;
        }
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  Match *Pool::GetMatch (guint i)
  {
    guint a_id;
    guint b_id;

    if (_match_order->GetPlayerPair (i,
                                     &a_id,
                                     &b_id))
    {
      Player *a = (Player *) g_slist_nth_data (_sorted_fencer_list, a_id-1);
      Player *b = (Player *) g_slist_nth_data (_sorted_fencer_list, b_id-1);

      return GetMatch (a,
                       b);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gint Pool::CompareMatch (Match *a,
                           Match *b,
                           Pool  *pool)
  {
    return g_slist_index (pool->_sorted_fencer_list, a->GetOpponent (0)) - g_slist_index (pool->_sorted_fencer_list, b->GetOpponent (0));
  }

  // --------------------------------------------------------------------------------
  void Pool::Save (xmlTextWriter *xml_writer)
  {
    GSList *working_list;

    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST "Poule");
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "ID",
                                       "%d", _number);

    if (_sorted_fencer_list)
    {
      working_list = _sorted_fencer_list;
    }
    else
    {
      working_list = _fencer_list;
    }

    {
      GSList              *current = working_list;
      Player::AttributeId  attr_id ("", _single_owner);
      Attribute           *attr;

      for (guint i = 0; current != NULL; i++)
      {
        Player *player;

        player = (Player *) current->data;

        xmlTextWriterStartElement (xml_writer,
                                   BAD_CAST player->GetXmlTag ());
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "REF",
                                           "%d", player->GetRef ());
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "NoDansLaPoule",
                                           "%d", g_slist_index (working_list,
                                                                player) + 1);
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "NbVictoires",
                                           "%d", player->GetIntData (GetDataOwner (),
                                                                     "Victories"));
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "NbMatches",
                                           "%d", GetNbPlayers ()-1);
        attr_id._name = (gchar *) "HS";
        attr = player->GetAttribute (&attr_id);
        if (attr)
        {
          gint  indice;
          guint HS;

          HS = player->GetAttribute (&attr_id)->GetUIntValue ();
          xmlTextWriterWriteFormatAttribute (xml_writer,
                                             BAD_CAST "TD",
                                             "%d", HS);
          attr_id._name = (gchar *) "indice";
          indice = player->GetAttribute (&attr_id)->GetUIntValue ();
          xmlTextWriterWriteFormatAttribute (xml_writer,
                                             BAD_CAST "TR",
                                             "%d", HS - indice);
        }
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "RangPoule",
                                           "%d", player->GetIntData (this,
                                                                     "Rank"));
        xmlTextWriterEndElement (xml_writer);
        current = g_slist_next (current);
      }
    }

    {
      GSList *current = _referee_list;

      while (current)
      {
        Player *referee = (Player *) current->data;

        xmlTextWriterStartElement (xml_writer,
                                   BAD_CAST referee->GetXmlTag ());
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "REF",
                                           "%d", referee->GetRef ());
        xmlTextWriterEndElement (xml_writer);

        current = g_slist_next (current);
      }
    }

    {
      GSList *current = _match_list;

      while (current)
      {
        Match *match = (Match *) current->data;

        match->Save (xml_writer);
        current = g_slist_next (current);
      }
    }

    xmlTextWriterEndElement (xml_writer);
  }

  // --------------------------------------------------------------------------------
  void Pool::Load (xmlNode *xml_node,
                   GSList  *player_list)
  {
    if (xml_node == NULL)
    {
      return;
    }

    _is_over = TRUE;

    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        static xmlNode *A = NULL;
        static xmlNode *B = NULL;

        if (strcmp ((char *) n->name, "Match") == 0)
        {
          A = NULL;
          B = NULL;
        }
        else if (strcmp ((char *) n->name, _xml_player_tag) == 0)
        {
          if (A == NULL)
          {
            A = n;
          }
          else
          {
            gchar  *attr;
            Player *player_A = NULL;
            Player *player_B = NULL;
            Match  *match;

            B = n;
            {
              GSList *node;

              attr = (gchar *) xmlGetProp (A, BAD_CAST "REF");
              node = g_slist_find_custom (player_list,
                                          (void *) strtol (attr, NULL, 10),
                                          (GCompareFunc) Player::CompareWithRef);
              if (node)
              {
                player_A = (Player *) node->data;
              }
              xmlFree (attr);

              attr = (gchar *) xmlGetProp (B, BAD_CAST "REF");
              node = g_slist_find_custom (player_list,
                                          (void *) strtol (attr, NULL, 10),
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
                Player  *p = player_B;
                xmlNode *n = B;

                player_B = player_A;
                player_A = p;

                B = A;
                A = n;
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
  void Pool::CleanScores ()
  {
    GSList *current = _match_list;

    while (current)
    {
      Match *match = (Match *) current->data;

      match->CleanScore ();
      current = g_slist_next (current);
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
                          pool->GetDataOwner (),
                          pool->_rand_seed,
                          pool->GetDataOwner (),
                          WITH_RANDOM);
  }

  // --------------------------------------------------------------------------------
  void Pool::SortPlayers ()
  {
    g_slist_free (_sorted_fencer_list);

    _sorted_fencer_list = g_slist_copy (_fencer_list);

    _sorted_fencer_list = g_slist_sort_with_data (_sorted_fencer_list,
                                                  (GCompareDataFunc) _ComparePlayerWithFullRandom,
                                                  (void *) this);
  }

  // --------------------------------------------------------------------------------
  void Pool::DeleteMatchs ()
  {
    CleanScores ();

    {
      GSList *current = _match_list;

      while (current)
      {
        Match *match = (Match *) current->data;

        Object::TryToRelease (match);
        current = g_slist_next (current);
      }

      g_slist_free (_match_list);
      _match_list = NULL;
    }

    {
      GSList *current = _fencer_list;

      while (current)
      {
        Player              *player         = (Player *) current->data;
        Player::AttributeId  status_attr_id = Player::AttributeId ("status", GetDataOwner ());

        player->SetAttributeValue (&status_attr_id,
                                   "Q");
        current = g_slist_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Pool::DropPlayer (Player *player,
                         gchar  *reason)
  {
    Player::AttributeId  status_attr_id = Player::AttributeId ("status", GetDataOwner ());
    Attribute           *status_attr    = player->GetAttribute ( &status_attr_id);

    if (status_attr)
    {
      gchar *status = status_attr->GetStrValue ();

      if (status && (*status == 'Q'))
      {
        _nb_drop++;
      }
    }

    player->SetAttributeValue (&status_attr_id,
                               reason);

    for (guint i = 0; i < GetNbPlayers (); i++)
    {
      Player *opponent = GetPlayer (i, _sorted_fencer_list);

      if (opponent != player)
      {
        Match *match = GetMatch (opponent, player);

        match->DropFencer (player,
                           reason);
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

      if (status && (*status != 'Q'))
      {
        dropped = TRUE;
      }
    }

    player->SetAttributeValue (&status_attr_id,
                               "Q");
    if (dropped)
    {
      _nb_drop--;

      for (guint i = 0; i < GetNbPlayers (); i++)
      {
        Player *opponent = GetPlayer (i, _sorted_fencer_list);

        if (opponent != player)
        {
          Match *match = GetMatch (opponent, player);

          match->RestoreFencer (player);
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
                        AttributeDesc::DISCRETE_XML_IMAGE_str, &code,
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
