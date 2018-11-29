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
#include "network/advertiser.hpp"
#include "../../error.hpp"

class Match;
class XmlScheme;

namespace Table
{
  class TableSet;

  class Table : public Object,
                public Net::Advertiser::Feeder,
                public Error::Provider
  {
    public:
      Table (TableSet    *table_set,
             const gchar *xml_player_tag,
             guint        first_place,
             guint        size,
             guint        number,
             ...);

      gchar *GetImage ();

      const gchar *GetMiniName ();

      void SetRightTable (Table *right);

      Table *GetRightTable ();

      Table *GetLeftTable ();

      guint GetSize ();

      guint GetRow (guint for_index);

      guint GetColumn ();

      guint GetNumber ();

      void ConfigureExtensions ();

      void Show (guint    at_column,
                 gboolean display_header);

      void Hide ();

      void FeedParcel (Net::Message *parcel) override;

      void Spread () override;

      void Recall () override;

      void ClearRoadmaps ();

      void SaveHeader (XmlScheme *xml_scheme);

      void Save (XmlScheme *xml_scheme);

      void Load (xmlNode *xml_node);

      gboolean IsDisplayed ();

      gboolean IsHeaderDisplayed ();

      void ManageMatch (Match *match,
                        ...);

      void DropMatch (Match *match);

      Match *GetMatch (Net::Message *roadmap);

      void AddNode (GNode *node);

      GNode *GetNode (guint index);

      static gboolean NodeHasGooTable (GNode *node);

      guint GetLoosers (GSList **loosers,
                        GSList **withdrawals,
                        GSList **blackcardeds);

      Error::Provider *_first_error;
      gboolean         _is_over;
      gboolean         _ready_to_fence;
      guint            _roadmap_count;
      gboolean         _has_all_roadmap;
      GooCanvasItem   *_status_item;
      GooCanvasItem   *_header_item;
      TableSet        *_defeated_table_set;

    private:
      gchar         *_mini_name;
      guint         _size;
      guint         _number;
      guint         _column;
      gboolean      _is_displayed;
      gboolean      _is_header_displayed;
      gboolean      _loaded;
      Table        *_left_table;
      Table        *_right_table;
      GSList       *_match_list;
      TableSet     *_table_set;
      GNode       **_node_table;
      guint         _free_node_index;
      const gchar  *_xml_player_tag;

      ~Table () override;

      static gint CompareMatchNumber (Match *a,
                                      Match *b);

      void LoadMatch (xmlNode *xml_node,
                      Match   *match);

      void SimplifyLooserTree (GSList **list);

      gchar *GetAnnounce () override;

      Error *SpawnError () override;
  };
}
