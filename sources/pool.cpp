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
  _number       = number;
  _player_list  = NULL;
  _match_list   = NULL;
  _is_over      = FALSE;
  _has_error    = FALSE;
  _title_table  = NULL;
  _status_item  = NULL;
  _locked       = FALSE;
  _max_score    = max_score;
  _display_data = NULL;
  _nb_drop      = 0;

  _status_cbk_data = NULL;
  _status_cbk      = NULL;

  _score_collector = NULL;

  _name = g_strdup_printf (gettext ("Pool #%02d"), _number);
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
  _status_item = NULL;

  for (guint i = 0; i < GetNbPlayers (); i++)
  {
    Player *player;
    GSList *current;

    player  = (Player *) g_slist_nth_data (_player_list, i);

    current = _display_data;
    while (current)
    {
      player->RemoveData (GetDataOwner (),
                          (gchar *) current->data);

      current = g_slist_next (current);
    }
    g_slist_free (_display_data);
    _display_data = NULL;
  }

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
    Player::AttributeId attr_id ("previous_stage_rank",
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

    attr_id.MakeRandomReady (_rand_seed);
    _player_list = g_slist_insert_sorted_with_data (_player_list,
                                                    player,
                                                    (GCompareDataFunc) Player::Compare,
                                                    &attr_id);
    player->SetData (rank_owner,
                     "Pool No",
                     (void *) GetNumber ());
  }
}

// --------------------------------------------------------------------------------
gint Pool::GetPosition (Player *player)
{
  return g_slist_index (_player_list,
                        player);
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
void Pool::Draw (GooCanvas *on_canvas,
                 gboolean   print_for_referees)
{
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
    const guint    cell_w     = 45;
    const guint    cell_h     = 45;
    guint          nb_players = GetNbPlayers ();
    GooCanvasItem *root_item  = goo_canvas_get_root_item (on_canvas);
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
              {
                Player *B = GetPlayer (j);
                Match  *match;

                match = GetMatch (A, B);

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

                if ((print_for_referees == FALSE) && (_locked == FALSE))
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

      if (print_for_referees)
      {
        GooCanvasBounds grid_bounds;

        goo_canvas_item_get_bounds (grid_group,
                                    &grid_bounds);

        for (guint i = 0; i < nb_players; i++)
        {
          goo_canvas_rect_new (grid_group,
                               grid_bounds.x2 + cell_w,
                               i*cell_h + 2,
                               cell_w*5,
                               cell_h-4,
                               "fill-color", "Grey85",
                               "line-width", 0.0,
                               NULL);
          goo_canvas_text_new (grid_group,
                               gettext ("Signature"),
                               grid_bounds.x2 + cell_w,
                               cell_h * (i + (1.0/2.0)),
                               -1,
                               GTK_ANCHOR_W,
                               "fill-color", "Grey95",
                               "font", "Sans bold 30.0px",
                               NULL);
        }
      }
    }

    // Dashboard
    if (print_for_referees == FALSE)
    {
      GooCanvasItem *dashboard_group = goo_canvas_group_new (root_item,
                                                             NULL);

      {
        GooCanvasItem *goo_text;
        gint           x, y;

        x = cell_w/2;
        y = - 10;

        if (GetCanvas () == on_canvas)
        {
          goo_text = goo_canvas_text_new (dashboard_group,
                                          gettext ("Status"),
                                          0.0, y, -1,
                                          GTK_ANCHOR_WEST,
                                          "font", "Sans 18px",
                                          NULL);
          goo_canvas_item_rotate (goo_text, 315, 0.0, y);
          x += cell_w;
        }

        goo_text = goo_canvas_text_new (dashboard_group,
                                        gettext ("Victories"),
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        gettext ("Vict./Matchs"),
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        gettext ("H. scored"),
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        gettext ("H. received"),
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        gettext ("Index"),
                                        x, y, -1,
                                        GTK_ANCHOR_WEST,
                                        "font", "Sans 18px",
                                        NULL);
        goo_canvas_item_rotate (goo_text, 315, x, y);
        x += cell_w;

        goo_text = goo_canvas_text_new (dashboard_group,
                                        gettext ("Place"),
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
                                              cell, "pixbuf", AttributeDesc::DISCRETE_ICON,
                                              NULL);

              g_object_set_data (G_OBJECT (w), "player",  player);
              player->SetData (this, "combo_box", w);
              goo_item = goo_canvas_widget_new (dashboard_group,
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

            goo_item = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "VictoriesItem", goo_item);
            x += cell_w;

            goo_item = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "VictoriesRatioItem", goo_item);
            x += cell_w;

            goo_item = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "HSItem", goo_item);
            x += cell_w;

            goo_item = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "HRItem", goo_item);
            x += cell_w;

            goo_item = goo_canvas_text_new (dashboard_group,
                                            ".",
                                            x, y, -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans 18px",
                                            NULL);
            SetDisplayData (player, on_canvas, "IndiceItem", goo_item);
            x += cell_w;

            goo_item = goo_canvas_text_new (dashboard_group,
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

      {
        GooCanvasBounds grid_bounds;
        GooCanvasBounds dashboard_bounds;

        goo_canvas_item_get_bounds (grid_group,
                                    &grid_bounds);
        goo_canvas_item_get_bounds (dashboard_group,
                                    &dashboard_bounds);

        goo_canvas_item_translate (GOO_CANVAS_ITEM (dashboard_group),
                                   grid_bounds.x2 - dashboard_bounds.x1 + cell_w/2,
                                   0);
      }
    }

    // Matches
    if (print_for_referees)
    {
      GooCanvasBounds  bounds;
      GooCanvasItem   *match_main_table;
      GooCanvasItem   *text_item;
      guint            nb_column;

      if (nb_players < 12)
      {
        nb_column = 3;
      }
      else
      {
        nb_column = 4;
      }

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
          image = GetPlayerImage (match->GetPlayerA ());

          text_item = Canvas::PutTextInTable (match_table,
                                              image->str,
                                              0,
                                              0);
          g_string_free (image,
                         TRUE);
          match->DrawScoreCell (match_table,
                                10,
                                0);
        }

        {
          image = GetPlayerImage (match->GetPlayerB ());
          text_item = Canvas::PutTextInTable (match_table,
                                              image->str,
                                              1,
                                              0);
          g_string_free (image,
                         TRUE);

          match->DrawScoreCell (match_table,
                                10,
                                1);
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

  if (print_for_referees == FALSE)
  {
    RefreshScoreData ();
    RefreshDashBoard ();
  }

  for (guint i = 0; i < GetNbPlayers (); i++)
  {
    Player    *player;
    GtkWidget *w;

    player = GetPlayer (i);
    w      = GTK_WIDGET (player->GetPtrData (this, "combo_box"));

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

  Draw (canvas,
        (gboolean) GPOINTER_TO_INT(g_object_get_data (G_OBJECT (operation), "print_for_referees")));

  g_object_set_data (G_OBJECT (operation), "operation_canvas", (void *) canvas);
  CanvasModule::OnDrawPage (operation,
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
        FALSE);
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
    guint   ratio_A;
    guint   ratio_B;
    gint    average_A;
    gint    average_B;
    guint   HS_A;
    guint   HS_B;
    Player::AttributeId attr_id ("", data_owner);

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
          FALSE);
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
          FALSE);
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
          if (score_a->IsTheBest ())
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

      player = GetPlayer (p);

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
                                AttributeDesc::DISCRETE_XML_IMAGE, &code,
                                -1);
            if (strcmp (text, code) == 0)
            {
              gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data),
                                             &iter);

              break;
            }
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
gint Pool::CompareMatch (Match *a,
                         Match *b,
                         Pool  *pool)
{
  return g_slist_index (pool->_player_list, a->GetPlayerA ()) - g_slist_index (pool->_player_list, b->GetPlayerA ());
}

// --------------------------------------------------------------------------------
void Pool::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "Poule");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "ID",
                                     "%d", _number);

  {
    GSList              *current = _player_list;
    Player::AttributeId  attr_id ("", _single_owner);
    Attribute           *attr;

    for (guint i = 0; current != NULL; i++)
    {
      Player *player;
      guint   HS;
      gint    indice;

      player = (Player *) current->data;

      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "Tireur");
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "REF",
                                         "%d", player->GetRef ());
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "NoDansLaPoule",
                                         "%d", i+1);
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
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST "Arbitre");
    xmlTextWriterEndElement (xml_writer);
  }

  // To avoid the GREG pool display issue
  // the order of the saved matchs must be consistent
  // with the order of the players.
  for (guint p1 = 0; p1 < GetNbPlayers (); p1++)
  {
    Player *player_1;

    player_1 = (Player *) g_slist_nth_data (_player_list, p1);
    for (guint p2 = p1+1; p2 < GetNbPlayers (); p2++)
    {
      Match  *match;
      Player *player_2;

      player_2 = (Player *) g_slist_nth_data (_player_list, p2);
      match = GetMatch (player_1, player_2);
      if (match)
      {
        match->SaveInOrder (xml_writer,
                            player_1);
      }
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
      else if (strcmp ((char *) n->name, "Tireur") == 0)
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

            attr = (gchar *) xmlGetProp (B, BAD_CAST "REF");
            node = g_slist_find_custom (player_list,
                                        (void *) strtol (attr, NULL, 10),
                                        (GCompareFunc) Player::CompareWithRef);
            if (node)
            {
              player_B = (Player *) node->data;
            }
          }

          match = GetMatch (player_A, player_B);

          if (match)
          {
            {
              gboolean is_the_best = FALSE;

              attr = (gchar *) xmlGetProp (A, BAD_CAST "Statut");
              if (attr && attr[0] == 'V')
              {
                is_the_best = TRUE;
              }

              attr = (gchar *) xmlGetProp (A, BAD_CAST "Score");

              if (attr)
              {
                match->SetScore (player_A, atoi (attr), is_the_best);
              }
            }

            {
              gboolean is_the_best = FALSE;

              attr = (gchar *) xmlGetProp (B, BAD_CAST "Statut");
              if (attr && attr[0] == 'V')
              {
                is_the_best = TRUE;
              }

              attr = (gchar *) xmlGetProp (B, BAD_CAST "Score");
              if (attr)
              {
                match->SetScore (player_B, atoi (attr), is_the_best);
              }
            }

            if (   (match->PlayerHasScore (player_A) == FALSE)
                   || (match->PlayerHasScore (player_B) == FALSE))
            {
              _is_over = FALSE;
            }

            {
              Score *score_A = match->GetScore (player_A);
              Score *score_B = match->GetScore (player_B);

              if (   (score_A->IsValid () == false)
                     || (score_B->IsValid () == false)
                     || (score_A->IsConsistentWith (score_B) == false))
              {
                _has_error = TRUE;
                _is_over   = FALSE;
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
                        pool->GetDataOwner (),
                        WITH_RANDOM);
}

// --------------------------------------------------------------------------------
void Pool::SortPlayers ()
{
  _player_list = g_slist_sort_with_data (_player_list,
                                         (GCompareDataFunc) _ComparePlayerWithFullRandom,
                                         (void *) this);

  for (guint i = 0; i < g_slist_length (_match_list); i++)
  {
    Match *match;

    match = GetMatch (i);
    match->SetNumber (i+1);
  }
}

// --------------------------------------------------------------------------------
void Pool::ResetMatches (Object *rank_owner)
{
  {
    Player::AttributeId attr_id ("previous_stage_rank",
                                 rank_owner);

    attr_id.MakeRandomReady (_rand_seed);
    _player_list = g_slist_sort_with_data (_player_list,
                                           (GCompareDataFunc) Player::Compare,
                                           &attr_id);
  }

  for (guint i = 0; i < g_slist_length (_match_list); i++)
  {
    Match *match;

    match = GetMatch (i);
    match->SetNumber (0);
  }
}

// --------------------------------------------------------------------------------
void Pool::DropPlayer (Player *player,
                       gchar  *reason)
{
  Player::AttributeId status_attr_id = Player::AttributeId ("status", GetDataOwner ());
  Attribute           *status_attr   = player->GetAttribute (&status_attr_id);
  gboolean             dropped       = TRUE;

  if (status_attr)
  {
    gchar *status = status_attr->GetStrValue ();

    if (status && *status == 'Q')
    {
      dropped = FALSE;
    }
  }

  player->SetAttributeValue (&status_attr_id,
                             reason);

  if (dropped == FALSE)
  {
    _nb_drop++;

    for (guint i = 0; i < GetNbPlayers (); i++)
    {
      Player *opponent;

      opponent = GetPlayer (i);
      if (opponent != player)
      {
        Match *match = GetMatch (opponent, player);

        match->DropPlayer (player);
      }
    }
  }

  if (_title_table)
  {
    Wipe ();
    Draw (GetCanvas (),
          FALSE);
  }
}

// --------------------------------------------------------------------------------
void Pool::RestorePlayer (Player *player)
{
  Player::AttributeId status_attr_id = Player::AttributeId ("status", GetDataOwner ());
  Attribute           *status_attr   = player->GetAttribute (&status_attr_id);
  gboolean             dropped       = TRUE;

  if (status_attr)
  {
    gchar *status = status_attr->GetStrValue ();

    if (status && (*status == 'Q'))
    {
      dropped = FALSE;
    }
  }

  player->SetAttributeValue (&status_attr_id,
                             "Q");
  if (dropped)
  {
    _nb_drop--;

    for (guint i = 0; i < GetNbPlayers (); i++)
    {
      Player *opponent;

      opponent = GetPlayer (i);
      if (opponent != player)
      {
        Match *match = GetMatch (opponent, player);

        match->RestorePlayer (player);
      }
    }
  }

  if (_title_table)
  {
    Wipe ();
    Draw (GetCanvas (),
          FALSE);
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
                      AttributeDesc::DISCRETE_XML_IMAGE, &code,
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
