@echo OFF

echo Stopping LightTPD...
start /WAIT /B /D "webserver\LightTPD\service" PROCESS.exe -k lighttpd.exe

echo Starting LightTPD...
start /B /D "webserver\LightTPD" lighttpd.exe
