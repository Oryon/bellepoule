.SILENT:

DOMAIN      = BellePoule
POT_FILE    = $(DOMAIN).pot
MO_FILES    = $(wildcard */LC_MESSAGES/$(DOMAIN).mo)
CPP_FILES   = $(wildcard ../../sources/*/*cpp)
GLADE_FILES = $(wildcard ../../resources/glade/*glade)

all: $(MO_FILES)

$(POT_FILE): $(CPP_FILES) $(GLADE_FILES)
	echo "[1;33;44m  $(POT_FILE) [0m"
	xgettext --default-domain=$(DOMAIN)\
           --output=$(POT_FILE)\
           --from-code=UTF-8\
           --keyword=N_\
           $^

%.mo: $(POT_FILE)
	echo
	echo "[1;33;44m  $@ [0m"
	msgmerge --no-fuzzy-matching\
           --update\
           --backup=none\
           --verbose\
           $(subst /LC_MESSAGES/$(DOMAIN).mo,.po,$@) $(POT_FILE)
	msgfmt --check-format\
         --statistics\
         --verbose\
         --output-file=$@\
           $(subst /LC_MESSAGES/$(DOMAIN).mo,.po,$@)
