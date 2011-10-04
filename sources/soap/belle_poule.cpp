#include <stdio.h>
#include <stdsoap2.h>

#include "soapBellePouleProxy.h"
#include "soapBellePouleService.h"
#include "BellePoule.nsmap"

int main (int argc, char **argv)
{
  BellePouleService belle_poule;

  belle_poule.run (8080);
  return 0;
}

int BellePouleService::SetMatchResult (unsigned int  competition_id,
                                       unsigned int  round_id,
                                       unsigned int  match_id,
                                       unsigned int  fencerA_id,
                                       unsigned int  fencerB_id,
                                       unsigned int  fencerA_score,
                                       unsigned int  fencerB_score,
                                       int                &status)
{
  status = 0;

  printf ("competition_id %d\n", competition_id);
  printf ("round_id       %d\n", round_id);
  printf ("match_id       %d\n", match_id);
  printf ("fencerA_id     %d\n", fencerA_id);
  printf ("fencerB_id     %d\n", fencerB_id);
  printf ("fencerA_score  %d\n", fencerA_score);
  printf ("fencerB_score  %d\n", fencerB_score);

  return SOAP_OK;
}
