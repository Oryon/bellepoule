EXEC    = bellepoulebeta
TARGET  = Release
PACKAGE = /local/$(USER)/$(EXEC)
VERSION = 4.0
STEP    = ubuntu14
RELEASE = $(EXEC)_$(VERSION)
UBUNTU  = precise
V       = 1

CC = gcc
ifeq ($(TARGET), Debug)
CC += -g
endif

MODULE  = common/network
MODULE += common/util
MODULE += BellePoule/people_management
MODULE += BellePoule/pool_round
MODULE += BellePoule/pool_round/smart_swapper
MODULE += BellePoule/table_round

CFLAGS  =
CFLAGS += -Wshadow
CFLAGS += -Winit-self
CFLAGS += -Wredundant-decls
CFLAGS += -Wcast-align
CFLAGS += -Wundef
CFLAGS += -Wunreachable-code
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wswitch-enum
CFLAGS += -Wswitch-default
CFLAGS += -pedantic
CFLAGS += -std=c++98
CFLAGS += -Wextra
CFLAGS += -Wall
CFLAGS += -Wno-int-to-pointer-cast
CFLAGS += -Wno-long-long
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-shadow
CFLAGS += -Wno-vla
CFLAGS += -Wno-variadic-macros
CFLAGS += $(shell pkg-config --cflags gtk+-2.0 gthread-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd libqrencode libssl)
CFLAGS += -Isources/common
CFLAGS += -Isources/BellePoule
ifeq ($(TARGET), Debug)
CFLAGS += -DDEBUG=1
endif

LDFLAGS  = -lstdc++
LDFLAGS += $(shell pkg-config --libs gtk+-2.0 gthread-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd libqrencode libssl)

SRC   = $(wildcard sources/*/*/*.cpp)
SRC  += $(wildcard sources/*/*/*/*.cpp)
OBJ   = $(SRC:.cpp=.o)
#OBJ = $(subst sources,$(TARGET),$(SRC:.cpp=.o))

##
## Compilation section
##
all: subdir $(TARGET)/$(EXEC)

subdir:
	mkdir -p $(TARGET)

$(TARGET)/$(EXEC): $(OBJ)
	@echo "\033[1;31m" $@ "\033[0m"
	@$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo "\033[1;34m" $@ "\033[0m"
	@$(CC) -o $@ -c $< $(CFLAGS)

valgrind:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --log-file=memory_leak.log --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 $(TARGET)/$(EXEC)
	#G_SLICE=always-malloc G_DEBUG=gc-friendly valkyrie $(TARGET)/$(EXEC)

##
## Packaging section
##
package: prepare_packaging prepare_sources prepare_examples prepare_resources prepare_www
	rm -rf `find $(PACKAGE) -name "*~*"`
	rm -rf `find $(PACKAGE) -name "*.swp"`
	find $(PACKAGE)/$(RELEASE) -type f -exec chmod a-x {} \;
	chmod a+x $(PACKAGE)/$(RELEASE)/debian/rules

prepare_packaging: prepare_staging prepare_changelog prepare_control prepare_copyright prepare_desktop

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
	cp -rf ../../sources/www $(PACKAGE)/$(RELEASE)
	cp -f  99-bellepoule.conf $(PACKAGE)/$(RELEASE)
	mkdir -p $(PACKAGE)/$(RELEASE)/scripts
	cp -f  ../../scripts/www* $(PACKAGE)/$(RELEASE)/scripts

prepare_resources:
	mkdir -p $(PACKAGE)/$(RELEASE)/resources
	cp -f ../../resources/user_manual.pdf $(PACKAGE)/$(RELEASE)/resources
	mkdir -p $(PACKAGE)/$(RELEASE)/resources/translations
	cp -f ../../resources/translations/index.txt $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/fr    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/de    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/nl    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ru    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ar    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/es    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/it    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ko    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/pt_br $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/sv    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../../resources/translations/ja    $(PACKAGE)/$(RELEASE)/resources/translations
	mkdir -p $(PACKAGE)/$(RELEASE)/resources/glade
	cp -f ../../resources/glade/*.png   $(PACKAGE)/$(RELEASE)/resources/glade
	cp -f ../../resources/glade/*.jpg   $(PACKAGE)/$(RELEASE)/resources/glade
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
	cp -rf debian $(PACKAGE)/$(RELEASE)

prepare_changelog:
	rm -f $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "$(EXEC) ($(VERSION)$(STEP)~$(UBUNTU)$(V)) $(UBUNTU); urgency=low" > $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "  * Miscellaneous improvments" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo "" >> $(PACKAGE)/$(RELEASE)/debian/changelog
	@echo " -- betonniere <le.roux.yannick@free.fr>  Fri, 29 May 2013 11:42:10 +0100" >> $(PACKAGE)/$(RELEASE)/debian/changelog

prepare_control:
	sed -i 's/__EXEC__/$(EXEC)/g' $(PACKAGE)/$(RELEASE)/debian/control

prepare_copyright:
	sed -i 's/__EXEC__/$(EXEC)/g' $(PACKAGE)/$(RELEASE)/debian/copyright

prepare_desktop:
	mv -f $(PACKAGE)/$(RELEASE)/debian/*.desktop $(PACKAGE)/$(RELEASE)/debian/$(EXEC).desktop
	sed -i 's/__EXEC__/$(EXEC)/g' $(PACKAGE)/$(RELEASE)/debian/$(EXEC).desktop

##
## debuild section
##
.PHONY: install clean_sources

install:
	mkdir -p $(DESTDIR)/usr/bin
	install $(TARGET)/$(EXEC) $(DESTDIR)/usr/bin/$(EXEC)
	mkdir -p $(DESTDIR)/usr/share/applications
	install --mode=644 debian/$(EXEC).desktop $(DESTDIR)/usr/share/applications/$(EXEC).desktop
	mkdir -p $(DESTDIR)/usr/share/$(EXEC)
	#Examples
	mkdir -p `find Exemples -type d | sed 's?Exemples?$(DESTDIR)/usr/share/$(EXEC)/&?'`
	find Exemples -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/$(EXEC)/'$$file ;\
		done
	#Resources
	mkdir -p `find resources -type d | sed 's?resources?$(DESTDIR)/usr/share/$(EXEC)/&?'`
	find resources -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/$(EXEC)/'$$file ;\
		done
	#Lighttpd
	mkdir -p $(DESTDIR)/etc/lighttpd/conf-available
	install --mode=644 99-bellepoule.conf '$(DESTDIR)/etc/lighttpd/conf-available/'99-bellepoule.conf ;
	mkdir -p `find www -type d | sed 's?www?$(DESTDIR)/usr/share/$(EXEC)/&?'`
	find www -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/$(EXEC)/'$$file ;\
		done
	mkdir -p `find scripts -type d | sed 's?scripts?$(DESTDIR)/usr/share/$(EXEC)/&?'`
	find scripts -type f  | while read file;\
		do \
		install --mode=755 $$file '$(DESTDIR)/usr/share/$(EXEC)/'$$file ;\
		done

clean_sources:
	rm -f `find ../../sources -name "*.o"`
	rm -rf $(TARGET)

help:
	@echo "make -s UBUNTU=precise V=$(V) package"
	@echo "make -s UBUNTU=trusty  V=$(V) package"
	@echo "cd $(PACKAGE)/$(RELEASE)"
	@echo "debuild -us -uc --lintian-opts --color always"
	@echo "debuild -S -sa -k<1234>"
	@echo "ftp /~betonniere/bellepoule/ubuntu packagename_version.tar.gz, packagename_version_source.changes, packagename_version.dsc"
	@echo "dput bellepoule $(EXEC)_$(VERSION)$(STEP)~$(UBUNTU)$(V)_source.changes"
