CC=g++
SRCDIR=sources
OBJDIR=obj
BINDIR=bin
DBGDIR=$(BINDIR)/Debug
RLSDIR=$(BINDIR)/Release

# Lancer les commandes suivantes dans un terminal pour voir les bibliothèques et options par défaut
LIBS=`pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas`
CFLAGS=`pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas`

# Options de compilation, l'option -Wno-write-strings est utilisé car beaucoup de warnings
# sur la conversion de chaines constantes en *gchar
#OPTS=-Wall -Wno-write-strings   -Werror -pedantic-errors
OPTS=-Wall -std=c++98 -pedantic
OPTSDBG=$(OPTS) -g

# Fichiers source à compiler et à linker, ATTENTION à mettre à jour la liste à chaque nouvelle version
SRCS=attendees.cpp attribute.cpp canvas.cpp canvas_module.cpp checkin.cpp classification.cpp contest.cpp data.cpp filter.cpp general_classification.cpp glade.cpp main.cpp match.cpp module.cpp object.cpp player.cpp players_list.cpp pool_allocator.cpp pool.cpp pool_match_order.cpp pool_supervisor.cpp schedule.cpp score_collector.cpp score.cpp sensitivity_trigger.cpp splitting.cpp stage.cpp swapper.cpp table.cpp tournament.cpp table_supervisor.cpp table_set.cpp upload.cpp
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

.PHONY: clean
clean:
	@mkdir -p $(OBJDIR)
	cd $(OBJDIR) && rm -f $(OBJS)
	cd $(SRCDIR) && rm -f *~
	rm -f $(PROG)
	rm -f $(DBGDIR)/$(PROG)
	rm -f $(RLSDIR)/$(PROG)

