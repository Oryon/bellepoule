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

#ifndef schedule_hpp
#define schedule_hpp

#include <libxml/xmlwriter.h>
#include "stage.hpp"
#include "module.hpp"

class Contest;

class Schedule : public Module
{
  public:
    typedef void (Object::*StageEvent_t) ();

  public:
     Schedule (Contest *contest);

    void DisplayList ();

    void CreateDefault (gboolean without_pools = FALSE);

    void AddStage    (Stage *stage,
                      Stage *after);

    void Save (xmlTextWriter *xml_writer);

    void Load (xmlDoc        *doc);

    Stage *GetStage (guint index);

    void SetScoreStuffingPolicy (gboolean allowed);
    gboolean ScoreStuffingIsAllowed ();

    void on_previous_stage_toolbutton_clicked ();
    void on_next_stage_toolbutton_clicked     ();
    void on_stage_selected ();
    void on_stage_removed ();

  protected:
    void RefreshSensitivity ();

  private:
    GtkListStore       *_list_store;
    GtkTreeModelFilter *_list_store_filter;
    GList              *_stage_list;
    guint               _current_stage;
    gboolean            _score_stuffing_allowed;
    Contest            *_contest;

    void    SetCurrentStage    (guint index);
    Module *GetSelectedModule  ();

    gint GetNotebookPageNum (Stage *stage);

    void AddStage (Stage *stage);

    void RemoveStage (Stage *stage);

    void LoadStage (Stage   *stage,
                    xmlNode *xml_node,
                    guint   *nb_stage,
                    gint     current_stage_index);

    static void OnStageStatusUpdated (Stage    *stage,
                                      Schedule *schedule);

    static void on_row_selected (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer  *data);

    static gboolean on_new_stage_selected (GtkWidget      *widget,
                                           GdkEventButton *event,
                                           Schedule       *owner);

  private:
    GtkWidget *_formula_dlg;

    ~Schedule ();
    void OnPlugged ();
    void PlugStage (Stage *stage);
};

#endif
