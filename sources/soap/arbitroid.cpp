#include <sstream>
#include <iostream>

#include "soapBellePouleProxy.h"
#include "BellePoule.nsmap"

const char server[] = "http://127.0.0.1:8080";
//const char server[] = "http://192.168.0.10:8080";

int main (int argc, char *argv[])
{
  BellePouleProxy belle_poule;
  int             status;

  belle_poule.soap_endpoint = server;

  if (belle_poule.SetMatchResult (111u,
                                  222u,
                                  333u,
                                  10u,
                                  20u,
                                  11u,
                                  21u,
                                  status) != SOAP_OK)
  {
    belle_poule.soap_stream_fault (std::cerr);
  }
}
