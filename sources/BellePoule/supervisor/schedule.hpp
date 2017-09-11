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

#include <libxml/xmlwriter.h>

#include "util/data.hpp"
#include "util/module.hpp"
#include "book/book.hpp"

#include "stage.hpp"

namespace People
{
  class Checkin;
  class RefereesList;
  class CheckinSupervisor;
}

class Contest;

class Schedule : public Module,
                 public Stage::Listener
{
  public:
    typedef void (Object::*StageEvent_t) ();

  public:
    Schedule (Contest *contest,
              GList   *advertisers,
              Data    *minimum_team_size,
              Data    *manual_classification,
              Data    *default_classification);

    People::CheckinSupervisor *GetCheckinSupervisor ();

    gboolean IsOver ();

    void Freeze ();

    void CreateDefault (gboolean without_pools = FALSE);

    void SavePeoples (xmlTextWriter   *xml_writer,
                      People::Checkin *referees);

    void Save (xmlTextWriter *xml_writer);

    void DumpToHTML (FILE *file);

    void Load (xmlDoc               *doc,
               const gchar          *contest_keyword,
               People::RefereesList *referees);

    void OnLoadingCompleted ();

    void ApplyNewConfig ();

    void SetScoreStuffingPolicy (gboolean allowed);
    gboolean ScoreStuffingIsAllowed ();

    void OnPrint ();

    gboolean OnMessage (Net::Message *message);

    gboolean OnHttpPost (const gchar *command,
                         const gchar **ressource,
                         const gchar *data);

    void SetTeamEvent (gboolean team_event);

    void PrepareBookPrint (GtkPrintOperation *operation,
                           GtkPrintContext   *context);
    void DrawBookPage (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr);
    void StopBookPrint (GtkPrintOperation *operation,
                        GtkPrintContext   *context);

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
    Data               *_minimum_team_size;
    Data               *_default_classification;
    Data               *_manual_classification;
    GList              *_advertisers;

    void SetCurrentStage (guint index);

    Module *GetSelectedModule  ();

    void DisplayLocks ();

    gint GetNotebookPageNum (Stage *stage);

    void AddStage (Stage *stage,
                   Stage *after = NULL);

    void InsertStage (Stage *stage,
                      Stage *after);

    void RemoveStage (Stage *stage);

    void GiveStagesAnId ();

    Stage *GetStage (guint index);

    void LoadStage (Stage   *stage,
                    xmlNode *xml_node,
                    guint   *nb_stage,
                    gint     current_stage_index);

    void LoadStage (Stage   *stage,
                    xmlNode *xml_node,
                    guint    nb_stage,
                    gint     current_stage_index);

    void GiveName (Stage *stage);

    guint PreparePrint (GtkPrintOperation *operation,
                        GtkPrintContext   *context);

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page_nr);

    void OnStageStatusChanged (Stage *stage);

    static void on_row_selected (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer  *data);

    static gboolean on_new_stage_selected (GtkWidget      *widget,
                                           GdkEventButton *event,
                                           Schedule       *owner);

  private:
    Book *_book;

    virtual ~Schedule ();
    void OnPlugged ();
    void PlugStage (Stage *stage);
    void RefreshStageName (Stage *stage);
    void RemoveFromNotebook (Stage *stage);
    Stage *CreateStage (const gchar *class_name);
};
