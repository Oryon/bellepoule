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
#include "actors/referee.hpp"
#include "marshaller.hpp"

// --------------------------------------------------------------------------------
class MarshallerApp : public Application
{
  public:
    MarshallerApp (int *argc, char ***argv);

  private:
    Marshaller *_marshaller;

    virtual ~MarshallerApp ();

    void Prepare ();

    void Start (int argc, char **argv);

    gboolean OnHttpPost (Net::Message *message);
};

// --------------------------------------------------------------------------------
MarshallerApp::MarshallerApp (int    *argc,
                              char ***argv)
  : Application ("Marshaller", 35840, argc, argv)
{
}

// --------------------------------------------------------------------------------
MarshallerApp::~MarshallerApp ()
{
  _main_module->Release ();
}

// --------------------------------------------------------------------------------
void MarshallerApp::Prepare ()
{
  Referee::RegisterPlayerClass ();

  Application::Prepare ();
}

// --------------------------------------------------------------------------------
void MarshallerApp::Start (int    argc,
                           char **argv)
{
  _marshaller = new Marshaller ();

  _main_module = _marshaller;

  Application::Start (argc,
                      argv);

  _marshaller->Start ();

  gtk_main ();
}

// --------------------------------------------------------------------------------
gboolean MarshallerApp::OnHttpPost (Net::Message *message)
{
  if (Application::OnHttpPost (message) == FALSE)
  {
    _marshaller->OnHttpPost (message);

    return FALSE;
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new MarshallerApp (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv);

  application->Release ();

  return 0;
}
