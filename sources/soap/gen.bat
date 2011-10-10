soap\wsdl2h -nsoap -y -o output/BellePoule.h -s BellePoule.wsdl
soap\soapcpp2.exe -i -doutput -L output/BellePoule.h
