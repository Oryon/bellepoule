#----------------------------------------------------
#-- Variables ---------------------------------------
#----------------------------------------------------

ifneq ($(wildcard ./.DISTRIB),"")
DISTRIB = $(shell cat ./.DISTRIB)
endif
ifneq ($(wildcard ./.V),"")
V = $(shell cat ./.V)
endif

ifeq (,$(wildcard ../../sources/BellePoule/application/version.h))
	VERSION_FILE = "sources/BellePoule/application/version.h"
else
	VERSION_FILE = "../../sources/BellePoule/application/version.h"
endif
VERSION_MAJOR:=$(shell grep "VERSION " $(VERSION_FILE) | sed 's/.* //')
VERSION_MINOR:=$(shell grep VERSION_REVISION $(VERSION_FILE) | sed 's/.* //')
VERSION_MATURITY:=$(shell grep VERSION_MATURITY $(VERSION_FILE) | sed 's/.* //')
ifneq ($(DISTRIB), windows)
	ifneq ($(VERSION_MATURITY), )
		VERSION_MICRO := $(shell echo $(VERSION_MATURITY) | sed 's/alpha0//' | sed 's/alpha//')
	else
		VERSION_MICRO := "1"
	endif
endif

PRODUCT      = bellepoulebeta
#PRODUCT      = bellepoule
OUTPUT       = Release
#OUTPUT       = Debug
PACKAGE      = $(HOME)/Project/PPA/$(PRODUCT)
VERSION      = $(VERSION_MAJOR).$(VERSION_MINOR)
RELEASE      = $(PRODUCT)_$(VERSION)
LONG_VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)
DATE         = $(shell date -R)
OSX_CROSS    = $(HOME)/Project/Cross/Osxcross/target/macports/pkgs/opt/local

GTK = gtk+-2.0
GOO = goocanvas
#GTK = gtk+-3.0
#GOO = goocanvas-2.0

#----------------------------------------------------
#-- WEBKIT  -----------------------------------------
#----------------------------------------------------
DWEBKIT    = -DWEBKIT
WEBKIT_LIB = webkit-1.0
ifeq ($(DISTRIB), osx)
	undefine DWEBKIT
	undefine WEBKIT_LIB
endif

#----------------------------------------------------
#-- Compilers ---------------------------------------
#----------------------------------------------------
ifeq ($(DISTRIB), windows)
CC_FAMILY = i686-w64-mingw32-
EXE       = .exe
MINGW=$(HOME)/Project/Cross/Xroad/w32/$(PRODUCT)
PKG_CONFIG = env PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR=$(MINGW)/lib/pkgconfig $(CC_FAMILY)pkg-config
else
CC_FAMILY  =
EXE        =
PKG_CONFIG = pkg-config
endif

WINDRES = $(CC_FAMILY)windres
CC      = $(CC_FAMILY)g++
AR      = $(CC_FAMILY)ar

ifeq ($(DISTRIB), osx)
PKG_CONFIG = x86_64-apple-darwin15-pkg-config
CC  = o64-clang++
#CC += -arch i386
CC += -arch x86_64
endif

ifeq ($(OUTPUT), Debug)
CC += -g
else
CC += -O2
endif

#----------------------------------------------------
#-- CFLAGS ------------------------------------------
#----------------------------------------------------
CFLAGS  =
CFLAGS += -pedantic
CFLAGS += -std=c++11
CFLAGS += -DGTK_DISABLE_SINGLE_INCLUDES
CFLAGS += -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES
CFLAGS += -DGSEAL_ENABLE
CFLAGS += $(DWEBKIT)
#CFLAGS += -DGDK_DISABLE_DEPRECATED
#CFLAGS += -DGTK_DISABLE_DEPRECATED

CFLAGS += -Wall
CFLAGS += -Wcast-align
CFLAGS += -Wextra
CFLAGS += -Winit-self
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wredundant-decls
CFLAGS += -Wshadow
CFLAGS += -Wswitch-default
CFLAGS += -Wswitch-enum
CFLAGS += -Wundef
CFLAGS += -Wunreachable-code

CFLAGS += -Wno-int-to-pointer-cast
CFLAGS += -Wno-long-long
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-variadic-macros
CFLAGS += -Wno-vla
ifeq ($(OUTPUT), Debug)
CFLAGS += -DDEBUG=1
endif

ifeq ($(DISTRIB), windows)
CFLAGS += -DWINDOWS_TEMPORARY_PATCH=1
CFLAGS += -DWIN32
CFLAGS += -I$(MINGW)/include
else ifeq ($(DISTRIB), osx)
CFLAGS += -DOSX
CFLAGS += -Wno-cast-align
CFLAGS += -Wno-overloaded-virtual
CFLAGS += -Wno-unused-private-field
CFLAGS += -Wno-format-security
else
CFLAGS += -fPIC
endif

CFLAGS += $(shell $(PKG_CONFIG) --cflags $(GTK) glib-2.0 gthread-2.0 gmodule-2.0 libxml-2.0 $(GOO) libcurl libmicrohttpd libqrencode json-glib-1.0 $(WEBKIT_LIB))

CFLAGS += -Isources/common
CFLAGS += -Isources/common/network
CFLAGS += -Isources/BellePoule

#----------------------------------------------------
#-- LDFLAGS -----------------------------------------
#----------------------------------------------------
LDFLAGS  = -lstdc++
LDFLAGS += -lssl
LDFLAGS += -lcrypto
LDFLAGS += -L$(OUTPUT)

ifeq ($(DISTRIB), windows)
LDFLAGS += -lws2_32
LDFLAGS += -liphlpapi
ifeq ($(OUTPUT), Release)
LDFLAGS += -mwindows
endif
endif
LDFLAGS += $(shell $(PKG_CONFIG) --libs $(GTK) glib-2.0 gthread-2.0 gmodule-2.0 libxml-2.0 $(GOO) libcurl libmicrohttpd libqrencode json-glib-1.0 $(WEBKIT_LIB))

ifeq ($(DISTRIB), osx)
LDFLAGS += -headerpad_max_install_names
LDFLAGS += -framework CoreFoundation
else ifeq ($(OUTPUT), Release)
LDFLAGS += -s
endif

#----------------------------------------------------
#-- Sources -----------------------------------------
#----------------------------------------------------
SRC  = $(wildcard sources/common/network/*.cpp)
SRC += $(wildcard sources/common/network/oauth/*.cpp)
SRC += $(wildcard sources/common/network/twitter/*.cpp)
SRC += $(wildcard sources/common/network/facebook/*.cpp)
SRC += $(wildcard sources/common/util/*.cpp)
SRC += $(wildcard sources/BellePoule/actors/*.cpp)
SRC += $(wildcard sources/BellePoule/application/*.cpp)
OBJ_LIB += $(SRC:.cpp=.o)

ifeq ($(DISTRIB), bionic)
GOOCANVAS_LIB     = sources/goocanvas/src/.libs/libgoocanvas.so
install_goocanvas = install_goocanvas
else
GOOCANVAS_LIB     =
install_goocanvas =
endif

SRC_MARSHALLER = $(wildcard sources/BellePoule/marshaller/*.cpp)
OBJ_MARSHALLER = $(SRC_MARSHALLER:.cpp=.o)
ifeq ($(DISTRIB), windows)
OBJ_MARSHALLER += sources/BellePoule/marshaller/marshaller-ico.o
endif

SRC_SUPERVISOR  = $(wildcard sources/BellePoule/supervisor/*.cpp)
SRC_SUPERVISOR += $(wildcard sources/BellePoule/supervisor/*/*.cpp)
SRC_SUPERVISOR += $(wildcard sources/BellePoule/supervisor/*/*/*.cpp)
SRC_SUPERVISOR += $(wildcard sources/BellePoule/supervisor/*/*/*/*.cpp)
OBJ_SUPERVISOR = $(SRC_SUPERVISOR:.cpp=.o)
ifeq ($(DISTRIB), windows)
OBJ_SUPERVISOR += sources/BellePoule/supervisor/supervisor-ico.o
endif

ifeq ($(DISTRIB), windows)
SHARED_LIB =
RPATH      =
STATIC_LIB = $(OUTPUT)/lib$(PRODUCT).a

else
ifeq ($(DISTRIB), fedora)
LIB_FOLDER = $(shell rpm --eval '%{_libdir}')
else
LIB_FOLDER = /usr/lib
endif

ifeq ($(DISTRIB), osx)
SHARED_OPT = -dynamiclib
SHARED_LIB = $(OUTPUT)/lib$(PRODUCT).dylib
else
SHARED_OPT = -shared
SHARED_LIB = $(OUTPUT)/lib$(PRODUCT).so
endif

STATIC_LIB =
RPATH      = -Wl,-rpath,$(LIB_FOLDER)/$(PRODUCT)
endif

#----------------------------------------------------
#-- Targets -----------------------------------------
#----------------------------------------------------
all: output sources/goocanvas $(OUTPUT)/$(PRODUCT)-marshaller$(EXE) $(OUTPUT)/$(PRODUCT)-supervisor$(EXE)

sources/goocanvas:
	tar xvjf goocanvas.tgz
	cd sources/goocanvas && make

output:
	@mkdir -p $(OUTPUT)

#--

$(SHARED_LIB): $(OBJ_LIB)
	@echo -e "\e[1;31m" $@ "\e[0m"
	@$(CC) $(SHARED_OPT) -o $@ $^ $(LDFLAGS)

$(STATIC_LIB): $(OBJ_LIB)
	@echo -e "\e[1;31m" $@ "\e[0m"
	@$(AR) rcs $@ $^

$(OUTPUT)/$(PRODUCT)-supervisor$(EXE): $(SHARED_LIB) $(STATIC_LIB) $(OBJ_SUPERVISOR)
	@echo -e "\e[1;31m" $@ "\e[0m"
	@$(CC) -o $@ $(OBJ_SUPERVISOR) $(RPATH) $(STATIC_LIB) -l$(PRODUCT) $(GOOCANVAS_LIB) $(LDFLAGS)

$(OUTPUT)/$(PRODUCT)-marshaller$(EXE): $(SHARED_LIB) $(STATIC_LIB) $(OBJ_MARSHALLER)
	@echo -e "\e[1;31m" $@ "\e[0m"
	@$(CC) -o $@ $(OBJ_MARSHALLER) $(RPATH) $(STATIC_LIB) -l$(PRODUCT) $(GOOCANVAS_LIB) $(LDFLAGS)

#--

%.o: %.cpp
	@echo -e "\e[0;0m" $(notdir $@) "\e[0m"
	@$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.rc
	@echo -e "\e[1;33m" $@ "\e[0m"
	@$(WINDRES) --output=$@ --input=$<

#--
vars:
	@echo -e "\e[1;41m**** Variables ****\e[0m"
	@echo "$(DISTRIB)"
	@echo "$(V)"
	@echo "$(VERSION)"
	@echo "$(VERSION_MATURITY)"
	@echo "$(VERSION_MICRO)"
	@echo "---------------"
	@echo $(CFLAGS)
	@echo "---------------"
	@echo $(LDFLAGS)
	@echo ""

valgrind:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --log-file=memory_leak.log --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 $(OUTPUT)/$(PRODUCT)
	#G_SLICE=always-malloc G_DEBUG=gc-friendly valkyrie $(OUTPUT)/$(PRODUCT)

#----------------------------------------------------
#-- Packaging ---------------------------------------
#----------------------------------------------------
GOOCANVAS_BUILD_DEP = libgoocanvas-dev\,
GOOCANVAS_DEP       = libgoocanvas3\,
prepare_goocanvas   = prepare_fake_goocanvas
ifeq ($(DISTRIB), windows)
prepare_resources = prepare_windows_resources
build_staging     = build_windows
publish_staging   = publish_windows
RESOURCES         = $(PACKAGE)/$(RELEASE)/resources
else ifeq ($(DISTRIB), archlinux)
prepare_linux     = prepare_arch
build_staging     = build_arch
publish_staging   = publish_arch
else ifeq ($(DISTRIB), osx)
prepare_osx       = prepare_osx
prepare_examples  = prepare_examples
prepare_resources = prepare_osx_resources
build_staging     = build_osx
publish_staging   = publish_osx
EXAMPLES          = $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/$(PRODUCT)/Exemples
RESOURCES         = $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/$(PRODUCT)/resources
else
prepare_linux     = prepare_ubuntu
prepare_www       = prepare_www
prepare_examples  = prepare_examples
prepare_resources = prepare_resources
build_staging     = build_ubuntu
publish_staging   = publish_ubuntu
EXAMPLES          = $(PACKAGE)/$(RELEASE)/Exemples
RESOURCES         = $(PACKAGE)/$(RELEASE)/resources
endif

ifeq ($(DISTRIB), bionic)
prepare_goocanvas   = prepare_goocanvas
GOO                 =
GOOCANVAS_BUILD_DEP =
GOOCANVAS_DEP       =
CFLAGS             += -Isources/goocanvas/src
endif

ifeq ($(DISTRIB), stretch)
publish_staging   = publish_debian
endif

ifeq ($(DISTRIB), fedora)
prepare_linux     = prepare_fedora
build_staging     = build_fedora
publish_staging   = publish_fedora
prepare_www       = 
endif

ifneq ("$(wildcard ./99-$(PRODUCT).conf)","")
install_lighttpd = install_lighttpd
else
install_lighttpd = 
endif

package: prepare_staging $(prepare_linux) $(prepare_osx) prepare_sources $(prepare_examples) $(prepare_resources) $(prepare_www) $(prepare_goocanvas)
	rm -rf `find $(PACKAGE) -name "*~*"`
	rm -rf `find $(PACKAGE) -name "*.swp"`
	rm -rf `find $(PACKAGE) -name ".*.swp"`
	find $(PACKAGE)/$(RELEASE) -type f -exec chmod a-x {} \;

prepare_osx:
	cp -rf osx/$(PRODUCT).app $(PACKAGE)/$(RELEASE)
	sed -i 's/__VERSION__/$(subst $\",,$(LONG_VERSION))/' $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Info.plist
	cp $(OSX_CROSS)/../relocated/*.dylib $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Frameworks
	cp $(OSX_CROSS)/../loaders/*.so $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/lib/gdk-pixbuf-2.0/2.10.0/loaders
	cp $(OSX_CROSS)/../printbackends/*.so $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/lib/gtk-2.0/2.10.0/printbackends
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/fr/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/fr/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/fr/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/nl/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/nl/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/nl/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/pl/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/pl/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/pl/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ca/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/ca/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ca/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/de/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/de/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/de/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ru/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/ru/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ru/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ar/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/ar/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ar/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/es/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/es/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/es/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/it/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/it/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/it/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ko/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/ko/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ko/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/pt/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/pt/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/pt/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/sv/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/sv/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/sv/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ja/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/ja/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/ja/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/hu/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/hu/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/hu/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/sl/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/sl/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/sl/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/eu/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/eu/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/eu/LC_MESSAGES/gtk20r.mo
	mkdir -p $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/fa/LC_MESSAGES
	cp $(OSX_CROSS)/share/locale/fa/LC_MESSAGES/gtk20.mo            $(PACKAGE)/$(RELEASE)/$(PRODUCT).app/Contents/Resources/share/locale/fa/LC_MESSAGES/gtk20r.mo

prepare_arch:
	git clone --quiet git+ssh://aur@aur.archlinux.org/$(PRODUCT).git $(PACKAGE)/$(RELEASE)/arch
	cp -rf arch/* $(PACKAGE)/$(RELEASE)/arch

prepare_fedora: prepare_debian prepare_desktop
	mkdir $(PACKAGE)/$(RELEASE)/rpm
	cp rpm/bellepoule.spec $(PACKAGE)/$(RELEASE)/rpm/$(PRODUCT).spec
	sed -i 's/__PRODUCT__/$(subst $\",,$(PRODUCT))/g' $(PACKAGE)/$(RELEASE)/rpm/$(PRODUCT).spec
	sed -i 's/__VERSION__/$(subst $\",,$(VERSION))/g' $(PACKAGE)/$(RELEASE)/rpm/$(PRODUCT).spec
	sed -i 's/__VERSION_MICRO__/$(subst $\",,$(VERSION_MICRO))/g' $(PACKAGE)/$(RELEASE)/rpm/$(PRODUCT).spec

prepare_ubuntu: prepare_debian prepare_changelog prepare_control prepare_rules prepare_copyright prepare_desktop

prepare_examples:
	mkdir -p $(EXAMPLES)
	cp -f  ../../Exemples/exemple.cotcot      $(EXAMPLES)
	mkdir -p $(EXAMPLES)/FFE
	cp -rf ../../Exemples/FFE/Individuelle.XML $(EXAMPLES)/FFE
	cp -rf ../../Exemples/FFE/ParEquipe.XML    $(EXAMPLES)/FFE
	cp -rf ../../Exemples/FFE/Fencers.FFF      $(EXAMPLES)/FFE
	cp -rf ../../Exemples/FFE/Referees.txt     $(EXAMPLES)/FFE
	mkdir -p $(EXAMPLES)/Classements
	cp -rf ../../Exemples/Classements/EHBEN.TXT $(EXAMPLES)/Classements
	cp -rf ../../Exemples/Classements/EHBEN.xml $(EXAMPLES)/Classements
	cp -rf ../../Exemples/Classements/FHBEN.xml $(EXAMPLES)/Classements
	cp -rf ../../Exemples/Classements/FHBEN.xml $(EXAMPLES)/Classements
	find $(EXAMPLES) -type f -exec chmod a-x {} \;

prepare_www:
	mkdir $(PACKAGE)/$(RELEASE)/www
	cp -rf ../../sources/www/*\.* $(PACKAGE)/$(RELEASE)/www
	rm -rf $(PACKAGE)/$(RELEASE)/www/cotcot
	cp -f  99-bellepoule.conf $(PACKAGE)/$(RELEASE)/99-$(PRODUCT).conf
	sed -i 's/bellepoule/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/99-$(PRODUCT).conf
	mkdir -p $(PACKAGE)/$(RELEASE)/scripts
	cp -f  ../../scripts/www*sh $(PACKAGE)/$(RELEASE)/scripts
	sed -i 's/bellepouledebug/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/scripts/wwwstart.sh
	sed -i 's/bellepouledebug/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/scripts/wwwstop.sh

prepare_windows_resources:
	mkdir -p $(RESOURCES)/glade/images
	cp -f ../../resources/glade/images/supervisor.ico $(RESOURCES)/glade/images
	cp -f ../../resources/glade/images/marshaller.ico $(RESOURCES)/glade/images
	ln -s $(MINGW)/bin/*dll $(PACKAGE)/$(RELEASE)
	curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem

prepare_osx_resources: prepare_resources
	cp -f ../../resources/gtkrc.osx $(RESOURCES)/gtkrc

prepare_resources:
	mkdir -p $(RESOURCES)/translations
	cp -f ../../resources/translations/user_manual.pdf $(RESOURCES)/translations
	cp -f ../../resources/translations/index.txt $(RESOURCES)/translations
	cp -rf ../../resources/translations/fr    $(RESOURCES)/translations
	cp -rf ../../resources/translations/de    $(RESOURCES)/translations
	cp -rf ../../resources/translations/nl    $(RESOURCES)/translations
#	cp -rf ../../resources/translations/pl    $(RESOURCES)/translations
	cp -rf ../../resources/translations/ru    $(RESOURCES)/translations
	cp -rf ../../resources/translations/ar    $(RESOURCES)/translations
	cp -rf ../../resources/translations/es    $(RESOURCES)/translations
#	cp -rf ../../resources/translations/ca    $(RESOURCES)/translations
	cp -rf ../../resources/translations/it    $(RESOURCES)/translations
	cp -rf ../../resources/translations/ko    $(RESOURCES)/translations
	cp -rf ../../resources/translations/pt    $(RESOURCES)/translations
	cp -rf ../../resources/translations/pt_BR $(RESOURCES)/translations
	cp -rf ../../resources/translations/sv    $(RESOURCES)/translations
	cp -rf ../../resources/translations/ja    $(RESOURCES)/translations
#	cp -rf ../../resources/translations/hu    $(RESOURCES)/translations
#	cp -rf ../../resources/translations/sl    $(RESOURCES)/translations
#	cp -rf ../../resources/translations/eu    $(RESOURCES)/translations
#	cp -rf ../../resources/translations/fa    $(RESOURCES)/translations
	mkdir -p $(RESOURCES)/glade/images
	cp -f ../../resources/glade/images/*.png   $(RESOURCES)/glade/images
	cp -f ../../resources/glade/images/*.gif   $(RESOURCES)/glade/images
	cp -f ../../resources/glade/images/*.jpg   $(RESOURCES)/glade/images
	cp -f ../../resources/glade/*.glade $(RESOURCES)/glade
	cp -rf ../../resources/countries      $(RESOURCES)/countries
	cp -rf ../../resources/localized_data $(RESOURCES)/localized_data
	find $(RESOURCES) -type f -exec chmod a-x {} \;

prepare_sources: clean_sources
	mkdir -p $(PACKAGE)/$(RELEASE)/sources
	cp -rf ../../sources/BellePoule $(PACKAGE)/$(RELEASE)/sources/BellePoule
	cp -rf ../../sources/common     $(PACKAGE)/$(RELEASE)/sources/common

prepare_staging:
	rm -rf $(PACKAGE)/*
	mkdir -p $(PACKAGE)/$(RELEASE)
	cp Makefile $(PACKAGE)/$(RELEASE)
	@echo "$(DISTRIB)" > $(PACKAGE)/$(RELEASE)/.DISTRIB
	@echo "$(V)"      > $(PACKAGE)/$(RELEASE)/.V

prepare_debian:
	cp -rf debian $(PACKAGE)/$(RELEASE)
	cp bionic/goocanvas.tgz $(PACKAGE)/$(RELEASE)

prepare_goocanvas:
	cp bionic/goocanvas.tgz $(PACKAGE)/$(RELEASE)

prepare_fake_goocanvas:
	touch $(PACKAGE)/$(RELEASE)/sources/goocanvas

prepare_changelog:
	rm -f $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "$(PRODUCT) ($(VERSION)ubuntu$(VERSION_MICRO)~$(DISTRIB)$(V)) $(DISTRIB); urgency=low" > $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "  * Miscellaneous improvements" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo " -- betonniere <bellepoule.dev@free.fr>  $(DATE)" >> $(PACKAGE)/$(RELEASE)/debian/changelog

prepare_control:
	# language-pack-gnome-fr-base  ...
	sed -i 's/__PRODUCT__/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/debian/control
	sed -i 's/__GOOCANVAS_BUILD_DEP__/$(GOOCANVAS_BUILD_DEP)/g' $(PACKAGE)/$(RELEASE)/debian/control
	sed -i 's/__GOOCANVAS_DEP__/$(GOOCANVAS_DEP)/g' $(PACKAGE)/$(RELEASE)/debian/control
ifeq ($(PRODUCT), bellepoulebeta)
	sed -i 's/Replaces/#Replaces/' $(PACKAGE)/$(RELEASE)/debian/control
endif

prepare_rules:
	sed -i 's/__PRODUCT__/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/debian/rules

prepare_copyright:
	sed -i 's/__PRODUCT__/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/debian/copyright

prepare_desktop:
	cp -f $(PACKAGE)/$(RELEASE)/debian/*.desktop.template $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-marshaller.desktop
	sed -i 's/__PRODUCT__/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-marshaller.desktop
	sed -i 's/__EXE__/marshaller/g'     $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-marshaller.desktop
	sed -i 's/__NAME__/bellepoule-2D (beta)/g'     $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-marshaller.desktop
	cp -f $(PACKAGE)/$(RELEASE)/debian/*.desktop.template $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-supervisor.desktop
	sed -i 's/__PRODUCT__/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-supervisor.desktop
	sed -i 's/__EXE__/supervisor/g'     $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-supervisor.desktop
	sed -i 's/__NAME__/bellepoule (beta)/g'    $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-supervisor.desktop
	rm -f $(PACKAGE)/$(RELEASE)/debian/*.desktop.template

build:   $(build_staging)
publish: $(publish_staging)

build_windows: all
	cp $(OUTPUT)/$(PRODUCT)*.exe ~/Project/Gtk/$(VERSION_MAJOR).0/build/BellePoule/windows/exe
	#echo "cp Debug/$(PRODUCT)-*.exe ~/Project/Gtk/$(VERSION)/build/BellePoule/windows/exe/"
	#cp ~/Project/Gtk/$(VERSION)/build/BellePoule/windows/exe/$(PRODUCT)-*.exe ~/.wine/drive_c/Program\ Files\ \(x86\)/$(PRODUCT)/bin
	env WINEPREFIX="$(HOME)/.wine" wine C:\\windows\\command\\start.exe /Unix $(HOME)/.wine/dosdevices/c:/users/Public/Start\ Menu/Programs/Inno\ Setup\ 5/Inno\ Setup\ Compiler.lnk

publish_windows:
	lftp ftp://betton.escrime:`ssh-askpass`@ftpperso.free.fr -e "put -O /documents/BellePoule/unstable ~/Project/Gtk/$(VERSION_MAJOR).0/build/BellePoule/windows/Output/$(PRODUCT)-$(VERSION_MAJOR)-$(VERSION_MINOR)-$(VERSION_MATURITY)-win.exe; quit"

build_osx: all
	cp $(OUTPUT)/$(PRODUCT)-supervisor $(PRODUCT).app/Contents/MacOS/$(PRODUCT)-supervisor
	cp $(OUTPUT)/$(PRODUCT)-marshaller $(PRODUCT).app/Contents/MacOS/$(PRODUCT)-marshaller
	mkdir $(PRODUCT).app/Contents/Resources/lib/$(PRODUCT)
	cp $(OUTPUT)/lib$(PRODUCT).dylib $(PRODUCT).app/Contents/Resources/lib/$(PRODUCT)/lib$(PRODUCT).dylib
	chmod a+x $(PRODUCT).app/Contents/MacOS/$(PRODUCT)*
	find $(PRODUCT).app/Contents/Frameworks -type f -printf "%f\n" | while read lib;\
		do \
		install_name_tool -change /opt/local/lib/$$lib @executable_path/../Frameworks/$$lib $(PRODUCT).app/Contents/MacOS/$(PRODUCT)-supervisor; \
		install_name_tool -change /opt/local/lib/$$lib @executable_path/../Frameworks/$$lib $(PRODUCT).app/Contents/MacOS/$(PRODUCT)-marshaller; \
		install_name_tool -change /opt/local/lib/$$lib @executable_path/../Frameworks/$$lib $(PRODUCT).app/Contents/Resources/lib/$(PRODUCT)/lib$(PRODUCT).dylib; \
		done
	install_name_tool -id @executable_path/../Resources/lib/$(PRODUCT)/lib$(PRODUCT).dylib $(PRODUCT).app/Contents/Resources/lib/$(PRODUCT)/lib$(PRODUCT).dylib
	install_name_tool -change $(OUTPUT)/lib$(PRODUCT).dylib @executable_path/../Resources/lib/$(PRODUCT)/lib$(PRODUCT).dylib $(PRODUCT).app/Contents/MacOS/$(PRODUCT)-supervisor
	install_name_tool -change $(OUTPUT)/lib$(PRODUCT).dylib @executable_path/../Resources/lib/$(PRODUCT)/lib$(PRODUCT).dylib $(PRODUCT).app/Contents/MacOS/$(PRODUCT)-marshaller
	zip -qr $(PRODUCT)-$(subst .,-,$(LONG_VERSION))-osx.zip $(PRODUCT).app

publish_osx:
	lftp ftp://betton.escrime:`ssh-askpass`@ftpperso.free.fr -e "put -O /documents/BellePoule/unstable $(PRODUCT)-$(subst .,-,$(LONG_VERSION))-osx.zip; quit"

build_ubuntu:
	debuild -us -uc --lintian-opts --color always

publish_ubuntu:
	debuild -S -sa -k`gpg -K | grep "sec " | sed 's/.*\///' | sed 's/ .*//'`
	dput ppa:betonniere/bellepoule ../$(PRODUCT)*_source.changes

publish_debian:
	lftp ftp://betton.escrime:`ssh-askpass`@ftpperso.free.fr -e "put -O /documents/BellePoule/unstable ../$(PRODUCT)_$(VERSION_MAJOR).$(VERSION_MINOR)ubuntu1~stretch1_amd64.deb; quit"

build_fedora:
	cd .. && tar cf $(PRODUCT)_$(VERSION).tar.gz --exclude="$(PRODUCT)_$(VERSION).tar.gz" .
	mv ../$(PRODUCT)_$(VERSION).tar.gz $(HOME)/rpmbuild/SOURCES
	cp rpm/$(PRODUCT).spec $(HOME)/rpmbuild/SPECS
	cd $(HOME)/rpmbuild/SPECS && rpmbuild -ba $(PRODUCT).spec
	cp $(HOME)/rpmbuild/SRPMS/$(PRODUCT)-$(VERSION)-$(VERSION_MICRO).fc28.src.rpm .

publish_fedora:
	lftp ftp://betton.escrime:`ssh-askpass`@ftpperso.free.fr -e "put -O /documents/BellePoule/unstable $(PRODUCT)-$(VERSION)-$(VERSION_MICRO).fc28.src.rpm; quit"

build_arch:
	python arch/make_archlinux.py $(VERSION_MAJOR) $(VERSION_MINOR) $(V) $(VERSION_MICRO)
	git -C arch add .
	git -C arch commit --quiet -m "$(VERSION)-$(VERSION_MICRO)"

publish_arch:
	git -C arch push origin master

#----------------------------------------------------
#-- debuild -----------------------------------------
#----------------------------------------------------
.PHONY: install clean_sources

install: $(install_lighttpd) $(install_goocanvas)
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)$(LIB_FOLDER)/$(PRODUCT)
	install $(OUTPUT)/$(PRODUCT)-supervisor $(DESTDIR)/usr/bin/$(PRODUCT)-supervisor
	install $(OUTPUT)/$(PRODUCT)-marshaller $(DESTDIR)/usr/bin/$(PRODUCT)-marshaller
	install $(OUTPUT)/lib$(PRODUCT).so $(DESTDIR)$(LIB_FOLDER)/$(PRODUCT)/lib$(PRODUCT).so
	mkdir -p $(DESTDIR)/usr/share/applications
	install --mode=644 debian/$(PRODUCT)-marshaller.desktop $(DESTDIR)/usr/share/applications/$(PRODUCT)-marshaller.desktop
	install --mode=644 debian/$(PRODUCT)-supervisor.desktop $(DESTDIR)/usr/share/applications/$(PRODUCT)-supervisor.desktop
	mkdir -p $(DESTDIR)/usr/share/$(PRODUCT)
	#Examples
	mkdir -p `find Exemples -type d | sed 's?Exemples?$(DESTDIR)/usr/share/$(PRODUCT)/&?'`
	find Exemples -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/$(PRODUCT)/'$$file ;\
		done
	#Resources
	mkdir -p `find resources -type d | sed 's?resources?$(DESTDIR)/usr/share/$(PRODUCT)/&?'`
	find resources -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/$(PRODUCT)/'$$file ;\
		done

install_goocanvas:
	mkdir -p $(DESTDIR)$(LIB_FOLDER)/$(PRODUCT)
	install $(GOOCANVAS_LIB)   $(DESTDIR)$(LIB_FOLDER)/$(PRODUCT)/libgoocanvas.so.3

install_lighttpd:
	mkdir -p $(DESTDIR)/etc/lighttpd/conf-available
	install --mode=644 99-$(PRODUCT).conf '$(DESTDIR)/etc/lighttpd/conf-available/'99-$(PRODUCT).conf ;
	mkdir -p `find www -type d | sed 's?www?$(DESTDIR)/usr/share/$(PRODUCT)/webserver/LightTPD/&?'`
	find www -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/$(PRODUCT)/webserver/LightTPD/'$$file ;\
		done
	mkdir -p `find scripts -type d | sed 's?scripts?$(DESTDIR)/usr/share/$(PRODUCT)/&?'`
	find scripts -type f  | while read file;\
		do \
		install --mode=755 $$file '$(DESTDIR)/usr/share/$(PRODUCT)/'$$file ;\
		done
	install --mode=1777 -d $(DESTDIR)/usr/share/$(PRODUCT)/webserver/LightTPD/www/cotcot

clean_sources:
	rm -f `find ../../sources -name "*.o"`
	rm -rf $(OUTPUT)

#----------------------------------------------------
#-- Help --------------------------------------------
#----------------------------------------------------
help: vars
	@echo -e "\e[1;44m**** Ubuntu ****\e[0m"
	@echo -e "make -s DISTRIB=xenial V=1 package"
	@echo -e "make -s DISTRIB=bionic V=1 package"
	@echo -e ""
	@echo -e "\e[1;42m**** Windows ****\e[0m"
	@echo -e "make -s DISTRIB=windows V=1 package"
	@echo -e ""
	@echo -e "\e[1;45m**** MacOs ****\e[0m"
	@echo -e "make -s DISTRIB=osx V=1 package"
	@echo -e ""
	@echo -e "\e[1;46m**** Archlinux ****\e[0m"
	@echo -e "make -s DISTRIB=archlinux V=1 package"
	@echo -e ""
	@echo -e "\e[1;41m**** Fedora (VirtualBox) ****\e[0m"
	@echo -e "make -s DISTRIB=fedora V=1 package"
	@echo -e ""
	@echo -e "\e[1;43m**** Debian (VirtualBox) ****\e[0m"
	@echo -e "make -s DISTRIB=stretch V=1 package"
	@echo -e ""
	@echo -e "cd $(PACKAGE)/$(RELEASE)"
	@echo -e "make -s build"
	@echo -e "make -s publish"
	@echo -e ""
	@#echo "ftp /~betonniere/bellepoule/ubuntu packagename_version.tar.gz, packagename_version_source.changes, packagename_version.dsc"
