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

#include "util/wifi_code.hpp"
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

      virtual ~MarshallerApp ();

      void Prepare ();

      void Start (int argc, char **argv);

      gboolean OnHttpPost (Net::Message *message);

      gchar *GetSecretKey (const gchar *authentication_scheme);
  };

  // --------------------------------------------------------------------------------
  MarshallerApp::MarshallerApp (int    *argc,
                                char ***argv)
    : Application ("Marshaller", "BellePoule2D", 35840, argc, argv)
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
    EnlistedReferee::RegisterPlayerClass ();
    Fencer::RegisterPlayerClass          ();
    Team::RegisterPlayerClass            ();

    Application::Prepare ();
  }

  // --------------------------------------------------------------------------------
  void MarshallerApp::Start (int    argc,
                             char **argv)
  {
    _marshaller = new Marshaller ();

    WifiCode::SetIpPort (35840);

    _main_module = _marshaller;
    _main_module->SetData (NULL,
                           "application",
                           this);

    Application::Start (argc,
                        argv);

    _marshaller->Start ();

    gtk_main ();
  }

  // --------------------------------------------------------------------------------
  gboolean MarshallerApp::OnHttpPost (Net::Message *message)
  {
    return _marshaller->OnHttpPost (message);
  }

  // --------------------------------------------------------------------------------
  gchar *MarshallerApp::GetSecretKey (const gchar *authentication_scheme)
  {
    if (_marshaller)
    {
      return _marshaller->GetSecretKey (authentication_scheme);
    }

    return NULL;
  }
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new Marshaller::MarshallerApp (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv);

  application->Release ();

  Object::DumpList ();

  return 0;
}
