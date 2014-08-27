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

#ifndef match_hpp
#define match_hpp

#include <libxml/xmlwriter.h>
#include <gtk/gtk.h>
#include <goocanvas.h>

#include "util/object.hpp"
#include "util/data.hpp"

#include "score.hpp"

class Player;
class Match : public Object
{
  public:
    Match  (Data *max_score);

    Match  (Player *A,
            Player *B,
            Data   *max_score);

    void SetNameSpace (const gchar *name_space);

    Player *GetOpponent (guint position);

    Player *GetWinner  ();

    Player *GetLooser  ();

    void SetOpponent (guint   position,
                      Player *fencer);

    void RemoveOpponent (guint position);

    void DropFencer (Player *fencer,
                     gchar  *reason);

    gboolean IsDropped ();

    void RestoreFencer (Player *fencer);

    gboolean HasFencer (Player *fencer);

    gboolean IsOver ();

    gboolean HasError ();

    void SetScore (Player *fencer, gint score, gboolean is_the_best);

    gboolean SetScore (Player *fencer, gchar *score);

    Score *GetScore (Player *fencer);

    void Save (xmlTextWriter *xml_writer);

    void Load (xmlNode *node_a,
               Player  *fencer_a,
               xmlNode *node_b,
               Player  *fencer_b);

    void Load (xmlNode *node,
               Player        *fencer);

    void SynchronizeScores ();

    void CleanScore ();

    void SetNumber (gint number);

    gint GetNumber ();

    const gchar *GetName ();

    GooCanvasItem *GetScoreTable (GooCanvasItem *parent,
                                  gdouble        size);

    void AddReferee (Player *referee);

    void RemoveReferee (Player *referee);

    GSList *GetRefereeList ();

  private:
    struct Opponent
    {
      Player   *_fencer;
      gboolean  _is_known;
      Score    *_score;
    };

    Opponent  _opponents[2];
    Data     *_max_score;
    gchar    *_name_space;
    gchar    *_name;
    guint     _number;
    GSList   *_referee_list;

    gboolean ScoreIsNumber (gchar *score);

    void Save (xmlTextWriter *xml_writer,
               Player        *fencer);

    void Init (Data *max_score);

    virtual ~Match ();
};

#endif
