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

#include "match.hpp"
#include "pool_match_order.hpp"
#include "pool.hpp"

// --------------------------------------------------------------------------------
Pool::Pool (Data  *max_score,
            guint  number)
  : CanvasModule ("pool.glade",
                  "canvas_scrolled_window")
{
  _number      = number;
  _player_list = NULL;
  _match_list  = NULL;
  _rand_seed   = 0;
  _is_over     = FALSE;
  _has_error   = FALSE;
  _title_table = NULL;
  _status_item = NULL;
  _locked      = FALSE;
  _max_score   = max_score;

  _status_cbk_data = NULL;
  _status_cbk      = NULL;

  _score_collector = NULL;

  _name = g_strdup_printf ("Poule No %02d", _number);
}

// --------------------------------------------------------------------------------
Pool::~Pool ()
{
  Wipe ();

  g_free (_name);

  g_slist_free (_player_list);

  for (guint i = 0; i < g_slist_length (_match_list); i++)
  {
    Match *match;

    match = (Match *) g_slist_nth_data (_match_list, i);
    Object::TryToRelease (match);
  }
  g_slist_free (_match_list);

  Object::TryToRelease (_score_collector);
}

// --------------------------------------------------------------------------------
void Pool::Wipe ()
{
  _title_table = NULL;

  CanvasModule::Wipe ();
}

// --------------------------------------------------------------------------------
void Pool::Stuff ()
{
  for (guint i = 0; i < g_slist_length (_match_list); i++)
  {
    Match  *match;
    Player *A;
    Player *B;
    gint    score;

    match = (Match *) g_slist_nth_data (_match_list, i);
    A     = match->GetPlayerA ();
    B     = match->GetPlayerB ();
    score = g_random_int_range (0,
                                _max_score->_value);

    if (g_random_boolean ())
    {
      match->SetScore (A, _max_score->_value);
      match->SetScore (B, score);
    }
    else
    {
      match->SetScore (A, score);
      match->SetScore (B, _max_score->_value);
    }
  }

  RefreshScoreData ();
}

// --------------------------------------------------------------------------------
void Pool::SetStatusCbk (StatusCbk  cbk,
                         void      *data)
{
  _status_cbk_data = data;
  _status_cbk      = cbk;

  if (_status_cbk)
  {
    _status_cbk (this,
                 _status_cbk_data);
  }
}

// --------------------------------------------------------------------------------
void Pool::SetRandSeed (guint32 seed)
{
  _rand_seed = seed;
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
void Pool::AddPlayer (Player *player,
                      Object *rank_owner)
{
  if (player == NULL)
  {
    return;
  }

  if (   (_player_list == NULL)
         || (g_slist_find (_player_list,
                           player) == NULL))
  {
    Player::AttributeId attr_id ("rank",
                                 rank_owner);

    for (guint i = 0; i < GetNbPlayers (); i++)
    {
      Player *current_player;
      Match  *match;

      current_player = GetPlayer (i);
      match = new Match (player,
                         current_player,
                         _max_score);

      _match_list = g_slist_append (_match_list,
                                    match);
    }

    _player_list = g_slist_insert_sorted_with_data (_player_list,
                                                    player,
                                                    (GCompareDataFunc) Player::Compare,
                                                    &attr_id);
  }
}

// --------------------------------------------------------------------------------
void Pool::RemovePlayer (Player *player)
{
  if (g_slist_find (_player_list,
                    player))
  {
    _player_list = g_slist_remove (_player_list,
                                   player);

    {
      GSList *node = _match_list;

      while (node)
      {
        GSList  *next_node;
        Match   *match;

        next_node = g_slist_next (node);

        match = (Match *) node->data;
        if (match->HasPlayer (player))
        {
          _match_list = g_slist_delete_link (_match_list,
                                             node);
          match->Release ();
        }

        node = next_node;
      }
    }
  }
}

// --------------------------------------------------------------------------------
guint Pool::GetNbPlayers ()
{
  return g_slist_length (_player_list);
}

// --------------------------------------------------------------------------------
Player *Pool::GetPlayer (guint i)
{
  return (Player *) g_slist_nth_data (_player_list,
                                      i);
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
GString *Pool::GetPlayerImage (Player *player)
{
  GString *image         = g_string_new ("");
  GSList  *selected_attr = NULL;

  if (_filter)
  {
    selected_attr = _filter->GetSelectedAttrList ();
  }

  for (guint a = 0; a < g_slist_length (selected_attr); a++)
  {
    AttributeDesc       *attr_desc;
    Attribute           *attr;
    Player::AttributeId *attr_id;

    attr_desc = (AttributeDesc *) g_slist_nth_data (selected_attr,
                                                    a);
    attr_id = new Player::AttributeId (attr_desc->_xml_name);
    attr = player->GetAttribute (attr_id);
    attr_id->Release ();

    if (attr)
    {
      image = g_string_append (image,
                               attr->GetUserImage ());
      image = g_string_append (image,
                               "  ");
    }
  }

  return image;
}

// --------------------------------------------------------------------------------
void Pool::OnPlugged ()
{
  _title_table = NULL;
  _status_item = NULL;

  CanvasModule::OnPlugged ();

  if (_score_collector)
  {
    for (guint i = 0; i < g_slist_length (_match_list); i++)
    {
      Match *match;

      match = (Match *) g_slist_nth_data (_match_list, i);
      _score_collector->RemoveCollectingPoints (match);
    }

    _score_collector->Release ();
  }

  _score_collector = new ScoreCollector (this,
                                         (ScoreCollector::OnNewScore_cbk) &Pool::OnNewScore);

  {
    const guint    cell_w     = 40;
    const guint    cell_h     = 40;
    guint          nb_players = GetNbPlayers ();
    GooCanvasItem *root_item  = GetRootItem ();
    GooCanvasItem *grid_group = goo_canvas_group_new (root_item, NULL);

    // Grid
    {
      // Border
      {
        goo_canvas_rect_new (grid_group,
                             0, 0,
                             nb_players * cell_w, nb_players * cell_h,
                             "line-width", 5.0,
                             NULL);
      }

      // Cells
      {
        GooCanvasItem *previous_goo_rect = NULL;

        for (guint i = 0; i < nb_players; i++)
        {
          Player *A;

          A = GetPlayer (i);

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
              Player *B = GetPlayer (j);
              Match  *match;

              match = GetMatch (A,
                                B);

              // Text
              {
                Score *score       = match->GetScore (A);
                gchar *score_image = score->GetImage ();

                score_text = goo_canvas_text_new (grid_group,
                                                  score_image,
                                                  x + cell_w / 2,
                                                  y + cell_h / 2,
                                                  -1,
                                                  GTK_ANCHOR_CENTER,
                                                  "font", "Sans bold 18px",
                                                  NULL);
                g_free (score_image);
              }

              if (_locked == FALSE)
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
        gint      x, y;
        GString  *image;

        x = - 5;
        y = cell_h / 2 + i * cell_h;
        image = GetPlayerImage (GetPlayer (i));

        {
          gchar *index = g_strdup_printf (" %d", i+1);

          image = g_string_append (image,
                                   index);
          g_free (index);
        }

        goo_canvas_text_new (grid_group,
                             image->str,
                             x, y, -1,
                             GTK_ANCHOR_EAST,
                             "font", "Sans 18px",
                             NULL);
        g_string_free (image,
                       TRUE);
      }

      // Players (horizontally)
      for (guint i = 0; i < nb_players; i++)
      {
        GooCanvasItem *goo_text;
        gint           x, y;
        gchar         *text;

        x = cell_w / 2 + i * cell_w;;
        y = - 13;
        text = g_strdup_printf ("%d", i+1);

        goo_text = goo_canvas_text_new (grid_group,
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
      GooCanvasItem *dashboard_group = goo_canvas_group_new (root_item,
                                                             NULL);

      {
        GooCanvasItem *goo_text;
        gint           x, y;

        x = cell_w/2;
        y = - 10;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        "Victoires",
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        "Vict./Matchs",
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        "T. données",
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        "T. reçues",
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        "Indice",
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        "Place",
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;
      }

      for (guint i = 0; i < nb_players; i++)
      {
        Player *player;

        player = GetPlayer (i);

        {
          GooCanvasItem *goo_text;
          gint           x, y;

          x = cell_w/2;
          y = cell_h/2 + i*cell_h;

          {
            goo_text = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            player->SetData (GetDataOwner (), "VictoriesItem",  goo_text);
            x += cell_w;

            goo_text = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            player->SetData (GetDataOwner (), "VictoriesRatioItem",  goo_text);
            x += cell_w;

            goo_text = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            player->SetData (GetDataOwner (), "HSItem",  goo_text);
            x += cell_w;

            goo_text = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            player->SetData (GetDataOwner (), "HRItem",  goo_text);
            x += cell_w;

            goo_text = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            player->SetData (GetDataOwner (), "IndiceItem",  goo_text);
            x += cell_w;

            goo_text = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            player->SetData (GetDataOwner (), "RankItem",  goo_text);
            x += cell_w;
          }
        }
      }

      {
        GooCanvasBounds grid_bounds;
        GooCanvasBounds dashboard_bounds;

        goo_canvas_item_get_bounds (grid_group,
                                    &grid_bounds);
        goo_canvas_item_get_bounds (dashboard_group,
                                    &dashboard_bounds);

        goo_canvas_item_translate (GOO_CANVAS_ITEM (dashboard_group),
                                   grid_bounds.x2 - dashboard_bounds.x1 + cell_w,
                                   0);
      }
    }

    // Matches
    {
      GooCanvasBounds  bounds;
      GooCanvasItem   *match_main_table;
      GooCanvasItem   *text_item;
      const guint      nb_column = 3;

      goo_canvas_item_get_bounds (root_item,
                                  &bounds);

      match_main_table = goo_canvas_table_new (root_item,
                                               "row-spacing", (gdouble) cell_h/2,
                                               "column-spacing", (gdouble) cell_w/2, NULL);

      for (guint m = 0; m < g_slist_length (_match_list); m++)
      {
        Match         *match;
        GString       *image;
        GooCanvasItem *match_table;

        match_table = goo_canvas_table_new (match_main_table, NULL);

        match = GetMatch (m);

        {
          gchar *text = g_strdup_printf ("%d", m+1);

          text_item = Canvas::PutTextInTable (match_main_table,
                                              text,
                                              m/nb_column,
                                              m%nb_column + 2*(m%nb_column));
          g_object_set (G_OBJECT (text_item),
                        "font", "Sans bold 18px",
                        NULL);
          Canvas::SetTableItemAttribute (text_item, "y-align", 0.5);
        }

        {
          image = GetPlayerImage (match->GetPlayerA ());
          text_item = Canvas::PutTextInTable (match_table,
                                              image->str,
                                              0,
                                              0);
          g_string_free (image,
                         TRUE);
          for (guint i = 0; i < _max_score->_value; i++)
          {
            GooCanvasItem *rect = goo_canvas_rect_new (match_table,
                                                       0.0, 0.0,
                                                       10.0, 10.0,
                                                       "stroke-color", "grey",
                                                       "line-width", 1.0,
                                                       NULL);
            Canvas::PutInTable (match_table,
                                rect,
                                0,
                                i+1);
            Canvas::SetTableItemAttribute (rect, "y-align", 0.5);
          }
        }

        {
          image = GetPlayerImage (match->GetPlayerB ());
          text_item = Canvas::PutTextInTable (match_table,
                                              image->str,
                                              1,
                                              0);
          g_string_free (image,
                         TRUE);

          for (guint i = 0; i < _max_score->_value; i++)
          {
            GooCanvasItem *rect = goo_canvas_rect_new (match_table,
                                                       0.0, 0.0,
                                                       10.0, 10.0,
                                                       "stroke-color", "grey",
                                                       "line-width", 1.0,
                                                       NULL);

            Canvas::PutInTable (match_table,
                                rect,
                                1,
                                i+1);
            Canvas::SetTableItemAttribute (rect, "y-align", 0.5);
          }
        }
        Canvas::PutInTable (match_main_table,
                            match_table,
                            m/nb_column,
                            m%nb_column + 2*(m%nb_column) + 1);
        Canvas::SetTableItemAttribute (match_table, "x-expand", 1U);
        Canvas::SetTableItemAttribute (match_table, "x-fill", 1U);
        Canvas::SetTableItemAttribute (match_table, "x-shrink", 1U);
      }
      goo_canvas_item_translate (match_main_table,
                                 bounds.x1,
                                 bounds.y2 + cell_h);
    }

    // Name
    {
      GooCanvasBounds  bounds;
      GooCanvasItem   *text_item;

      _title_table = goo_canvas_table_new (root_item, NULL);
      text_item = Canvas::PutTextInTable (_title_table,
                                          _name,
                                          0,
                                          1);
      g_object_set (G_OBJECT (text_item),
                    "font", "Sans bold 18px",
                    NULL);

      goo_canvas_item_get_bounds (root_item,
                                  &bounds);

      goo_canvas_item_translate (_title_table,
                                 bounds.x1,
                                 bounds.y1);
    }
  }

  RefreshScoreData ();
  RefreshDashBoard ();
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
                        FALSE);
}

// --------------------------------------------------------------------------------
gint Pool::ComparePlayer (Player   *A,
                          Player   *B,
                          Object   *data_owner,
                          guint32   rand_seed,
                          gboolean  with_full_random)
{
  if (with_full_random == FALSE)
  {
    guint ratio_A;
    guint ratio_B;
    gint  average_A;
    gint  average_B;
    guint HS_A;
    guint HS_B;
    Player::AttributeId attr_id ("", data_owner);

    attr_id._name = "victories_ratio";
    ratio_A = (guint) A->GetAttribute (&attr_id)->GetValue ();

    attr_id._name = "victories_ratio";
    ratio_B = (guint) B->GetAttribute (&attr_id)->GetValue ();

    attr_id._name = "indice";
    average_A = (gint)  A->GetAttribute (&attr_id)->GetValue ();

    attr_id._name = "indice";
    average_B = (gint)  B->GetAttribute (&attr_id)->GetValue ();

    attr_id._name = "HS";
    HS_A = (guint) A->GetAttribute (&attr_id)->GetValue ();

    attr_id._name = "HS";
    HS_B = (guint) B->GetAttribute (&attr_id)->GetValue ();

    if (ratio_B != ratio_A)
    {
      return ratio_B - ratio_A;
    }
    if (average_B != average_A)
    {
      return average_B - average_A;
    }
    else if (HS_B != HS_A)
    {
      return HS_B - HS_A;
    }
  }

  // Return always the same random value for the given players
  // to avoid human manipulations. Without that, filling in the
  // same result twice could modify the ranking.
  {
    guint         ref_A  = A->GetRef ();
    guint         ref_B  = B->GetRef ();
    const guint32 seed[] = {MIN (ref_A, ref_B), MAX (ref_A, ref_B), rand_seed};
    GRand        *rand;
    gint          result;

    rand = g_rand_new_with_seed_array (seed,
                                       sizeof (seed) / sizeof (guint32));
    result = (gint) g_rand_int (rand);
    g_rand_free (rand);

    if (ref_A > ref_B)
    {
      return -result;
    }
    else
    {
      return result;
    }
  }
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
}

// --------------------------------------------------------------------------------
void Pool::UnLock ()
{
  _locked = FALSE;

  if (_score_collector)
  {
    _score_collector->UnLock ();
  }
}

// --------------------------------------------------------------------------------
void Pool::RefreshScoreData ()
{
  GSList             *ranking    = NULL;
  guint               nb_players = GetNbPlayers ();
  Player::AttributeId attr_id ("", GetDataOwner ());

  _is_over   = TRUE;
  _has_error = FALSE;

  for (guint a = 0; a < nb_players; a++)
  {
    Player *player_a;
    guint   victories     = 0;
    guint   hits_scored   = 0;
    gint    hits_received = 0;

    player_a = GetPlayer (a);

    for (guint b = 0; b < nb_players; b++)
    {
      if (a != b)
      {
        Player *player_b = GetPlayer (b);
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
          if (score_a->Get () > score_b->Get ())
          {
            victories++;
          }
        }
        else
        {
          _is_over = FALSE;
        }

        if (   (score_a->IsValid () == false)
            || (score_b->IsValid () == false)
            || (score_a->IsConsistentWith (score_b) == false))
        {
          _is_over   = FALSE;
          _has_error = TRUE;
        }
      }
    }

    player_a->SetData (GetDataOwner (), "Victories", (void *) victories);
    player_a->SetData (GetDataOwner (), "HR", (void *) hits_received);

    attr_id._name = "victories_ratio";
    player_a->SetAttributeValue (&attr_id, (victories*1000 / (GetNbPlayers ()-1)));
    attr_id._name = "indice";
    player_a->SetAttributeValue (&attr_id, hits_scored+hits_received);
    attr_id._name = "HS";
    player_a->SetAttributeValue (&attr_id, hits_scored);

    ranking = g_slist_append (ranking,
                              player_a);
  }

  ranking = g_slist_sort_with_data (ranking,
                                    (GCompareDataFunc) _ComparePlayer,
                                    (void *) this);

  for (guint p = 0; p < nb_players; p++)
  {
    Player *player;

    player = (Player *) g_slist_nth_data (ranking,
                                          p);
    if (player)
    {
      player->SetData (this, "Rank", (void *) (p+1));
    }
  }

  g_slist_free (ranking);

  if (_status_cbk)
  {
    _status_cbk (this,
                 _status_cbk_data);
  }

  if (_title_table)
  {
    if (_status_item)
    {
      goo_canvas_item_remove (_status_item);
      _status_item = NULL;
    }

    if (_is_over)
    {
      _status_item = Canvas::PutStockIconInTable (_title_table,
                                                  GTK_STOCK_APPLY,
                                                  0, 0);
    }
    else if (_has_error)
    {
      _status_item = Canvas::PutStockIconInTable (_title_table,
                                                  GTK_STOCK_DIALOG_WARNING,
                                                  0, 0);
    }
    else
    {
      _status_item = Canvas::PutStockIconInTable (_title_table,
                                                  GTK_STOCK_EXECUTE,
                                                  0, 0);
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
  guint               nb_players = GetNbPlayers ();
  Player::AttributeId attr_id ("", GetDataOwner ());

  for (guint p = 0; p < nb_players; p++)
  {
    Player        *player;
    GooCanvasItem *goo_text;
    gchar         *text;
    Attribute     *attr;
    gint           value;

    player = GetPlayer (p);

    attr_id._name = "victories_ratio";
    attr = player->GetAttribute (&attr_id);
    goo_text = GOO_CANVAS_ITEM (player->GetData (GetDataOwner (), "VictoriesRatioItem"));
    text = g_strdup_printf ("%0.3f", (gdouble) ((guint) attr->GetValue ()) / 1000.0);
    g_object_set (goo_text,
                  "text",
                  text, NULL);
    g_free (text);

    value = (gint) player->GetData (GetDataOwner (), "Victories");
    goo_text = GOO_CANVAS_ITEM (player->GetData (GetDataOwner (), "VictoriesItem"));
    text = g_strdup_printf ("%d", value);
    g_object_set (goo_text,
                  "text",
                  text, NULL);
    g_free (text);

    attr_id._name = "HS";
    attr = player->GetAttribute (&attr_id);
    goo_text = GOO_CANVAS_ITEM (player->GetData (GetDataOwner (), "HSItem"));
    text = g_strdup_printf ("%d", (guint) attr->GetValue ());
    g_object_set (goo_text,
                  "text",
                  text, NULL);
    g_free (text);

    value = (gint) player->GetData (GetDataOwner (), "HR");
    goo_text = GOO_CANVAS_ITEM (player->GetData (GetDataOwner (), "HRItem"));
    text = g_strdup_printf ("%d", -value);
    g_object_set (goo_text,
                  "text",
                  text, NULL);
    g_free (text);

    attr_id._name = "indice";
    attr = player->GetAttribute (&attr_id);
    goo_text = GOO_CANVAS_ITEM (player->GetData (GetDataOwner (), "IndiceItem"));
    text = g_strdup_printf ("%+d", (gint) attr->GetValue ());
    g_object_set (goo_text,
                  "text",
                  text, NULL);
    g_free (text);

    value = (gint) player->GetData (this, "Rank");
    goo_text = GOO_CANVAS_ITEM (player->GetData (GetDataOwner (), "RankItem"));
    text = g_strdup_printf ("%d", value);
    g_object_set (goo_text,
                  "text",
                  text, NULL);
    g_free (text);
  }
}

// --------------------------------------------------------------------------------
Match *Pool::GetMatch (Player *A,
                       Player *B)
{
  for (guint i = 0; i < g_slist_length (_match_list); i++)
  {
    Match *match;

    match = (Match *) g_slist_nth_data (_match_list, i);
    if (   match->HasPlayer (A)
        && match->HasPlayer (B))
    {
      return match;
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
Match *Pool::GetMatch (guint i)
{
  PoolMatchOrder::PlayerPair *pair = PoolMatchOrder::GetPlayerPair (GetNbPlayers ());

  if (pair)
  {
    Player *a = (Player *) g_slist_nth_data (_player_list, pair[i]._a-1);
    Player *b = (Player *) g_slist_nth_data (_player_list, pair[i]._b-1);

    return GetMatch (a,
                     b);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Pool::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "match_list");

  for (guint i = 0; i < g_slist_length (_match_list); i++)
  {
    Match *match;

    match = (Match *) g_slist_nth_data (_match_list, i);
    if (match)
    {
      match->Save (xml_writer);
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
      if (strcmp ((char *) n->name, "match") == 0)
      {
        gchar  *attr;
        Match  *match;
        Player *A      = NULL;
        Player *B      = NULL;

        attr = (gchar *) xmlGetProp (n, BAD_CAST "player_A");
        if (attr)
        {
          GSList *node = g_slist_find_custom (player_list,
                                              (void *) strtol (attr, NULL, 10),
                                              (GCompareFunc) Player::CompareWithRef);
          if (node)
          {
            A = (Player *) node->data;
          }
        }

        attr = (gchar *) xmlGetProp (n, BAD_CAST "player_B");
        if (attr)
        {
          GSList *node = g_slist_find_custom (player_list,
                                              (void *) strtol (attr, NULL, 10),
                                              (GCompareFunc) Player::CompareWithRef);
          if (node)
          {
            B = (Player *) node->data;
          }
        }

        match = GetMatch (A, B);

        if (match)
        {
          attr = (gchar *) xmlGetProp (n, BAD_CAST "score_A");
          if (attr)
          {
            match->SetScore (A, atoi (attr));
          }

          attr = (gchar *) xmlGetProp (n, BAD_CAST "score_B");
          if (attr)
          {
            match->SetScore (B, atoi (attr));
          }

          if (   (match->PlayerHasScore (A) == FALSE)
                 || (match->PlayerHasScore (B) == FALSE))
          {
            _is_over = FALSE;
          }

          {
            Score *score_A = match->GetScore (A);;
            Score *score_B = match->GetScore (B);;

            if (   (score_A->IsValid () == false)
                || (score_B->IsValid () == false)
                || (score_A->IsConsistentWith (score_B) == false))
            {
              _has_error = TRUE;
              _is_over   = FALSE;
            }
          }
        }
      }

      Load (n->children,
            player_list);
    }
  }
}

// --------------------------------------------------------------------------------
void Pool::ResetMatches ()
{
  for (guint i = 0; i < g_slist_length (_match_list); i++)
  {
    Match *match;

    match = (Match *) g_slist_nth_data (_match_list, i);
    match->CleanScore ();
  }
  _is_over = FALSE;

  RefreshScoreData ();
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
                        TRUE);
}

// --------------------------------------------------------------------------------
void  Pool::SortPlayers ()
{
  _player_list = g_slist_sort_with_data (_player_list,
                                         (GCompareDataFunc) _ComparePlayerWithFullRandom,
                                         (void *) this);
}
