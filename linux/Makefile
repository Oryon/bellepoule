EXEC    = bellepoule
TARGET  = Release
PACKAGE = ~/BellePoule
RELEASE = bellepoule_3.0

CC = gcc
ifeq ($(TARGET), Debug)
CC += -g
endif

MODULE  = common
MODULE += network
MODULE += people_management
MODULE += pool_round
MODULE += table_round
MODULE += util

CFLAGS  = -W -Wall -ansi -pedantic -Wno-unused -Wno-variadic-macros -Wno-int-to-pointer-cast
CFLAGS += $(shell pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd)
CFLAGS += $(addprefix -Isources/,$(MODULE))
ifeq ($(TARGET), Debug)
CFLAGS += -DDEBUG=1
endif

LDFLAGS  = -lstdc++
LDFLAGS += $(shell pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd)
LDFLAGS += $(addprefix -L$(TARGET)/,$(MODULE))

SRC   = $(wildcard sources/*/*.cpp)
OBJ   = $(SRC:.cpp=.o)
#OBJ = $(subst sources,$(TARGET),$(SRC:.cpp=.o))

##
## Compilation section
##
all: subdir $(TARGET)/$(EXEC)

subdir:
	mkdir -p $(TARGET)

$(TARGET)/$(EXEC): $(OBJ)
	@echo $@
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo $@
	$(CC) -o $@ -c $< $(CFLAGS)

valgrind:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --log-file=memory_leak.log --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 $(TARGET)/$(EXEC)
	#G_SLICE=always-malloc G_DEBUG=gc-friendly valkyrie $(TARGET)/$(EXEC)

##
## Packaging section
##
package: prepare_packaging prepare_sources prepare_examples prepare_resources
	rm -rf `find $(PACKAGE) -name "*~*"`
	rm -rf `find $(PACKAGE) -name "*.swp"`

prepare_packaging: clean
	rm -rf $(PACKAGE)
	mkdir -p $(PACKAGE)/$(RELEASE)
	cp Makefile $(PACKAGE)/$(RELEASE)
	cp -rf debian $(PACKAGE)/$(RELEASE)
	find $(PACKAGE)/$(RELEASE) -type f -exec chmod a-x {} \;
	chmod a+x $(PACKAGE)/$(RELEASE)/debian/rules

prepare_examples:
	mkdir -p $(PACKAGE)/$(RELEASE)/Exemples
	cp -f  ../Exemples/exemple.cotcot $(PACKAGE)/$(RELEASE)/Exemples
	cp -f  ../Exemples/CLS_SHM.FFF    $(PACKAGE)/$(RELEASE)/Exemples
	cp -f  ../Exemples/CLS_EDM.FFF    $(PACKAGE)/$(RELEASE)/Exemples
	cp -rf ../Exemples/FFE            $(PACKAGE)/$(RELEASE)/Exemples
	find $(PACKAGE)/$(RELEASE)/Exemples -type f -exec chmod a-x {} \;

prepare_resources:
	mkdir -p $(PACKAGE)/$(RELEASE)/resources
	cp -f ../resources/user_manual.pdf $(PACKAGE)/$(RELEASE)/resources
	mkdir -p $(PACKAGE)/$(RELEASE)/resources/translations
	cp -f ../resources/translations/index.txt $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/fr    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/de    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/nl    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/ru    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/ar    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/es    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/it    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/ko    $(PACKAGE)/$(RELEASE)/resources/translations
	cp -rf ../resources/translations/pt_br $(PACKAGE)/$(RELEASE)/resources/translations
	mkdir -p $(PACKAGE)/$(RELEASE)/resources/glade
	cp -f ../resources/glade/*.png   $(PACKAGE)/$(RELEASE)/resources/glade
	cp -f ../resources/glade/*.jpg   $(PACKAGE)/$(RELEASE)/resources/glade
	cp -f ../resources/glade/*.glade $(PACKAGE)/$(RELEASE)/resources/glade
	cp -rf ../resources/countries      $(PACKAGE)/$(RELEASE)/resources/countries
	cp -rf ../resources/localized_data $(PACKAGE)/$(RELEASE)/resources/localized_data
	find $(PACKAGE)/$(RELEASE)/resources -type f -exec chmod a-x {} \;

prepare_sources:
	cp -rf ../sources $(PACKAGE)/$(RELEASE)

##
## debuild section
##
.PHONY: install clean

install:
	mkdir -p $(DESTDIR)/usr/bin
	install $(TARGET)/$(EXEC) $(DESTDIR)/usr/bin/$(EXEC)
	mkdir -p $(DESTDIR)/usr/share/applications
	install --mode=644 debian/bellepoule.desktop $(DESTDIR)/usr/share/applications/bellepoule.desktop
	mkdir -p $(DESTDIR)/usr/share/BellePoule
	mkdir -p `find Exemples -type d | sed 's?Exemples?$(DESTDIR)/usr/share/BellePoule/&?'`
	find Exemples -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/BellePoule/'$$file ;\
		done
	mkdir -p `find resources -type d | sed 's?resources?$(DESTDIR)/usr/share/BellePoule/&?'`
	find resources -type f  | while read file;\
		do \
		install --mode=644 $$file '$(DESTDIR)/usr/share/BellePoule/'$$file ;\
		done

clean:
	rm -f sources/*/*.o
	rm -rf $(TARGET)
