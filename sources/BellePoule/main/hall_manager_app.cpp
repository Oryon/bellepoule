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

#include "application/application.hpp"
#include "people_management/referee.hpp"
#include "hall_manager.hpp"

// --------------------------------------------------------------------------------
class HallManagerApp : public Application
{
  public:
    HallManagerApp (int *argc, char ***argv);

  private:
    HallManager *_hall_manager;

    virtual ~HallManagerApp ();

    void Prepare ();

    void Start (int argc, char **argv);

    gboolean OnHttpPost (Net::Message *message);
};

// --------------------------------------------------------------------------------
HallManagerApp::HallManagerApp (int    *argc,
                                char ***argv)
  : Application ("HallManager", 35840, argc, argv)
{
}

// --------------------------------------------------------------------------------
HallManagerApp::~HallManagerApp ()
{
  _main_module->Release ();
}

// --------------------------------------------------------------------------------
void HallManagerApp::Prepare ()
{
  Referee::RegisterPlayerClass ();

  Application::Prepare ();
}

// --------------------------------------------------------------------------------
void HallManagerApp::Start (int    argc,
                            char **argv)
{
  _hall_manager = new HallManager ();

  _main_module = _hall_manager;

  Application::Start (argc,
                      argv);

  _hall_manager->Start ();

  gtk_main ();
}

// --------------------------------------------------------------------------------
gboolean HallManagerApp::OnHttpPost (Net::Message *message)
{
  if (Application::OnHttpPost (message) == FALSE)
  {
    _hall_manager->OnHttpPost (message);

    return FALSE;
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new HallManagerApp (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv);

  application->Release ();

  return 0;
}
