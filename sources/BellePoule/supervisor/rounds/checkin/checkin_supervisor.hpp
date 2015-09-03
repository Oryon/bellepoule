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

#ifndef checkin_supervisor_hpp
#define checkin_supervisor_hpp

#include <gtk/gtk.h>
#include <libxml/xmlwriter.h>

#include "actors/team.hpp"
#include "actors/fencer.hpp"
#include "actors/null_team.hpp"
#include "actors/checkin.hpp"
#include "../../stage.hpp"

namespace People
{
  class CheckinSupervisor : public virtual Checkin, public Stage
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
      guint32   _dnd_key;

      static Stage *CreateInstance (StageClass *stage_class);

      void Monitor (Player *player);

      gboolean IsOver ();

      void UpdateChecksum ();

      GSList *GetCurrentClassification ();

      void LoadConfiguration (xmlNode *xml_node);

      void Load (xmlNode *xml_node);

      void Load (xmlXPathContext *xml_context,
                 const gchar     *from_node);

      void OnPlayerLoaded (Player *player,
                           Player *owner);

      void Save (xmlTextWriter *xml_writer);

      void SavePlayer (xmlTextWriter *xml_writer,
                       const gchar   *player_class,
                       Player        *player);

      void RegisterNewTeam (Team *team);

      void OnPlayerEventFromForm (Player            *player,
                                  Form::PlayerEvent  event);

      Team *GetTeam (const gchar *name);

      void SetTeamEvent (gboolean team_event);

      Player *GetFencerFromDndRef (guint ref);

      void OnPlayerRemoved (Player *player);

      void OnAttendingChanged (Player *player,
                               guint   value);

      void EnableDragAndDrop ();

      void DisableDragAndDrop ();

      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *selection_data,
                          guint             key,
                          guint             time);

      void OnDragDataReceived (GtkWidget        *widget,
                               GdkDragContext   *drag_context,
                               gint              x,
                               gint              y,
                               GtkSelectionData *selection_data,
                               guint             key,
                               guint             time);

      gboolean OnDragDrop (GtkWidget      *widget,
                           GdkDragContext *drag_context,
                           gint            x,
                           gint            y,
                           guint           time);

      gboolean OnDragMotion (GtkWidget      *widget,
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

#endif
