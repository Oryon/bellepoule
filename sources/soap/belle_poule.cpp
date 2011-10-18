#include <stdio.h>
#include <stdsoap2.h>

#include "soapBellePouleService.h"
#include "BellePoule.nsmap"

int main (int argc, char **argv)
{
  BellePouleService belle_poule;

  belle_poule.run (8080);
  return 0;
}

// --------------------------------------------------------------------------------
int BellePouleService::GetCompetitionData (unsigned int   CompetitionId,
                                           char          *DataName,
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
