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

#include "stage.hpp"
#include "checkin.hpp"

class CheckinSupervisor : public virtual Checkin, public Stage
{
  public:
    static void Init ();

    CheckinSupervisor (StageClass  *stage_class);

    void UseInitialRank ();

    void UpdateRanking ();

    void ConvertFromBaseToResult ();

  private:
    void OnLocked (Reason reason);

    void OnUnLocked ();

    void OnLoadingCompleted ();

    void Wipe ();

  private:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

    gboolean  _use_initial_rank;
    GSList   *_checksum_list;

    static Stage *CreateInstance (StageClass *stage_class);

    gboolean IsOver ();

    void UpdateChecksum ();

    void ClearChecksum ();

    GSList *GetCurrentClassification ();

    void Load (xmlNode *xml_node);

    void Load (xmlXPathContext *xml_context,
               const gchar     *from_node);

    void OnLoaded ();

    void OnPlayerLoaded (Player *player);

    void Save (xmlTextWriter *xml_writer);

    ~CheckinSupervisor ();
};

#endif
