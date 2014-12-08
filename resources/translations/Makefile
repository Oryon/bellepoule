.SILENT:

DOMAIN       = BellePoule
POT_FILE     = $(DOMAIN).pot
PO_FILES     = $(wildcard *.po)
MO_FILES     = $(wildcard */LC_MESSAGES/$(DOMAIN).mo)
CPP_FILES    = $(wildcard ../../sources/*/*cpp)
CPP_FILES   += $(wildcard ../../sources/*/*/*cpp)
CPP_FILES   += $(wildcard ../../sources/*/*/*/*cpp)
GLADE_FILES  = $(wildcard ../../resources/glade/*glade)

all: $(MO_FILES)

$(POT_FILE): $(CPP_FILES) $(GLADE_FILES)
	echo "[1;33;44m  $(POT_FILE) [0m"
	xgettext --default-domain=$(DOMAIN)\
           --output=$(POT_FILE)\
           --from-code=UTF-8\
           --no-location\
           --keyword=N_\
           $^

clean:
	rm $(POT_FILE)

revert:
	bzr revert `bzr status --versioned --short --no-classify . | sed 's/ M //' | sed 's/.*translations\///' | grep -v Makefile`

%.mo: $(POT_FILE)
	echo
	echo "[1;33;44m  $@ [0m"
	msgmerge --no-fuzzy-matching\
           --update\
           --backup=none\
           --quiet\
           $(subst /LC_MESSAGES/$(DOMAIN).mo,.po,$@) $(POT_FILE)
	msgfmt --check-format\
         --statistics\
         --verbose\
         --output-file=$@\
           $(subst /LC_MESSAGES/$(DOMAIN).mo,.po,$@)

to_trunk:
	cp $(PO_FILES) ../../../trunk/resources/translations
	cp $(POT_FILE) ../../../trunk/resources/translations

from_trunk:
	cp ../../../trunk/resources/translations/$(POT_FILE) .
	cp ../../../trunk/resources/translations/*.po        .
