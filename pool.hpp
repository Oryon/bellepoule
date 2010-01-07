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

#include "stage.hpp"
#include "canvas_module.hpp"
#include "score_collector.hpp"

class Pool_c : public CanvasModule_c
{
  public:
    typedef void (*StatusCbk) (Pool_c *pool,
                               void   *data);

    Pool_c (guint number);

    void  AddPlayer    (Player_c *player);
    void  RemovePlayer (Player_c *player);
    guint GetNbPlayers ();
    guint GetNumber    ();
    void  ResetMatches ();
    void  Lock         ();
    void  UnLock       ();
    void  SetMaxScore  (guint max_score);
    void  SetStatusCbk (StatusCbk  cbk,
                        void      *data);

    gboolean IsOver ();
    gboolean HasError ();

    Player_c *GetPlayer (guint i);

    gchar *GetName ();

    void Save  (xmlTextWriter *xml_writer);
    void Load  (xmlNode       *xml_node,
                GSList        *player_list);

    void SetRandSeed (guint32 seed);

    void SetDataOwner (Object_c *owner);

    static gint ComparePlayer (Player_c *A,
                               Player_c *B,
                               Pool_c   *pool);

  private:
    guint           _max_score;
    guint           _rand_seed;
    Object_c       *_data_owner;
    guint           _number;
    GSList         *_player_list;
    ScoreCollector *_score_collector;
    GSList         *_match_list;
    gchar          *_name;
    gboolean        _is_over;
    gboolean        _has_error;

    void           *_status_cbk_data;
    StatusCbk       _status_cbk;

  private:
    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);
    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

    void OnPlugged ();

    Match_c *GetMatch (Player_c *A,
                       Player_c *B);

    static void OnNewScore (CanvasModule_c *client);

    void RefreshScoreData ();

    void RefreshDashBoard ();

    ~Pool_c ();
};

#endif

