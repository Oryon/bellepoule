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

#pragma once

#include <libxml/xmlschemas.h>
#include <gtk/gtk.h>
#include <goocanvas.h>

#include "util/object.hpp"

#include "error.hpp"

class Player;
class FieTime;
class Data;
class Score;
class XmlScheme;

class Match : public Object,
              public Error::Provider
{
  public:
    Match (Data     *max_score,
           gboolean  overflow_allowed);

    Match (Player   *A,
           Player   *B,
           Data     *max_score,
           gboolean  overflow_allowed = FALSE);

    void SetNameSpace (const gchar *name_space);

    gboolean ExemptedMatch ();

    Player *GetOpponent (guint position);

    Player *GetWinner  ();

    Player *GetLooser  ();

    gboolean IsStarted ();

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

    void SetPiste (guint piste);

    void SetStartTime (FieTime *time);

    void SetDuration (guint duration);

    void SetDurationSpan (guint span);

    gboolean AdjustRoadmap (Match *according_to);

    guint GetPiste ();

    FieTime *GetStartTime ();

    void SetScore (Player *fencer, gint score, gboolean is_the_best);

    gboolean SetScore (Player *fencer, gchar *value);

    Score *GetScore (Player *fencer);

    Score *GetScore (guint fencer);

    void Save (XmlScheme *xml_scheme);

    void Load (xmlNode *node_a,
               Player  *fencer_a,
               xmlNode *node_b,
               Player  *fencer_b);

    void Load (xmlNode *node,
               Player        *fencer);

    void DiscloseWithIdChain (va_list chain_id);

    void ChangeIdChain (guint batch_id,
                        guint netid);

    guint GetNetID ();

    void CleanScore ();

    void SetNumber (gint number);

    gint GetNumber ();

    const gchar *GetName ();

    GooCanvasItem *GetScoreTable (GooCanvasItem *parent,
                                  gdouble        size);

    void AddReferee (Player *referee);

    void RemoveReferee (Player *referee);

    void RemoveAllReferees ();

    GSList *GetRefereeList ();

    Player *GetFirstReferee ();

    void Timestamp ();

    void Spread () override;

    void Recall () override;

    gboolean IsDirty ();

    gboolean IsFalsified ();

    static gint Compare (Match *A,
                         Match *B);

    static void OnClockOffset (Net::Message *message);

    static guint GetDuration (GDateTime *start_time);

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
    guint     _piste;
    FieTime  *_start_time;
    guint     _duration_sec;
    guint     _duration_span;
    gboolean  _dirty;
    gboolean  _falsified;
    gboolean  _overflow_allowed;

    static GTimeSpan _clock_offset;

    gboolean ScoreIsNumber (gchar *score);

    void Save (XmlScheme *xml_scheme,
               Player    *fencer);

    void FeedParcel (Net::Message *parcel) override;

    ~Match () override;

    Error *SpawnError () override;

    void CheckFalsification (xmlNode  *node,
                             Opponent *opponent);
};
