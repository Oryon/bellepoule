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

#include "object.hpp"
#include "data.hpp"
#include "score.hpp"

class Player;
class Match : public Object
{
  public:
    Match  (Data *max_score);

    Match  (Player *A,
            Player *B,
            Data   *max_score);

    Player *GetPlayerA ();
    Player *GetPlayerB ();
    Player *GetWinner  ();
    Player *GetLooser  ();

    void SetPlayerA (Player *player);

    void SetPlayerB (Player *player);

    void DropPlayer (Player *player);

    gboolean IsDropped ();

    void RestorePlayer (Player *player);

    gboolean HasPlayer (Player *player);

    gboolean PlayerHasScore (Player *player);

    void SetScore (Player *player, gint score, gboolean is_the_best);

    gboolean SetScore (Player *player, gchar *score);

    Score *GetScore (Player *player);

    void Save (xmlTextWriter *xml_writer);

    // Workaround to avoid the GREG issue
    void SaveInOrder (xmlTextWriter *xml_writer,
                      Player        *first_player);

    void CleanScore ();

    void SetNumber (gint number);

    gint GetNumber ();

    gchar *GetName ();

    void DrawScoreCell (GooCanvasItem *table,
                        guint          size,
                        guint          line);

  private:
    Data     *_max_score;
    Player   *_A;
    Player   *_B;
    gboolean  _A_is_dropped;
    gboolean  _B_is_dropped;
    gboolean  _A_is_known;
    gboolean  _B_is_known;
    Score    *_A_score;
    Score    *_B_score;
    gchar    *_name;
    guint     _number;

    gboolean ScoreIsNumber (gchar *score);

    void Save (xmlTextWriter *xml_writer,
               Player        *player);

    void Init (Data *max_score);

    ~Match ();
};

#endif
