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

#include "common/stage.hpp"
#include "team.hpp"

#include "checkin.hpp"

namespace People
{
  class CheckinSupervisor : public virtual Checkin, public Stage
  {
    public:
      static void Declare ();

      CheckinSupervisor (StageClass  *stage_class);

      void UseInitialRank ();

      void UpdateRanking ();

      void ConvertFromBaseToResult ();

    private:
      void OnLocked ();

      void OnUnLocked ();

      void OnLoadingCompleted ();

      void Wipe ();

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

    private:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

      gboolean  _use_initial_rank;
      GSList   *_checksum_list;
      gboolean  _attending_recursivity_running;

      static Stage *CreateInstance (StageClass *stage_class);

      gboolean IsOver ();

      void UpdateChecksum ();

      GSList *GetCurrentClassification ();

      void Load (xmlNode *xml_node);

      void Load (xmlXPathContext *xml_context,
                 const gchar     *from_node);

      void OnPlayerLoaded (Player *player);

      void Save (xmlTextWriter *xml_writer);

      void RegisterNewTeam (const gchar *name);

      void OnPlayerEventFromForm (Player            *player,
                                  Form::PlayerEvent  event);

      Team *GetTeam (const gchar *name);

      void SetTeamEvent (gboolean team_event);

      void OnPlayerRemoved (Player *player);

      void OnAttendingChanged (Player    *player,
                               guint   value);

      virtual ~CheckinSupervisor ();
  };
}

#endif
