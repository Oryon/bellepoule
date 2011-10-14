#include <sstream>
#include <iostream>
#include <windows.h>

#include "soapBellePouleProxy.h"
#include "BellePoule.nsmap"

#include "soapScoringSystemService.h"

const char server[] = "http://127.0.0.1:8080";
//const char server[] = "http://192.168.0.10:8080";

int main (int argc, char *argv[])
{
#if 0
  BellePouleProxy belle_poule;

  belle_poule.soap_endpoint = server;

  // Competition data
  {
    char *competition_data;

    if (belle_poule.GetCompetitionData (111u,
                                        "club",
                                        competition_data) != SOAP_OK)
    {
      belle_poule.soap_stream_fault (std::cerr);
    }
    else
    {
      printf ("=== > %s\n", competition_data);
    }
  }

  // Player data
  {
    char *player_data;

    if (belle_poule.GetPlayerData (111u,
                                   35830,
                                   "club",
                                   player_data) != SOAP_OK)
    {
      belle_poule.soap_stream_fault (std::cerr);
    }
    else
    {
      printf ("=== > %s\n", player_data);
    }
  }

  // Match result
  {
    soap1__MatchResult match_result;
    int                status;

    match_result.fencerAId = 888;
    match_result.fencerBId = 777;
    match_result.fencerAScore = 15;
    match_result.fencerBScore = 9;

    if (belle_poule.SetMatchResult (111u,
                                    222u,
                                    333u,
                                    &match_result,
                                    status) != SOAP_OK)
    {
      belle_poule.soap_stream_fault (std::cerr);
    }
  }
#endif

  {
    ScoringSystemService scoring_system;

    scoring_system.run (8080);
    Sleep (10000);
  }
}

int ScoringSystemService::SetPoolMatchs (unsigned int  competitionId,
                                         unsigned int  roundId,
                                         unsigned int  matchId,
                                         unsigned int  fencerAId,
                                         unsigned int  fencerBId,
                                         int          &status)
{
  printf ("Feuille de match\n");
  return SOAP_OK;
}
