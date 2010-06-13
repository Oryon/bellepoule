@echo off
set BELLEPOULE=INSTALL_DIR
set PATH=%BELLEPOULE%;%BELLEPOULE%\porting_layer\lib;%BELLEPOULE%\porting_layer\lib\gtk-2.0\2.10.0\loaders
cd %BELLEPOULE%
start bellepoule %1
