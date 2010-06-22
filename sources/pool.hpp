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

#ifndef pool_hpp
#define pool_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "data.hpp"
#include "stage.hpp"
#include "canvas_module.hpp"
#include "score_collector.hpp"

class Pool : public CanvasModule
{
  public:
    typedef void (*StatusCbk) (Pool *pool,
                               void *data);

    Pool (Data  *max_score,
          guint  number);

    void  AddPlayer    (Player *player, Object *rank_owner);
    void  RemovePlayer (Player *player);
    guint GetNbPlayers ();
    guint GetNumber    ();
    void  CleanScores  ();
    void  ResetMatches (Object *rank_owner);
    void  SortPlayers  ();
    void  Lock         ();
    void  UnLock       ();
    void  SetStatusCbk (StatusCbk  cbk,
                        void      *data);

    gboolean IsOver ();
    gboolean HasError ();

    void RefreshScoreData ();

    Player *GetPlayer (guint i);

    gchar *GetName ();

    void Wipe ();

    void Save  (xmlTextWriter *xml_writer);
    void Load  (xmlNode       *xml_node,
                GSList        *player_list);

    void SetRandSeed (guint32 seed);

    void Stuff ();

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page_nr);

    static gint ComparePlayer (Player   *A,
                               Player   *B,
                               Object   *data_owner,
                               guint32   rand_seed,
                               gboolean  with_full_random = FALSE);

  private:
    Data           *_max_score;
    guint           _rand_seed;
    guint           _number;
    GSList         *_player_list;
    ScoreCollector *_score_collector;
    GSList         *_match_list;
    gchar          *_name;
    gboolean        _is_over;
    gboolean        _has_error;
    GooCanvasItem  *_title_table;
    GooCanvasItem  *_status_item;
    gboolean        _locked;

    void           *_status_cbk_data;
    StatusCbk       _status_cbk;

  private:
    static gint _ComparePlayer (Player *A,
                                Player *B,
                                Pool   *pool);

    static gint _ComparePlayerWithFullRandom (Player *A,
                                              Player *B,
                                              Pool   *pool);

    void OnPlugged ();

    void Draw (GooCanvas *on_canvas,
               gboolean   print_for_referees);

    Match *GetMatch (Player *A,
                     Player *B);

    Match *GetMatch (guint i);

    static void OnNewScore (ScoreCollector *score_collector,
                            CanvasModule   *client,
                            Match          *match,
                            Player         *player);

    void RefreshDashBoard ();

    ~Pool ();
};

#endif
