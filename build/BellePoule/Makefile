#----------------------------------------------------
#-- Variables ---------------------------------------
#----------------------------------------------------
PRODUCT = bellepoulebeta
#PRODUCT = bellepoule
OUTPUT  = Release
PACKAGE = $(HOME)/PPA/$(PRODUCT)
VERSION = 5.0
STEP    = ubuntu1
RELEASE = $(PRODUCT)_$(VERSION)
UBUNTU  = precise
V       = 1
DATE    = $(shell date -R)

#----------------------------------------------------
#-- Compilers ---------------------------------------
#----------------------------------------------------
ifeq ($(UBUNTU), windows)
CC = i586-mingw32msvc-g++
AR = i586-mingw32msvc-ar
else
CC = g++
AR = ar
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
CFLAGS += -DGTK_DISABLE_SINGLE_INCLUDES
CFLAGS += -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES
CFLAGS += -DGDK_DISABLE_DEPRECATED
CFLAGS += -DGSEAL_ENABLE
CFLAGS += -Wall
CFLAGS += -Wcast-align
CFLAGS += -Wextra
CFLAGS += -Winit-self
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wno-long-long
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-variadic-macros
CFLAGS += -Wshadow
CFLAGS += -Wswitch-default
CFLAGS += -Wswitch-enum
CFLAGS += -Wundef
ifeq ($(OUTPUT), Debug)
CFLAGS += -DDEBUG=1
endif

ifeq ($(UBUNTU), windows)
CFLAGS += -mms-bitfields
CFLAGS += -D_FILE_OFFSET_BITS=32
CFLAGS += -DWINDOWS_TEMPORARY_PATCH=1
CFLAGS += -DWIN32
CFLAGS += -DLIBXML2_STATIC
else
CFLAGS += -std=c++98
CFLAGS += -fPIC
CFLAGS += -Wno-int-to-pointer-cast
CFLAGS += -Wno-vla
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Wredundant-decls
CFLAGS += -Wunreachable-code
CFLAGS += $(shell pkg-config --cflags gtk+-2.0 gthread-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd libqrencode)
endif

ifeq ($(UBUNTU), windows)
CFLAGS += -I/usr/i586-mingw32msvc/include
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/msys/1.0/include
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include/gtk-2.0
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include/cairo
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include/gdk-pixbuf-2.0
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include/glib-2.0
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/lib/glib-2.0/include
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include/pango-1.0
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/lib/gtk-2.0/include
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include/atk-1.0
CFLAGS += -I/home/yannick/.wine/drive_c/MinGW/include/goocanvas-1.0.0/goocanvas
endif

CFLAGS += -Isources/common
CFLAGS += -Isources/BellePoule

#----------------------------------------------------
#-- LDFLAGS -----------------------------------------
#----------------------------------------------------
LDFLAGS  = -lstdc++
LDFLAGS += -lssl
LDFLAGS += -lcrypto
LDFLAGS += -L$(OUTPUT)

ifeq ($(UBUNTU), windows)
LDFLAGS += -mwindows
LDFLAGS += -lgtk-win32-2.0
LDFLAGS += -lgobject-2.0
LDFLAGS += -lglib-2.0
LDFLAGS += -llibgthread-2.0
LDFLAGS += -lgoocanvas
LDFLAGS += -lgdk-win32-2.0
LDFLAGS += -latk-1.0
LDFLAGS += -lgio-2.0
LDFLAGS += -lgdk_pixbuf-2.0
LDFLAGS += -lpangowin32-1.0
LDFLAGS += -lgdi32
LDFLAGS += -lpangocairo-1.0
LDFLAGS += -lpango-1.0
LDFLAGS += -lcairo
LDFLAGS += -lgmodule-2.0
LDFLAGS += -lintl
LDFLAGS += -lws2_32
LDFLAGS += -lcurl
LDFLAGS += -lcurldll
LDFLAGS += -lz
LDFLAGS += -lzdll
LDFLAGS += -lmicrohttpd
LDFLAGS += -lqrencode
LDFLAGS += -liphlpapi
LDFLAGS += -lpthreadGC2
LDFLAGS += -lxml2
LDFLAGS += -L/usr/i586-mingw32msvc/lib
LDFLAGS += -L/home/yannick/.wine/drive_c/MinGW/lib
LDFLAGS += -L/home/yannick/.wine/drive_c/MinGW/msys/1.0/lib
else
LDFLAGS += $(shell pkg-config --libs gtk+-2.0 gthread-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd libqrencode)
endif

ifeq ($(OUTPUT), Release)
LDFLAGS += -s
endif

#----------------------------------------------------
#-- Sources -----------------------------------------
#----------------------------------------------------
SRC  = $(wildcard sources/common/network/*.cpp)
SRC += $(wildcard sources/common/util/*.cpp)
SRC += $(wildcard sources/BellePoule/actors/*.cpp)
SRC += $(wildcard sources/BellePoule/application/*.cpp)
OBJ_LIB += $(SRC:.cpp=.o)

SRC_MARSHALLER = $(wildcard sources/BellePoule/marshaller/*.cpp)
OBJ_MARSHALLER = $(SRC_MARSHALLER:.cpp=.o)
ifeq ($(UBUNTU), windows)
OBJ_MARSHALLER += sources/BellePoule/marshaller/marshaller-ico.o
endif

SRC_SUPERVISOR  = $(wildcard sources/BellePoule/supervisor/*.cpp)
SRC_SUPERVISOR += $(wildcard sources/BellePoule/supervisor/*/*.cpp)
SRC_SUPERVISOR += $(wildcard sources/BellePoule/supervisor/*/*/*.cpp)
SRC_SUPERVISOR += $(wildcard sources/BellePoule/supervisor/*/*/*/*.cpp)
OBJ_SUPERVISOR = $(SRC_SUPERVISOR:.cpp=.o)
ifeq ($(UBUNTU), windows)
OBJ_SUPERVISOR += sources/BellePoule/supervisor/supervisor-ico.o
endif

ifeq ($(UBUNTU), windows)
SHARED_LIB = 
RPATH      =
STATIC_LIB = $(OUTPUT)/lib$(PRODUCT).a
else
SHARED_LIB = $(OUTPUT)/lib$(PRODUCT).so
RPATH      = -Wl,--rpath,/usr/lib/$(PRODUCT)
STATIC_LIB =
endif

#----------------------------------------------------
#-- Targets -----------------------------------------
#----------------------------------------------------
all: output $(OUTPUT)/$(PRODUCT)-marshaller $(OUTPUT)/$(PRODUCT)-supervisor

output:
	@mkdir -p $(OUTPUT)

#--

$(SHARED_LIB): $(OBJ_LIB)
	@echo "\033[1;31m" $@ "\033[0m"
	@$(CC) -shared -o $@ $^ $(LDFLAGS)

$(STATIC_LIB): $(OBJ_LIB)
	@echo "\033[1;31m" $@ "\033[0m"
	@$(AR) rcs $@ $^

$(OUTPUT)/$(PRODUCT)-supervisor: $(SHARED_LIB) $(STATIC_LIB) $(OBJ_SUPERVISOR)
	@echo "\033[1;31m" $@ "\033[0m"
	@$(CC) -o $@ $(OBJ_SUPERVISOR) $(RPATH) $(STATIC_LIB) -l$(PRODUCT) $(LDFLAGS)

$(OUTPUT)/$(PRODUCT)-marshaller: $(SHARED_LIB) $(STATIC_LIB) $(OBJ_MARSHALLER)
	@echo "\033[1;31m" $@ "\033[0m"
	@$(CC) -o $@ $(OBJ_MARSHALLER) $(RPATH) $(STATIC_LIB) -l$(PRODUCT) $(LDFLAGS)

#--

%.o: %.cpp
	@echo "\033[1;34m" $@ "\033[0m"
	@$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.rc
	@echo "\033[1;33m" $@ "\033[0m"
	@i586-mingw32msvc-windres --output=$@ --input=$<

#--

cflags:
	@echo $(CFLAGS)

valgrind:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --log-file=memory_leak.log --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 $(OUTPUT)/$(PRODUCT)
	#G_SLICE=always-malloc G_DEBUG=gc-friendly valkyrie $(OUTPUT)/$(PRODUCT)

#----------------------------------------------------
#-- Packaging ---------------------------------------
#----------------------------------------------------
ifeq ($(UBUNTU), windows)
prepare_resources = prepare_windows_resources
else
prepare_ubuntu    = prepare_ubuntu
prepare_www       = prepare_www
prepare_examples  = prepare_examples
prepare_resources = prepare_resources
endif

package: prepare_staging $(prepare_ubuntu) prepare_sources $(prepare_examples) $(prepare_resources) $(prepare_www)
	rm -rf `find $(PACKAGE) -name "*~*"`
	rm -rf `find $(PACKAGE) -name "*.swp"`
	rm -rf `find $(PACKAGE) -name ".*.swp"`
	find $(PACKAGE)/$(RELEASE) -type f -exec chmod a-x {} \;

prepare_ubuntu: prepare_debian prepare_changelog prepare_control prepare_rules prepare_copyright prepare_desktop

prepare_examples:
	mkdir -p $(PACKAGE)/$(RELEASE)/Exemples
	cp -f  ../../Exemples/exemple.cotcot      $(PACKAGE)/$(RELEASE)/Exemples
	mkdir -p $(PACKAGE)/$(RELEASE)/Exemples/FFE
	cp -rf ../../Exemples/FFE/Individuelle.XML $(PACKAGE)/$(RELEASE)/Exemples/FFE
	cp -rf ../../Exemples/FFE/ParEquipe.XML    $(PACKAGE)/$(RELEASE)/Exemples/FFE
	cp -rf ../../Exemples/FFE/Fencers.FFF      $(PACKAGE)/$(RELEASE)/Exemples/FFE
	cp -rf ../../Exemples/FFE/Referees.txt     $(PACKAGE)/$(RELEASE)/Exemples/FFE
	mkdir -p $(PACKAGE)/$(RELEASE)/Exemples/Classements
	cp -rf ../../Exemples/Classements/EHBEN.TXT $(PACKAGE)/$(RELEASE)/Exemples/Classements
	cp -rf ../../Exemples/Classements/EHBEN.xml $(PACKAGE)/$(RELEASE)/Exemples/Classements
	cp -rf ../../Exemples/Classements/FHBEN.xml $(PACKAGE)/$(RELEASE)/Exemples/Classements
	cp -rf ../../Exemples/Classements/FHBEN.xml $(PACKAGE)/$(RELEASE)/Exemples/Classements
	find $(PACKAGE)/$(RELEASE)/Exemples -type f -exec chmod a-x {} \;

prepare_www:
	cp -rf ../../webserver/LightTPD/www $(PACKAGE)/$(RELEASE)
	rm -rf $(PACKAGE)/$(RELEASE)/www/cotcot
	cp -f  debian/99-bellepoule.conf $(PACKAGE)/$(RELEASE)/99-$(PRODUCT).conf
	sed -i 's/bellepoule/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/99-$(PRODUCT).conf
	mkdir -p $(PACKAGE)/$(RELEASE)/scripts
	cp -f  ../../scripts/www*sh $(PACKAGE)/$(RELEASE)/scripts
	sed -i 's/bellepouledebug/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/scripts/wwwstart.sh
	sed -i 's/bellepouledebug/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/scripts/wwwstop.sh

prepare_windows_resources:
	mkdir -p $(PACKAGE)/$(RELEASE)/resources/glade/images
	cp -f ../../resources/glade/images/exe.ico $(PACKAGE)/$(RELEASE)/resources/glade/images

prepare_resources:
	mkdir -p $(PACKAGE)/$(RELEASE)/resources/translations
	cp -f ../../resources/translations/user_manual.pdf $(PACKAGE)/$(RELEASE)/resources/translations
	cp -f ../../resources/translations/index.txt $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/fr    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/de    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/nl    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ru    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ar    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/es    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ca    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/it    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ko    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/pt    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/pt_BR $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/sv    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ja    $(PACKAGE)/$(RELEASE)/resources/translations
	mkdir -p $(PACKAGE)/$(RELEASE)/resources/glade/images
	cp -f ../../resources/glade/images/*.png   $(PACKAGE)/$(RELEASE)/resources/glade/images
	cp -f ../../resources/glade/images/*.jpg   $(PACKAGE)/$(RELEASE)/resources/glade/images
	cp -f ../../resources/glade/*.glade $(PACKAGE)/$(RELEASE)/resources/glade
	cp -rf ../../resources/countries      $(PACKAGE)/$(RELEASE)/resources/countries
	cp -rf ../../resources/localized_data $(PACKAGE)/$(RELEASE)/resources/localized_data
	find $(PACKAGE)/$(RELEASE)/resources -type f -exec chmod a-x {} \;

prepare_sources: clean_sources
	mkdir -p $(PACKAGE)/$(RELEASE)/sources
	cp -rf ../../sources/BellePoule $(PACKAGE)/$(RELEASE)/sources/BellePoule
	cp -rf ../../sources/common     $(PACKAGE)/$(RELEASE)/sources/common

prepare_staging:
	rm -rf $(PACKAGE)/*
	mkdir -p $(PACKAGE)/$(RELEASE)
	cp Makefile $(PACKAGE)/$(RELEASE)

prepare_debian:
	cp -rf debian $(PACKAGE)/$(RELEASE)

prepare_changelog:
	rm -f $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "$(PRODUCT) ($(VERSION)$(STEP)~$(UBUNTU)$(V)) $(UBUNTU); urgency=low" > $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "  * Miscellaneous improvments" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo " -- betonniere <bellepoule.dev@free.fr>  $(DATE)" >> $(PACKAGE)/$(RELEASE)/debian/changelog

prepare_control:
	sed -i 's/__PRODUCT__/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/debian/control
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
	cp -f $(PACKAGE)/$(RELEASE)/debian/*.desktop.template $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-supervisor.desktop
	sed -i 's/__PRODUCT__/$(PRODUCT)/g' $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-supervisor.desktop
	sed -i 's/__EXE__/supervisor/g'     $(PACKAGE)/$(RELEASE)/debian/$(PRODUCT)-supervisor.desktop
	rm -f $(PACKAGE)/$(RELEASE)/debian/*.desktop.template

#----------------------------------------------------
#-- debuild -----------------------------------------
#----------------------------------------------------
.PHONY: install clean_sources

install:
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/lib/$(PRODUCT)
	install $(OUTPUT)/$(PRODUCT)-supervisor $(DESTDIR)/usr/bin/$(PRODUCT)-supervisor
	install $(OUTPUT)/$(PRODUCT)-marshaller $(DESTDIR)/usr/bin/$(PRODUCT)-marshaller
	install $(OUTPUT)/lib$(PRODUCT).so  $(DESTDIR)/usr/lib/$(PRODUCT)/lib$(PRODUCT).so
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
	#Lighttpd
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
help:
	@echo "\033[1;42m**** Windows ****\033[0m"
	@echo "make -s UBUNTU=windows V=$(V) package"
	@echo "cd $(PACKAGE)/$(RELEASE)"
	@echo "make -s UBUNTU=windows V=$(V)"
	@echo "cp Release/$(PRODUCT)-marshaller ~/Project/BellePoule/trunk/build/BellePoule/windows/exe/$(PRODUCT)-marshaller.exe"
	@echo "cp Release/$(PRODUCT)-supervisor ~/Project/BellePoule/trunk/build/BellePoule/windows/exe/$(PRODUCT)-supervisor.exe"
	@echo "cp ~/Project/BellePoule/trunk/build/BellePoule/windows/exe/$(PRODUCT)-*.exe ~/.wine/drive_c/Program\ Files\ \(x86\)/bellepoulebeta/bin"
	@echo ""
	@echo "\033[1;44m**** Ubuntu ****\033[0m"
	@echo "make -s UBUNTU=precise V=$(V) package"
	@echo "make -s UBUNTU=trusty  V=$(V) package"
	@echo "make -s UBUNTU=vivid   V=$(V) package"
	@echo "make -s UBUNTU=wily    V=$(V) package"
	@echo ""
	@echo "cd $(PACKAGE)/$(RELEASE)"
	@echo ""
	@echo "debuild -us -uc --lintian-opts --color always"
	@echo "debuild -S -sa -k"`gpg -K | grep "sec " | sed 's/.*\///' | sed 's/ .*//'`
	@#echo "ftp /~betonniere/bellepoule/ubuntu packagename_version.tar.gz, packagename_version_source.changes, packagename_version.dsc"
	@echo "dput ppa:betonniere/bellepoule ../$(PRODUCT)*_source.changes"
	@echo ""