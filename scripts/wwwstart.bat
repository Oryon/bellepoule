@echo OFF

echo Stopping LightTPD...
F:\BellePoule\bazaar\trunk\windows\LightTPD\service\PROCESS.exe -k lighttpd.exe

echo Starting LightTPD...
START "LightTPD" /B /D "F:\BellePoule\bazaar\trunk\windows\LightTPD" F:\BellePoule\bazaar\trunk\windows\LightTPD\lighttpd.exe
