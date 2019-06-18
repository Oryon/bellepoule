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

#include <gtk/gtk.h>
#include <goocanvas.h>
#include <libxml/xpath.h>

#include "util/canvas_module.hpp"
#include "util/attribute_desc.hpp"
#include "../../score_collector.hpp"

class FieTime;
class Data;
class Player;
class ScoreCollector;
class Match;
class XmlScheme;
class Attribute;

namespace Pool
{
  class Dispatcher;

  class Pool : public CanvasModule,
               public ScoreCollector::Listener
  {
    public:
      struct StatusListener
      {
        virtual void OnPoolStatus (Pool *pool) = 0;
      };

    public:
      typedef enum
      {
        WITH_POOL_NR  = 0x1,
        WITH_CALCULUS = 0x2,
        WITH_RANDOM   = 0x4
      } ComparisonPolicy;

    public:
      Pool (Data        *max_score,
            guint        number,
            const gchar *xml_player_tag,
            guint32      rand_seed,
            guint        stage_id,
            Object      *rank_owner,
            ...);

      guint    GetPiste         ();
      FieTime *GetStartTime     ();
      void     AddFencer        (Player *player, gint position = -1);
      void     AllowFencer      (Player *player);
      void     AddReferee       (Player *player);
      void     CreateMatchs     (GSList *affinity_criteria_list);
      void     RemoveFencer     (Player *player);
      void     DismissFencer    (Player *player);
      void     RemoveReferee    (Player *player);
      void     RemoveAllReferee ();
      guint    GetNbPlayers     ();
      guint    GetNumber        ();
      guint    GetStrength      ();
      void     DropPlayer       (Player *player, gchar *reason);
      void     RestorePlayer    (Player *player);
      void     CleanScores      ();
      void     DeleteMatchs     ();
      void     Lock             ();
      void     UnLock           ();
      void     SetDataOwner     (Object *current_round_owner,
                                 Object *combined_owner,
                                 Object *combined_source_owner);

      void OnStatusChanged   (GtkComboBox *combo_box);
      void CopyPlayersStatus (Object *from);

      void  RegisterStatusListener (StatusListener *listener);

      void SetStrengthContributors (gint contributors_count);

      gboolean IsOver ();
      gboolean HasError ();

      gboolean OnMessage (Net::Message *message) override;

      void RefreshScoreData ();

      void RefreshStatus ();

      gchar *GetName ();

      void Wipe () override;

      void Save (XmlScheme *xml_scheme);

      void Load (xmlNode *xml_node);

      void Load (xmlNode *xml_node,
                 GSList  *player_list);

      void DumpToHTML (FILE  *file,
                       guint  grid_size);

      void Stuff ();

      GooCanvasItem *DrawPage (GtkPrintOperation *operation,
                               GooCanvas         *canvas);

      GSList *GetFencerList ();

      GSList *GetSortedFencerList ();

      GSList *GetRefereeList ();

      GdkPixbuf *GetStatusPixbuf ();

      static gint ComparePlayer (Player   *A,
                                 Player   *B,
                                 Object   *data_owner,
                                 guint32   rand_seed,
                                 guint     comparison_policy);

      static void SetWaterMarkingPolicy (gboolean enabled);

      static gboolean WaterMarkingEnabled ();

    private:
      static gboolean  _match_id_watermarked;
      Object          *_combined_rounds_owner;
      Object          *_previous_combined_round;
      Object          *_rank_owner;
      Data            *_max_score;
      guint            _number;
      GSList          *_fencer_list;
      GSList          *_sorted_fencer_list;
      GSList          *_referee_list;
      guint            _strip;
      FieTime         *_start_time;
      guint            _duration_sec;
      ScoreCollector  *_score_collector;
      GList           *_match_list;
      gchar           *_name;
      gboolean         _is_over;
      gboolean         _has_error;
      GooCanvasItem   *_title_table;
      GooCanvasItem   *_status_item;
      GdkPixbuf       *_status_pixbuf;
      gboolean         _locked;
      GSList          *_display_data;
      guint            _nb_drop;
      const gchar     *_xml_player_tag;
      guint            _strength;
      guint            _strength_contributors;
      Dispatcher      *_dispatcher;

      StatusListener  *_status_listener;

    private:
      enum class CombinedOperation
      {
        NONE,
        AVERAGE,
        SUM
      };

      static gint _ComparePlayer (Player *A,
                                  Player *B,
                                  Pool   *pool);

      static gint _ComparePlayerWithFullRandom (Player *A,
                                                Player *B,
                                                Pool   *pool);

      static gint CompareStatus (gchar A,
                                 gchar B);

      void OnPlugged () override;

      void OnUnPlugged () override;

      void SortPlayers ();

      gboolean FencerIsAbsent (Player *fencer);

      void RefreshStrength ();

      Player *GetPlayer (guint   i,
                         GSList *in_list);

      void SetDisplayData (Player      *player,
                           GooCanvas   *on_canvas,
                           const gchar *name,
                           void        *value);

      GooCanvasItem *Draw (GooCanvas *on_canvas,
                           gboolean   print_for_referees,
                           gboolean   print_matchs);

      void DumpToHTML (FILE                *file,
                       Player              *fencer,
                       const gchar         *attr_name,
                       AttributeDesc::Look  look);

      guint GetNbMatchs ();

      guint GetNbMatchs (Player *of);

      Match *GetMatch (Player *A,
                       Player *B);

      Match *GetMatch (guint i);

      void OnNewScore (ScoreCollector *score_collector,
                       Match          *match,
                       Player         *player) override;

      void Timestamp ();

      void SetRoadmap (guint    strip,
                       FieTime *start_time);

      void SetRoadmapTo (Player  *fencer,
                         guint    strip,
                         FieTime *start_time);

      void RefreshDashBoard ();

      void RefreshAttribute (Player            *player,
                             const gchar       *name,
                             guint              value,
                             CombinedOperation  operation,
                             guint              combined_value = 0);

      void RefreshAttribute (Player            *player,
                             const gchar       *name,
                             gchar             *value);

      void FeedParcel (Net::Message *parcel) override;

      static gint CompareMatch (Match *a,
                                Match *b,
                                Pool  *pool);

      static void on_status_changed (GtkComboBox *combo_box,
                                     Pool        *pool);

      static gboolean on_status_scrolled (GtkWidget *widget,
                                          GdkEvent  *event,
                                          gpointer   user_data);

      static void OnAttrConnectionChanged (Player    *player,
                                           Attribute *attr,
                                           Object    *owner,
                                           guint      step);

      ~Pool () override;
  };
}
