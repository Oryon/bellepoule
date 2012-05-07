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

#include "soapBellePouleService.h"
#include "BellePoule.nsmap"

#include "network.hpp"

// --------------------------------------------------------------------------------
Network::Network ()
{
  GError *error = NULL;

  if (!g_thread_create ((GThreadFunc) ThreadFunction,
                        this,
                        FALSE,
                        &error))
  {
    g_printerr ("Failed to create Network thread: %s\n", error->message);
    g_free (error);
  }
}

// --------------------------------------------------------------------------------
Network::~Network ()
{
}

// --------------------------------------------------------------------------------
gpointer Network::ThreadFunction (Network *network)
{
  BellePouleService belle_poule;

  belle_poule.run (8080);

  return NULL;
}

// --------------------------------------------------------------------------------
int BellePouleService::GetCompetitionData (unsigned int   competition_id,
                                           char          *data_name,
                                           char         *&competition_data)
{
  competition_data = "Championnat de Bretagne";
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
                                       soap1__MatchResult *result,
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
