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

#include "util/module.hpp"

#include "stage.hpp"

namespace People
{
  class Checkin;
  class RefereesList;
  class CheckinSupervisor;
}

class Contest;
class Data;
class Book;
class XmlScheme;

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

    void SaveFinalResults (XmlScheme *xml_scheme);

    void SavePeoples (XmlScheme       *xml_scheme,
                      People::Checkin *referees);

    void Save (XmlScheme *xml_scheme);

    void DumpToHTML (FILE *file) override;

    void Load (xmlDoc               *doc,
               const gchar          *contest_keyword,
               People::RefereesList *referees);

    void OnLoadingCompleted ();

    void ApplyNewConfig ();

    void Mutate (const gchar *from,
                 const gchar *to);

    void SetScoreStuffingPolicy (gboolean allowed);
    gboolean ScoreStuffingIsAllowed ();

    void OnPrint ();

    gboolean OnMessage (Net::Message *message) override;

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
    GtkMenuShell       *_menu_shell;
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
                   Stage *after = nullptr);

    void InsertStage (Stage *stage,
                      Stage *after);

    void RemoveStage (Stage *stage);

    void GiveStagesAnId ();

    Stage *GetStage (guint index);

    void LoadPeoples (Stage           *stage_host,
                      xmlXPathContext *xml_context,
                      const gchar     *contest_keyword);

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
                        GtkPrintContext   *context) override;

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page_nr) override;

    void OnStageStatusChanged (Stage *stage) override;

    static void on_row_selected (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer  *data);

    static gboolean on_new_stage_selected (GtkWidget      *widget,
                                           GdkEventButton *event,
                                           Schedule       *owner);

  private:
    Book *_book;

    ~Schedule () override;
    void OnPlugged () override;
    void PlugStage (Stage *stage);
    void RefreshStageName (Stage *stage);
    void RemoveFromNotebook (Stage *stage);
    Stage *CreateStage (const gchar *class_name);
};
