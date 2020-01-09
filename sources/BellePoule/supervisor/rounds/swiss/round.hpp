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

#include "util/canvas_module.hpp"
#include "../../stage.hpp"
#include "../../score_collector.hpp"

class Data;

namespace Swiss
{
  class Hall;
  class Elo;
  class Quest;

  class Round : public Stage,
                public CanvasModule,
                public ScoreCollector::Listener
  {
    public:
      static void Declare ();

      Round (StageClass *stage_class);

      void OnPisteCountChanged (GtkEditable *editable);

      void OnStuffClicked ();

      void OnHighlightChanged (GtkEditable *editable);

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

    private:
      static GdkPixbuf     *_moved_pixbuf;
      static const gdouble  _score_rect_w;
      static const gdouble  _score_rect_h;

      Elo            *_elo;
      Quest          *_quest;
      Hall           *_hall;
      Data           *_matches_per_fencer;
      Data           *_available_time;
      Data           *_piste_count;
      GList          *_matches;
      ScoreCollector *_score_collector;
      GooCanvasItem  *_table;
      guint           _rows;
      GtkWidget      *_piste_entry;
      GtkWidget      *_per_fencer_entry;
      GtkWidget      *_duration_entry;
      gulong          _per_fencer_entry_handler;
      gulong          _duration_entry_handler;

      ~Round () override;

      void SaveConfiguration (XmlScheme *xml_scheme) override;

      void Load (xmlNode *xml_node) override;

      void LoadConfiguration (xmlNode *xml_node) override;

      void LoadMatches (xmlNode *xml_node);

      void Save (XmlScheme *xml_scheme) override;

      void SaveHeader (XmlScheme *xml_scheme);

      void FillInConfig () override;

      void Garnish () override;

      void Reset () override;

      void Display () override;

      void Wipe () override;

      void Dump () override;

      void OnMatchesPerFencerChanged (GtkEditable *editable);

      void OnDurationChanged (GtkEditable *editable);


      void SetOutputShortlist () override;

      void TossMatches (GSList *fencers,
                        guint   matches_per_fencer);

      void DisplayMatch (Match *match);

      gboolean MatchIsCancelled (Match *match);

      void RefreshMatch (Match *match);

      void SynchronizeConfiguration (GtkEditable *editable);

      GooCanvasItem *DisplayScore (GooCanvasItem *table,
                                   guint          row,
                                   guint          column,
                                   Match         *match,
                                   Player        *fencer);

      void OnPlugged () override;

      void OnAttrListUpdated () override;

      GSList *GetCurrentClassification () override;

      void OnNewScore (ScoreCollector *score_collector,
                       Match          *match,
                       Player         *player) override;

      gboolean OnMessage (Net::Message *message) override;

      static void OnStatusChanged (GtkComboBox *combo_box,
                                   Round       *round);

      static gboolean OnStatusKeyPressEvent (GtkWidget   *widget,
                                             GdkEventKey *event,
                                             Round       *round);

      static gboolean OnStatusArrowPress (GooCanvasItem  *item,
                                          GooCanvasItem  *target,
                                          GdkEventButton *event,
                                          Round          *round);

      gboolean FencerHasMatch (Player *fencer,
                               GList  *matches);

      static Stage *CreateInstance (StageClass *stage_class);

      static void on_per_fencer_entry_changed (GtkEditable *editable,
                                               Object      *owner);

      static void on_duration_entry_changed (GtkEditable *editable,
                                             Object      *owner);
  };
}
