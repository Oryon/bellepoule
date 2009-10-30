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
#include "score.hpp"

class Player_c;
class Match_c : public Object_c
{
  public:
    Match_c  (Player_c *A,
              Player_c *B,
              guint     max_score);
    ~Match_c ();

    Player_c *GetPlayerA ();
    Player_c *GetPlayerB ();

    gboolean HasPlayer (Player_c *player);

    gboolean PlayerHasScore (Player_c *player);

    void SetScore (Player_c *player, gint score);

    gboolean SetScore (Player_c *player, gchar *score);

    Score_c *GetScore (Player_c *player);

    void Save (xmlTextWriter *xml_writer);

    void CleanScore ();

    void SetMaxScore (guint max_score);

  private:
    guint     _max_score;
    Player_c *_A;
    Player_c *_B;
    Score_c  *_A_score;
    Score_c  *_B_score;

    gboolean ScoreIsNumber (gchar *score);
};

#endif
