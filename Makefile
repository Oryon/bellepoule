CC=g++
SRCDIR=sources
OBJDIR=obj
BINDIR=bin
DBGDIR=$(BINDIR)/Debug
RLSDIR=$(BINDIR)/Release

# Define certain make variables in order to be more RPM friendly
REVNUM=$$(bzr revno lp:bellepoule)
MAJOR=$$(egrep 'VERSION ' sources/version.h | sed -e 's=\(.*\"\)\([[:alnum:]]*\)\(\".*\)=\2=')
MINOR=$$(egrep VERSION_REVISION sources/version.h | sed -e 's=\(.*\"\)\([[:alnum:]]*\)\(\".*\)=\2=')
MATURITY=$$(egrep VERSION_MATURITY sources/version.h | sed -e 's=\(.*\"\)\([[:alnum:]]*\)\(\".*\)=\2=')
ifndef MATURITY
  TGZNUM=${MAJOR}.${MINOR}-r${REVNUM}
else
  TGZNUM=${MAJOR}.${MINOR}.${MATURITY}-r${REVNUM}
endif
DESTDIR=
prefix?=/usr/local
bindir?=$(DESTDIR)/$(prefix)/bin
datadir?=$(DESTDIR)/$(prefix)/share
localedir?=$(datadir)/locale
CP=cp -p

# Lancer les commandes suivantes dans un terminal pour voir les bibliothèques et options par défaut
LIBS=`pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas`
CFLAGS=`pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas`

# Options de compilation, l'option -Wno-write-strings est utilisé car beaucoup de warnings
# sur la conversion de chaines constantes en *gchar
#OPTS=-Wall -Wno-write-strings   -Werror -pedantic-errors
OPTS=-Wall -std=c++98 -pedantic
OPTSDBG=$(OPTS) -g

# Fichiers source à compiler et à linker, ATTENTION à mettre à jour la liste à chaque nouvelle version
SRCS=attendees.cpp attribute.cpp canvas.cpp canvas_module.cpp checkin.cpp classification.cpp contest.cpp data.cpp filter.cpp general_classification.cpp glade.cpp main.cpp match.cpp module.cpp object.cpp player.cpp players_list.cpp pool_allocator.cpp pool.cpp pool_match_order.cpp pool_supervisor.cpp schedule.cpp score_collector.cpp score.cpp sensitivity_trigger.cpp splitting.cpp stage.cpp swapper.cpp table.cpp tournament.cpp table_supervisor.cpp table_set.cpp utilities.cpp
OBJS=$(SRCS:.cpp=.o)

# Nom du fichier exécutable à générer dans $(DBGDIR) et $(RLSDIR)
PROG=BellePoule

main: $(OBJS)
	@mkdir -p $(DBGDIR)
	@mkdir -p $(RLSDIR)
	@cd $(OBJDIR) && $(CC) $(OBJS) $(LIBS) $(OPTSDBG) -o ../$(DBGDIR)/$(PROG)
	@cd $(OBJDIR) && $(CC) $(OBJS) $(LIBS) $(OPTS) -o ../$(RLSDIR)/$(PROG)

%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	@$(CC) -c $< -o $(OBJDIR)/$@ $(CFLAGS) $(OPTSDBG)

.PHONY: clean install tgz png_icons
png_icons:
	convert resources/exe.ico BellePoule.png

tgz:
	bzr export --format tgz ${PROG}-${TGZNUM}.tgz

install: $(RLSDIR)/$(PROG)
	mkdir -p $(bindir)
	$(CP) $(RLSDIR)/$(PROG) $(bindir)
	for l in ar de es fr it nl ru ; do \
	   mkdir -p $(localedir)/$$l/LC_MESSAGES ; \
	   $(CP) resources/translations/$$l/LC_MESSAGES/BellePoule.mo $(localedir)/$$l/LC_MESSAGES/BellePoule.mo ; \
	done
	## install resources
	mkdir -p $(datadir)/BellePoule
	$(CP) resources/clubs.txt $(datadir)/BellePoule
	$(CP) resources/ioc_countries.txt $(datadir)/BellePoule
	$(CP) resources/ligues.txt $(datadir)/BellePoule
	$(CP) resources/user_manual.pdf $(datadir)/BellePoule
	$(CP) -r resources/glade $(datadir)/BellePoule
	## install icons
	mkdir -p $(datadir)/icons/hicolor/16x16/apps
	mv BellePoule-0.png $(datadir)/icons/hicolor/16x16/apps/BellePoule.png
	mkdir -p $(datadir)/icons/hicolor/32x32/apps
	mv BellePoule-1.png $(datadir)/icons/hicolor/32x32/apps/BellePoule.png
	mkdir -p $(datadir)/icons/hicolor/48x48/apps
	mv BellePoule-2.png $(datadir)/icons/hicolor/48x48/apps/BellePoule.png

clean:
	@mkdir -p $(OBJDIR)
	cd $(OBJDIR) && rm -f $(OBJS)
	cd $(SRCDIR) && rm -f *~
	rm -f $(PROG)
	rm -f $(DBGDIR)/$(PROG)
	rm -f $(RLSDIR)/$(PROG)

