@echo off
set BELLEPOULE=INSTALL_DIR
set PATH=%BELLEPOULE%;%BELLEPOULE%\lib;%BELLEPOULE%\lib\gtk-2.0\2.10.0\loaders
start /D"%BELLEPOULE%" bellepoule %1
