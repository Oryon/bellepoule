@echo OFF

echo Stopping LightTPD...
start /WAIT /B /D "webserver\LightTPD\service" PROCESS.exe -k lighttpd.exe
