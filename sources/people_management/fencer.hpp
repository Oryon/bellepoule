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

#ifndef fencer_hpp
#define fencer_hpp

#include "common/player.hpp"
#include "team.hpp"

class Fencer : public Player
{
  public:
    static void RegisterPlayerClass ();

    static Player *CreateInstance ();

    const gchar *GetXmlTag ();

    void SetTeam (Team *team);

    Team *GetTeam ();

  private:
    static const gchar *_class_name;
    static const gchar *_xml_tag;
    Team               *_team;

    Fencer ();

    virtual ~Fencer ();

    Player *Clone ();
};

#endif
