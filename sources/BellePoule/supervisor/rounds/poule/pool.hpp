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

#include "util/data.hpp"
#include "util/canvas_module.hpp"
#include "util/player.hpp"
#include "../../stage.hpp"
#include "../../score_collector.hpp"

#include "dispatcher/dispatcher.hpp"

class FieTime;

namespace Pool
{
  class Pool : public CanvasModule
  {
    public:
      class StatusListener
      {
        public:
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
            ...);

      guint        GetPiste         ();
      FieTime     *GetStartTime     ();
      void         AddFencer        (Player *player, Object *rank_owner);
      void         AddReferee       (Player *player);
      void         CreateMatchs     (GSList *affinity_criteria_list);
      void         RemoveFencer     (Player *player, Object *rank_owner);
      void         RemoveReferee    (Player *player);
      void         RemoveAllReferee ();
      guint        GetNbPlayers     ();
      guint        GetNbMatchs      ();
      guint        GetNumber        ();
      guint        GetStrength      ();
      void         DropPlayer       (Player *player, gchar *reason);
      void         RestorePlayer    (Player *player);
      void         CleanScores      ();
      void         DeleteMatchs     ();
      void         Lock             ();
      void         UnLock           ();
      void         SetDataOwner     (Object *current_round_owner,
                                     Object *combined_owner,
                                     Object *combined_source_owner);

      void OnStatusChanged   (GtkComboBox *combo_box);
      void CopyPlayersStatus (Object *from);

      void  RegisterStatusListener (StatusListener *listener);

      void SetStrengthContributors (gint contributors_count);

      gboolean IsOver ();
      gboolean HasError ();

      gboolean OnMessage (Net::Message *message);
      gboolean OnHttpPost (const gchar **ressource,
                           const gchar *data);

      void RefreshScoreData ();

      void RefreshStatus ();

      gchar *GetName ();

      void Wipe ();

      void Save (xmlTextWriter *xml_writer);

      void Load (xmlNode *xml_node);

      void Load (xmlNode *xml_node,
                 GSList  *player_list);

      void DumpToHTML (FILE  *file,
                       guint  grid_size);

      void Stuff ();

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

      GSList *GetFencerList ();

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
    Data            *_max_score;
    guint            _number;
    GSList          *_fencer_list;
    GSList          *_sorted_fencer_list;
    GSList          *_referee_list;
    guint            _piste;
    FieTime         *_start_time;
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
      typedef enum
      {
        NONE,
        AVERAGE,
        SUM
      } CombinedOperation;

      static gint _ComparePlayer (Player *A,
                                  Player *B,
                                  Pool   *pool);

      static gint _ComparePlayerWithFullRandom (Player *A,
                                                Player *B,
                                                Pool   *pool);

      static gint CompareStatus (gchar A,
                                 gchar B);

      void OnPlugged ();

      void OnUnPlugged ();

      void SortPlayers ();

      void RefreshStrength (Object *rank_owner);
      Player *GetPlayer (guint   i,
                         GSList *in_list);

      void SetDisplayData (Player      *player,
                           GooCanvas   *on_canvas,
                           const gchar *name,
                           void        *value);

      void Draw (GooCanvas *on_canvas,
                 gboolean   print_for_referees,
                 gboolean   print_matchs);

      void DumpToHTML (FILE                *file,
                       Player              *fencer,
                       const gchar         *attr_name,
                       AttributeDesc::Look  look);

      Match *GetMatch (Player *A,
                       Player *B);

      Match *GetMatch (guint i);

      static void OnNewScore (ScoreCollector *score_collector,
                              CanvasModule   *client,
                              Match          *match,
                              Player         *player);

      void RefreshDashBoard ();

      void RefreshAttribute (Player            *player,
                             const gchar       *name,
                             guint              value,
                             CombinedOperation  operation,
                             guint              combined_value = 0);

      void RefreshAttribute (Player            *player,
                             const gchar       *name,
                             gchar             *value);

      void FeedParcel (Net::Message *parcel);

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

      virtual ~Pool ();
  };
}
