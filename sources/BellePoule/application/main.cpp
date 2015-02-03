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
#include "application.hpp"
#include "tournament.hpp"

// --------------------------------------------------------------------------------
class BellePoule : public Application
{
  public:
    BellePoule (int *argc, char ***argv);

  private:
    Tournament *_tournament;

    virtual ~BellePoule ();

    void Prepare ();

    void Start (int argc, char **argv);
};

// --------------------------------------------------------------------------------
BellePoule::BellePoule (int *argc, char ***argv)
: Application (argc, argv)
{
  _tournament = NULL;
}

// --------------------------------------------------------------------------------
BellePoule::~BellePoule ()
{
  _tournament->Release ();
}

// --------------------------------------------------------------------------------
void BellePoule::Prepare ()
{
  _tournament = Tournament::New ("BellePoule");

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

  WifiCode::SetPort (35830);

  Application::Prepare ();
}

// --------------------------------------------------------------------------------
void BellePoule::Start (int argc, char **argv)
{
  {
    if (argc > 1)
    {
      _tournament->Start (g_strdup (argv[1]));
    }
    else
    {
      _tournament->Start (NULL);
    }

    People::Splitting::SetHostTournament (_tournament);

    gtk_main ();
  }

  _tournament->Release ();
}


// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new BellePoule (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv);

  application->Release ();

  return 0;
}
