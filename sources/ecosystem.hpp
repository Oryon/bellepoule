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

#ifndef ecosystem_hpp
#define ecosystem_hpp

#include "object.hpp"

class Tournament;
class Ecosystem : public Object
{
  public:
    static Ecosystem *Get ();

    int GetCompetitionData (unsigned int   competition_id,
                            char          *data_name,
                            char         *&competition_data)

  private:
    static Ecosystem *_ecosystem;
    Tournament       *_tournament;

    Ecosystem (Tournament *tournament);

    ~Ecosystem ();
};

#endif
