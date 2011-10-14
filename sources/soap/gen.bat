soap\wsdl2h -nsoap -y -o output/BellePoule.h -s BellePoule.wsdl ScoringSystem.wsdl
soap\soapcpp2.exe -i -doutput -L output/BellePoule.h
