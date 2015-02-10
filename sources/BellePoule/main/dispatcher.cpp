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
#include "hall.hpp"

// --------------------------------------------------------------------------------
class Dispatcher : public Application
{
  public:
    Dispatcher (int *argc, char ***argv);

  private:
    virtual ~Dispatcher ();

    void Prepare ();

    void Start (int argc, char **argv);
};

// --------------------------------------------------------------------------------
Dispatcher::Dispatcher (int    *argc,
                        char ***argv)
: Application ("Dispatcher", argc, argv)
{
}

// --------------------------------------------------------------------------------
Dispatcher::~Dispatcher ()
{
  _main_module->Release ();
}

// --------------------------------------------------------------------------------
void Dispatcher::Prepare ()
{
  Application::Prepare ();
}

// --------------------------------------------------------------------------------
void Dispatcher::Start (int    argc,
                        char **argv)
{
  Hall *hall = new Hall ();

  _main_module = hall;

  Application::Start (argc,
                      argv);

  hall->Start ();

  gtk_main ();
}


// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new Dispatcher (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv);

  application->Release ();

  return 0;
}

