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

#include "people_management/barrage.hpp"
#include "people_management/checkin_supervisor.hpp"
#include "people_management/general_classification.hpp"
#include "people_management/splitting.hpp"
#include "people_management/fencer.hpp"
#include "people_management/referee.hpp"
#include "people_management/team.hpp"
#include "pool_round/pool_allocator.hpp"
#include "pool_round/pool_supervisor.hpp"
#include "table_round/table_supervisor.hpp"
#include "util/wifi_code.hpp"
#include "application/application.hpp"
#include "application/contest.hpp"
#include "application/tournament.hpp"

// --------------------------------------------------------------------------------
class BellPouleApp : public Application
{
  public:
    BellPouleApp (int *argc, char ***argv);

  private:
    virtual ~BellPouleApp ();

    void Prepare ();

    void Start (int argc, char **argv);
};

// --------------------------------------------------------------------------------
BellPouleApp::BellPouleApp (int    *argc,
                            char ***argv)
  : Application ("BellePoule", argc, argv)
{
}

// --------------------------------------------------------------------------------
BellPouleApp::~BellPouleApp ()
{
  _main_module->Release ();
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
void BellPouleApp::Start (int    argc,
                          char **argv)
{
  Tournament *tournament = new Tournament ();

  WifiCode::SetPort (35830);
  People::Splitting::SetHostTournament (tournament);

  _main_module = tournament;
  Application::Start (argc,
                      argv);

  if (argc > 1)
  {
    tournament->Start (g_strdup (argv[1]));
  }
  else
  {
    tournament->Start (NULL);
  }

  gtk_main ();
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new BellPouleApp (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv);

  application->Release ();

  return 0;
}
