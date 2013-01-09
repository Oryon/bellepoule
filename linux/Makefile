EXEC    = bellepoule
TARGET  = Debug
PACKAGE = ~/BellePoule

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
CFLAGS += $(addprefix -I../sources/,$(MODULE))
CHECK_PATH += $(addprefix -I ../sources/,$(MODULE))
ifeq ($(TARGET), Debug)
CFLAGS += -DDEBUG=1
endif

LDFLAGS  = -lstdc++
LDFLAGS += $(shell pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd)
LDFLAGS += $(addprefix -L$(TARGET)/,$(MODULE))

SRC   = $(wildcard ../sources/*/*.cpp)
OBJ   = $(SRC:.cpp=.o)
CHECK = $(SRC:.cpp=.check)
#OBJ = $(subst ../sources,$(TARGET),$(SRC:.cpp=.o))

all: subdir $(TARGET)/$(EXEC)

subdir:
	mkdir -p $(TARGET)

check: clean_check $(CHECK)

clean_check:
	rm -f check.log

install:
	sudo dpkg -i BellePoule.deb

package: pre_packaging package_bin package_examples package_resources
	chmod -R 744 $(PACKAGE)/package/usr/share
	cd $(PACKAGE) ; dpkg-deb --build package
	cp $(PACKAGE)/package.deb BellePoule.deb

pre_packaging:
	rm -rf $(PACKAGE)/package
	mkdir -p $(PACKAGE)/package
	cp -rf package/DEBIAN $(PACKAGE)/package

package_bin:
	mkdir -p $(PACKAGE)/package/usr/bin
	cp -f Release/BellePoule $(PACKAGE)/package/usr/bin

package_examples:
	mkdir -p $(PACKAGE)/package/usr/share/BellePoule/Exemples
	cp -f  ../Exemples/exemple.cotcot $(PACKAGE)/package/usr/share/BellePoule/Exemples
	cp -f  ../Exemples/CLS_SHM.FFF    $(PACKAGE)/package/usr/share/BellePoule/Exemples
	cp -f  ../Exemples/CLS_EDM.FFF    $(PACKAGE)/package/usr/share/BellePoule/Exemples
	cp -rf ../Exemples/FFE            $(PACKAGE)/package/usr/share/BellePoule/Exemples

package_resources:
	mkdir -p $(PACKAGE)/package/usr/share/BellePoule/resources
	cp -f ../resources/user_manual.pdf $(PACKAGE)/package/usr/share/BellePoule/resources
	mkdir -p $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -f ../resources/translations/index.txt $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/fr    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/de    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/nl    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/ru    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/ar    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/es    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/it    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/ko    $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	cp -rf ../resources/translations/pt_br $(PACKAGE)/package/usr/share/BellePoule/resources/translations
	mkdir -p $(PACKAGE)/package/usr/share/BellePoule/resources/glade
	cp -f ../resources/glade/*.png   $(PACKAGE)/package/usr/share/BellePoule/resources/glade
	cp -f ../resources/glade/*.jpg   $(PACKAGE)/package/usr/share/BellePoule/resources/glade
	cp -f ../resources/glade/*.glade $(PACKAGE)/package/usr/share/BellePoule/resources/glade
	cp -rf ../resources/countries      $(PACKAGE)/package/usr/share/BellePoule/resources/countries
	cp -rf ../resources/localized_data $(PACKAGE)/package/usr/share/BellePoule/resources/localized_data

$(TARGET)/$(EXEC): $(OBJ)
	@echo $@
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo $@
	$(CC) -o $@ -c $< $(CFLAGS)

%.check: %.cpp
	@cppcheck --enable=all $(CHECK_PATH) $< 2>> check.log

valgrind:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --log-file=memory_leak.log --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 $(TARGET)/$(EXEC)
	#G_SLICE=always-malloc G_DEBUG=gc-friendly valkyrie $(TARGET)/$(EXEC)

.PHONY: clean

clean:
	rm -f ../sources/*/*.o
	rm -rf $(TARGET)/*
	rmdir $(TARGET)
