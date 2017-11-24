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
#include <libxml/xmlwriter.h>

#include "actors/checkin.hpp"
#include "../../stage.hpp"

class Data;
class Team;
class NullTeam;

namespace People
{
  class CheckinSupervisor : public Checkin, public Stage
  {
    public:
      static void Declare ();

      CheckinSupervisor (StageClass  *stage_class);

      void UpdateRanking ();

      void SetTeamData (Data *minimum_team_size,
                        Data *default_classification,
                        Data *manual_classification);

      void ConvertFromBaseToResult ();

      void OnImportRanking ();

      void OnConfigChanged ();

      void OnListChanged ();

      void Add (Player *player);

    private:
      void OnLocked ();

      void OnUnLocked ();

      void OnLoadingCompleted ();

      void FillInConfig ();

      void ApplyConfig ();

      void ApplyConfig (Team *team);

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

    private:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

      GSList   *_checksum_list;
      Data     *_manual_classification;
      Data     *_minimum_team_size;
      Data     *_default_classification;
      NullTeam *_null_team;

      static Stage *CreateInstance (StageClass *stage_class);

      void Monitor (Player *player);

      gboolean IsOver ();

      void UpdateChecksum ();

      GSList *GetCurrentClassification ();

      void Load (xmlNode *xml_node);

      void OnPlayerLoaded (Player *player,
                           Player *owner);

      void Save (xmlTextWriter *xml_writer);

      void SavePlayer (xmlTextWriter *xml_writer,
                       const gchar   *player_class,
                       Player        *player);

      void RegisterNewTeam (Team *team);

      void OnFormEvent (Player          *player,
                        Form::FormEvent  event);

      Team *GetTeam (const gchar *name);

      void SetTeamEvent (gboolean team_event);

      void OnPlayerRemoved (Player *player);

      void OnAttendingChanged (Player *player,
                               guint   value);

      void FeedParcel (Net::Message *parcel);

      void EnableDragAndDrop ();

      void DisableDragAndDrop ();

      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *selection_data,
                          guint             key,
                          guint             time);

      Object *GetDropObjectFromRef (guint32 ref,
                                    guint   key);

      gboolean OnDragMotion (GtkWidget      *widget,
                             GdkDragContext *drag_context,
                             gint            x,
                             gint            y,
                             guint           time);

      gboolean OnDragDrop (GtkWidget      *widget,
                           GdkDragContext *drag_context,
                           gint            x,
                           gint            y,
                           guint           time);

      static gboolean PresentPlayerFilter (Player      *player,
                                           PlayersList *owner);

      static void OnAttrTeamRenamed (Player    *player,
                                     Attribute *attr,
                                     Object    *owner,
                                     guint      step);

      static void OnAttrTeamChanged (Player    *player,
                                     Attribute *attr,
                                     Object    *owner,
                                     guint      step);

      void TogglePlayerAttr (Player              *player,
                             Player::AttributeId *attr_id,
                             gboolean             new_value,
                             gboolean             popup_on_error = FALSE);

      void CellDataFunc (GtkTreeViewColumn *tree_column,
                         GtkCellRenderer   *cell,
                         GtkTreeModel      *tree_model,
                         GtkTreeIter       *iter);

      gboolean PlayerIsPrintable (Player *player);

      void DrawConfig (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr);

      virtual ~CheckinSupervisor ();
  };
}
