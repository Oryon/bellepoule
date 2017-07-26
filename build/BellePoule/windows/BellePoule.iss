; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#include "..\..\..\sources\BellePoule\application\version.h"

#define MINGW     ".\crossroad"
#define WEBSERVER "..\..\..\..\webserver"
#define ICONS     ".\icons"
#define PRODUCT   "bellepoulebeta"
;#define PUBLIC_VERSION

[Setup]
AppName={#PRODUCT}
AppVerName={#PRODUCT} version {#VERSION}.{#VERSION_REVISION}{#VERSION_MATURITY}
OutputBaseFilename=setup{#VERSION}_{#VERSION_REVISION}{#VERSION_MATURITY}
AppPublisher=betton.escrime
AppPublisherURL=http://betton.escrime.free.fr/
AppSupportURL=http://betton.escrime.free.fr/
AppUpdatesURL=http://betton.escrime.free.fr/
UsePreviousAppDir=no
DefaultDirName={code:GetInstallDir}\{#PRODUCT}
DefaultGroupName={#PRODUCT}
AllowNoIcons=yes
LicenseFile=COPYING.txt
Compression=lzma
SolidCompression=yes
ChangesEnvironment=yes
ChangesAssociations=true
PrivilegesRequired=none
WizardImageFile=BellePoule.bmp
WizardSmallImageFile=BellePoule_small.bmp
SetupIconFile=setup_logo.ico

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Registry]
#ifdef PUBLIC_VERSION
Root: HKCU; Subkey: "Software\Classes\.cotcot"; ValueType: string; ValueName: ""; ValueData: "BellePoule"; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: "Software\Classes\.fff"; ValueType: string; ValueName: ""; ValueData: "BellePoule"; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: "Software\Classes\BellePoule"; ValueType: string; ValueName: ""; ValueData: "BellePoule"; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: "Software\Classes\BellePoule\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\{#PRODUCT}-supervisor.exe,0"; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: "Software\Classes\BellePoule\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#PRODUCT}-supervisor.exe"" ""%1"""; Flags: uninsdeletekey noerror

Root: HKLM; Subkey: "Software\Classes\.cotcot"; ValueType: string; ValueName: ""; ValueData: "BellePoule"; Flags: uninsdeletekey noerror
Root: HKLM; Subkey: "Software\Classes\.fff"; ValueType: string; ValueName: ""; ValueData: "BellePoule"; Flags: uninsdeletekey noerror
Root: HKLM; Subkey: "Software\Classes\BellePoule"; ValueType: string; ValueName: ""; ValueData: "BellePoule"; Flags: uninsdeletekey noerror
Root: HKLM; Subkey: "Software\Classes\BellePoule\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\{#PRODUCT}-supervisor.exe,0"; Flags: uninsdeletekey noerror
Root: HKLM; Subkey: "Software\Classes\BellePoule\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#PRODUCT}-supervisor.exe"" ""%1"""; Flags: uninsdeletekey noerror
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "greek"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "nepali"; MessagesFile: "compiler:Languages\Nepali.islu"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "serbiancyrillic"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "serbianlatin"; MessagesFile: "compiler:Languages\SerbianLatin.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Icons]
Name: "{userprograms}\{#PRODUCT}\bellepoule-2D (beta)"; Filename: "{app}\bin\{#PRODUCT}-marshaller.exe"; IconFilename: "{app}\share\{#PRODUCT}\resources\marshaller.ico"
Name: "{userprograms}\{#PRODUCT}\bellepoule (beta)"; Filename: "{app}\bin\{#PRODUCT}-supervisor.exe"; IconFilename: "{app}\share\{#PRODUCT}\resources\supervisor.ico"
Name: "{userprograms}\{#PRODUCT}\Uninstall {#PRODUCT}"; Filename: "{uninstallexe}"
Name: "{commonprograms}\{#PRODUCT}\bellepoule-2D (beta)"; Filename: "{app}\bin\{#PRODUCT}-marshaller.exe"; IconFilename: "{app}\share\{#PRODUCT}\resources\marshaller.ico"
Name: "{commonprograms}\{#PRODUCT}\bellepoule (beta)"; Filename: "{app}\bin\{#PRODUCT}-supervisor.exe"; IconFilename: "{app}\share\{#PRODUCT}\resources\supervisor.ico"
Name: "{commonprograms}\{#PRODUCT}\Uninstall {#PRODUCT}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\bellepoule-2D (beta)"; Filename: "{app}\bin\{#PRODUCT}-marshaller.exe"; Tasks: desktopicon; IconFilename: "{app}\share\{#PRODUCT}\resources\marshaller.ico"
Name: "{userdesktop}\bellepoule (beta)"; Filename: "{app}\bin\{#PRODUCT}-supervisor.exe"; Tasks: desktopicon; IconFilename: "{app}\share\{#PRODUCT}\resources\supervisor.ico"
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\bellepoule-2D (beta)"; Filename: "{app}\bin\{#PRODUCT}-marshaller.exe"; Tasks: quicklaunchicon; IconFilename: "{app}\share\{#PRODUCT}\resources\marshaller.ico"
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\bellepoule (beta)"; Filename: "{app}\bin\{#PRODUCT}-supervisor.exe"; Tasks: quicklaunchicon; IconFilename: "{app}\share\{#PRODUCT}\resources\supervisor.ico"

[Files]
Source: "exe\bellepoulebeta-marshaller.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "exe\bellepoulebeta-supervisor.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\..\resources\gtkrc"; DestDir: "{app}\share\{#PRODUCT}\resources"; Flags: ignoreversion
Source: "path_dependent_files\gtk.immodules"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion; AfterInstall: UpdatePath(ExpandConstant('{app}\etc\gtk-2.0\gtk.immodules'), 'INSTALL_DIR', ExpandConstant('{app}'), 0)
Source: "path_dependent_files\gdk-pixbuf.loaders"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion; AfterInstall: UpdatePath(ExpandConstant('{app}\etc\gtk-2.0\gdk-pixbuf.loaders'), 'INSTALL_DIR', ExpandConstant('{app}'), 0)
Source: "path_dependent_files\pango.modules"; DestDir: "{app}\etc\pango"; Flags: ignoreversion; AfterInstall: UpdatePath(ExpandConstant('{app}\etc\pango\pango.modules'), 'INSTALL_DIR', ExpandConstant('{app}'), 0)

;Exemple de fichiers
Source: "..\..\..\Exemples\exemple.cotcot"; DestDir: "{app}\share\{#PRODUCT}\Exemples";             Flags: ignoreversion
Source: "..\..\..\Exemples\FFE\*";          DestDir: "{app}\share\{#PRODUCT}\Exemples\FFE";         Flags: ignoreversion
Source: "..\..\..\Exemples\Classements\*";  DestDir: "{app}\share\{#PRODUCT}\Exemples\Classements"; Flags: ignoreversion

;Documentation
Source: "..\..\..\resources\translations\user_manual.pdf"; DestDir: "{app}\share\{#PRODUCT}\resources\translations"; Flags: ignoreversion

;Traductions
Source: "..\..\..\resources\translations\index.txt"; DestDir: "{app}\share\{#PRODUCT}\resources\translations"; Flags: ignoreversion

Source: "..\..\..\resources\translations\fr\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\fr"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\de\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\de"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\nl\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\nl"; Flags: ignoreversion recursesubdirs
;Source: "..\..\..\resources\translations\pl\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\pl"; Flags: ignoreversion recursesubdirs
;Source: "..\..\..\resources\translations\ca\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\ca"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\ru\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\ru"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\ar\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\ar"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\es\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\es"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\it\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\it"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\ko\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\ko"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\pt\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\pt"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\pt_br\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\pt_br"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\sv\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\sv"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\translations\ja\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\ja"; Flags: ignoreversion recursesubdirs
;Source: "..\..\..\resources\translations\hu\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\hu"; Flags: ignoreversion recursesubdirs
;Source: "..\..\..\resources\translations\sl\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\sl"; Flags: ignoreversion recursesubdirs
;Source: "..\..\..\resources\translations\eu\*"; DestDir: "{app}\share\{#PRODUCT}\resources\translations\eu"; Flags: ignoreversion recursesubdirs

#ifdef MINGW
;WebServer
[Dirs]
Name: "{app}\share\{#PRODUCT}\webserver\LightTPD\www\cotcot"; Permissions:everyone-modify

[Files]
Source: "{#WEBSERVER}\*"; DestDir: "{app}\share\{#PRODUCT}\webserver"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\sources\www\*"; DestDir: "{app}\share\{#PRODUCT}\webserver\LightTPD\www"; Flags: ignoreversion
Source: "..\..\..\scripts\wwwstart.bat"; DestDir: "{app}\share\{#PRODUCT}\scripts"; Flags: ignoreversion
Source: "..\..\..\scripts\wwwstop.bat"; DestDir: "{app}\share\{#PRODUCT}\scripts"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\fr\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\fr\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\fr\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\fr\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\fr\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\fr\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\nl\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\nl\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\nl\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\nl\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\nl\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\nl\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\pl\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\pl\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\pl\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\pl\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\pl\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\pl\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\ca\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\ca\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ca\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\ca\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ca\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\ca\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\de\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\de\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\de\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\de\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\de\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\de\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\ru\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\ru\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ru\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\ru\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ru\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\ru\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\ar\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\ar\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ar\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\ar\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ar\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\ar\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\es\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\es\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\es\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\es\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\es\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\es\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\it\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\it\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\it\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\it\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\it\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\it\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\ko\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\ko\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ko\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\ko\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ko\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\ko\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\pt\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\pt\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\pt\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\pt\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\pt\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\pt\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\pt_BR\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\pt_BR\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\pt_BR\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\pt_BR\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\pt_BR\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\pt_BR\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\sv\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\sv\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\sv\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\sv\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\sv\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\sv\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\ja\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\ja\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ja\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\ja\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\ja\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\ja\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\hu\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\hu\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\hu\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\hu\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\hu\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\hu\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\sl\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\sl\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\sl\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\sl\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\sl\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\sl\LC_MESSAGES"; Flags: ignoreversion

Source: "{#MINGW}\share\locale\eu\LC_MESSAGES\glib20.mo"; DestDir: "{app}\share\locale\eu\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\eu\LC_MESSAGES\gtk20.mo"; DestDir: "{app}\share\locale\eu\LC_MESSAGES"; Flags: ignoreversion
Source: "{#MINGW}\share\locale\eu\LC_MESSAGES\gtk20-properties.mo"; DestDir: "{app}\share\locale\eu\LC_MESSAGES"; Flags: ignoreversion
#endif

;Resources
Source: "..\..\..\resources\glade\images\*.png"; DestDir: "{app}\share\{#PRODUCT}\resources\glade\images\"; Flags: ignoreversion
Source: "..\..\..\resources\glade\images\*.gif"; DestDir: "{app}\share\{#PRODUCT}\resources\glade\images\"; Flags: ignoreversion
Source: "..\..\..\resources\glade\images\*.jpg"; DestDir: "{app}\share\{#PRODUCT}\resources\glade\images\"; Flags: ignoreversion
Source: "..\..\..\resources\glade\*.glade"; DestDir: "{app}\share\{#PRODUCT}\resources\glade\"; Flags: ignoreversion
Source: "..\..\..\resources\countries\*"; DestDir: "{app}\share\{#PRODUCT}\resources\countries\"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\resources\glade\images\supervisor.ico"; DestDir: "{app}\share\{#PRODUCT}\resources\"; Flags: ignoreversion
Source: "..\..\..\resources\glade\images\marshaller.ico"; DestDir: "{app}\share\{#PRODUCT}\resources\"; Flags: ignoreversion

Source: "..\..\..\resources\localized_data\*"; DestDir: "{app}\share\{#PRODUCT}\resources\localized_data\"; Flags: ignoreversion recursesubdirs

; GTK+ dependencies
; DLL
#ifdef MINGW
Source: "{#MINGW}\bin\libatk-1.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libcairo-2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libcrypto-10.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libcurl-4.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libffi-6.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libfontconfig-1.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libfreetype-6.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgcc_s_sjlj-1.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgcrypt-20.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgdk_pixbuf-2.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgdk-win32-2.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgio-2.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libglib-2.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgmodule-2.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgobject-2.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgoocanvas-3.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgpg-error-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libgtk-win32-2.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libharfbuzz-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libidn-11.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libintl-8.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libjasper-1.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libjpeg-8.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\liblzma-5.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libmicrohttpd-10.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libnspr4.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libpango-1.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libpangocairo-1.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libpangoft2-1.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libpangowin32-1.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libpixman-1-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libplc4.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libplds4.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libpng16-16.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libwinpthread-1.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libssh2-1.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libstdc++-6.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libtiff-5.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libxml2-2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libzmq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\nss3.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\nssutil3.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\ssl3.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\zlib1.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#MINGW}\bin\libjson-glib-1.0-0.dll"; DestDir: "{app}\bin"; Flags: ignoreversion

#endif

; theme
#ifdef MINGW
Source: "{#MINGW}\etc\gtk-2.0\gtkrc"; DestDir: "{app}\etc\gtk-2.0"; Flags: ignoreversion
Source: "{#ICONS}\share\themes\Aurora\gtk-2.0\gtkrc"; DestDir: "{app}\share\themes\Aurora\gtk-2.0"; Flags: ignoreversion
Source: "{#ICONS}\lib\gtk-2.0\2.10.0\engines\libaurora.dll"; DestDir: "{app}\lib\gtk-2.0\2.10.0\engines\"; Flags: ignoreversion
#endif

; icons
#ifdef MINGW
Source: "{#ICONS}\share\icons\hicolor\index.theme"; DestDir: "{app}\share\icons\hicolor\"; Flags: ignoreversion

Source: "{#ICONS}\share\icons\hicolor\16x16\apps\gnome-devel.png"; DestDir: "{app}\share\icons\hicolor\16x16\apps\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\24x24\apps\gnome-devel.png"; DestDir: "{app}\share\icons\hicolor\24x24\apps\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\32x32\apps\gnome-devel.png"; DestDir: "{app}\share\icons\hicolor\32x32\apps\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\48x48\apps\gnome-devel.png"; DestDir: "{app}\share\icons\hicolor\48x48\apps\"; Flags: ignoreversion

Source: "{#ICONS}\share\icons\hicolor\16x16\apps\preferences-desktop-theme.png"; DestDir: "{app}\share\icons\hicolor\16x16\apps\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\24x24\apps\preferences-desktop-theme.png"; DestDir: "{app}\share\icons\hicolor\24x24\apps\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\32x32\apps\preferences-desktop-theme.png"; DestDir: "{app}\share\icons\hicolor\32x32\apps\"; Flags: ignoreversion

Source: "{#ICONS}\share\icons\hicolor\16x16\apps\preferences-desktop-locale.png"; DestDir: "{app}\share\icons\hicolor\16x16\apps\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\24x24\apps\preferences-desktop-locale.png"; DestDir: "{app}\share\icons\hicolor\24x24\apps\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\32x32\apps\preferences-desktop-locale.png"; DestDir: "{app}\share\icons\hicolor\32x32\apps\"; Flags: ignoreversion

Source: "{#ICONS}\share\icons\hicolor\16x16\mimetypes\x-office-spreadsheet.png"; DestDir: "{app}\share\icons\hicolor\16x16\mimetypes\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\24x24\mimetypes\x-office-spreadsheet.png"; DestDir: "{app}\share\icons\hicolor\24x24\mimetypes\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\32x32\mimetypes\x-office-spreadsheet.png"; DestDir: "{app}\share\icons\hicolor\32x32\mimetypes\"; Flags: ignoreversion

Source: "{#ICONS}\share\icons\hicolor\16x16\status\software-update-available.png"; DestDir: "{app}\share\icons\hicolor\16x16\status\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\24x24\status\software-update-available.png"; DestDir: "{app}\share\icons\hicolor\24x24\status\"; Flags: ignoreversion
Source: "{#ICONS}\share\icons\hicolor\32x32\status\software-update-available.png"; DestDir: "{app}\share\icons\hicolor\32x32\status\"; Flags: ignoreversion
#endif

;
#ifdef MINGW
;Source: "{#MINGW}\etc\pango\pango.aliases"; Destdir: "{app}\etc\pango"
#endif

; loaders
#ifdef MINGW
;Source: "{#MINGW}\lib\gtk-2.0\2.10.0\loaders\*"; Destdir: "{app}\lib\gtk-2.0\2.10.0\loaders"
#endif

[Code]
//////////////////////////////////////////
function GetInstallDir(Param: String): String;
var pfdir, tmpdir: string;
begin
  pfdir  := ExpandConstant('{pf}');
  tmpdir := pfdir + '\MY_PROGRAM_TMP';

  if CreateDir(tmpdir) then
  begin
    RemoveDir(tmpdir);
    Result := pfdir;
  end
  else begin
    Result := ExpandConstant('{userdesktop}');
  end
end;

//////////////////////////////////////////
procedure UpdatePath (const file: String; const what: String; const by: String; const dos_separator: Integer);
var
  i     : Integer;
  lines : TArrayOfString;
begin

  begin
   if dos_separator = 0
    then StringChange(by, '\', '/');
  end;

  LoadStringsFromFile(file, lines);

  for i := 0 to GetArrayLength(lines)-1 do
    begin
      StringChange(lines[i], what, by);
    end;

  SaveStringsToFile(file, lines, False);
end;
