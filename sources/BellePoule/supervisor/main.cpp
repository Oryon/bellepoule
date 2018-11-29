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

#include "util/player.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "rounds/barrage/barrage.hpp"
#include "rounds/checkin/checkin_supervisor.hpp"
#include "rounds/classification/general_classification.hpp"
#include "rounds/splitting/splitting.hpp"
#include "rounds/poule/pool_allocator.hpp"
#include "rounds/poule/pool_supervisor.hpp"
#include "rounds/tableau/table_supervisor.hpp"
#include "actors/fencer.hpp"
#include "actors/referee.hpp"
#include "actors/team.hpp"
#include "application/application.hpp"

#include "contest.hpp"
#include "tournament.hpp"

// --------------------------------------------------------------------------------
class BellPouleApp : public Application
{
  public:
    BellPouleApp (int *argc, char ***argv);

  private:
    Tournament *_tournament;

    ~BellPouleApp () override;

    void Prepare () override;

    void Start (int                   argc,
                char                **argv,
                Net::Ring::Listener  *ring_listener) override;

    void OnQuit (GtkWindow *window) override;
};

// --------------------------------------------------------------------------------
BellPouleApp::BellPouleApp (int    *argc,
                            char ***argv)
  : Application (Net::Ring::RESOURCE_USER, "BellePoule", argc, argv)
{
}

// --------------------------------------------------------------------------------
BellPouleApp::~BellPouleApp ()
{
  _tournament->Release ();
}

// --------------------------------------------------------------------------------
void BellPouleApp::Prepare ()
{
  Tournament::Init ();
  Contest::Init    ();

  {
    People::CheckinSupervisor::Declare     ();
    Pool::Allocator::Declare               ();
    Pool::Supervisor::Declare              ();
    Table::Supervisor::Declare             ();
    People::Barrage::Declare               ();
    People::GeneralClassification::Declare ();
    People::Splitting::Declare             ();
  }

  {
    Fencer::RegisterPlayerClass  ();
    Team::RegisterPlayerClass    ();
    Referee::RegisterPlayerClass ();
  }

  Application::Prepare ();
}

// --------------------------------------------------------------------------------
void BellPouleApp::Start (int                   argc,
                          char                **argv,
                          Net::Ring::Listener  *ring_listener)
{
  _tournament = new Tournament ();

  People::Splitting::SetHostTournament (_tournament);

  _main_module = _tournament;
  _main_module->SetData (nullptr,
                         "application",
                         this);
  Application::Start (argc,
                      argv,
                      _tournament);

  if (argc > 1)
  {
    _tournament->Start (g_strdup (argv[1]));
  }
  else
  {
    _tournament->Start (nullptr);
  }

  gtk_main ();

  Object::DumpList ();
}

// --------------------------------------------------------------------------------
void BellPouleApp::OnQuit (GtkWindow *window)
{
  GtkWidget *dialog = gtk_message_dialog_new_with_markup (window,
                                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          gettext ("<b><big>Do you really want to quit BellePoule</big></b>"));

  gtk_window_set_title (GTK_WINDOW (dialog),
                        gettext ("Quit BellePoule?"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            gettext ("All the unsaved competions will be lost."));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    Application::OnQuit (window);
  }

  gtk_widget_destroy (dialog);
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new BellPouleApp (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv,
                      nullptr);

  return 0;
}
