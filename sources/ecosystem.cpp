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

#include <stdio.h>
#include <stdsoap2.h>
#include "soapBellePouleService.h"

#include "tournament.hpp"

#include "ecosystem.hpp"

Ecosystem *Ecosystem::_ecosystem = NULL;

// --------------------------------------------------------------------------------
Ecosystem::Ecosystem (Tournament *tournament)
: Object ("Ecosystem")
{
  BellePouleService belle_poule;

  belle_poule.run (8080);

  _tournament = tournament;
}

// --------------------------------------------------------------------------------
Ecosystem::~Ecosystem ()
{
}

// --------------------------------------------------------------------------------
Ecosystem Ecosystem::Get ()
{
  if (_ecosystem == NULL)
  {
    _ecosystem = new Ecosystem ();
  }
  return _ecosystem;
}

// --------------------------------------------------------------------------------
int BellePouleService::GetCompetitionData (unsigned int   competition_id,
                                           char          *data_name,
                                           char         *&competition_data)
{
  Ecosystem *ecosystem = Ecosystem::Get ();

  ecosystem->GetCompetitionData (CompetitionId,
                                 DataName,
                                 competition_data);

  return SOAP_OK;
}

// --------------------------------------------------------------------------------
int BellePouleService::GetPlayerData (unsigned int   CompetitionId,
                                      unsigned int   PlayerId,
                                      char          *DataName,
                                      char         *&player_data)
{
  player_data = "Le Roux";
  return SOAP_OK;
}

// --------------------------------------------------------------------------------
int BellePouleService::SetMatchResult (unsigned int        competition_id,
                                       unsigned int        round_id,
                                       unsigned int        match_id,
                                       soap2__MatchResult *result,
                                       int                &status)
{
  status = 0;

  printf ("competition_id %d\n", competition_id);
  printf ("round_id       %d\n", round_id);
  printf ("match_id       %d\n", match_id);
  printf ("fencerA_id     %d\n", result->fencerAId);
  printf ("fencerB_id     %d\n", result->fencerBId);
  printf ("fencerA_score  %d\n", result->fencerAScore);
  printf ("fencerB_score  %d\n", result->fencerBScore);

  return SOAP_OK;
}
