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

#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "application/application.hpp"
#include "actors/fencer.hpp"
#include "actors/team.hpp"
#include "enlisted_referee.hpp"
#include "marshaller.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  class MarshallerApp : public Application
  {
    public:
      MarshallerApp (int *argc, char ***argv);

    private:
      Marshaller *_marshaller;

      ~MarshallerApp () override;

      void Prepare () override;

      void Start (int                   argc,
                  char                **argv,
                  Net::Ring::Listener  *ring_listener) override;
  };

  // --------------------------------------------------------------------------------
  MarshallerApp::MarshallerApp (int    *argc,
                                char ***argv)
    : Application (Net::Ring::Role::RESOURCE_MANAGER, "BellePoule2D", argc, argv)
  {
  }

  // --------------------------------------------------------------------------------
  MarshallerApp::~MarshallerApp ()
  {
    _marshaller->Release ();
  }

  // --------------------------------------------------------------------------------
  void MarshallerApp::Prepare ()
  {
    EnlistedReferee::RegisterPlayerClass ();
    Fencer::RegisterPlayerClass          ();
    Team::RegisterPlayerClass            ();

    Application::Prepare ();
  }

  // --------------------------------------------------------------------------------
  void MarshallerApp::Start (int                   argc,
                             char                **argv,
                             Net::Ring::Listener  *ring_listener)
  {
    _marshaller = new Marshaller ();

    _main_module = _marshaller;
    _main_module->SetData (nullptr,
                           "application",
                           this);

    Application::Start (argc,
                        argv,
                        _marshaller);

    _marshaller->Start ();

    gtk_main ();

    Object::DumpList ();
  }
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new Marshaller::MarshallerApp (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv,
                      nullptr);

  return 0;
}
