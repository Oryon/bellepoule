CC=g++
SRCDIR=sources
OBJDIR=obj
#LIBS=$(shell pkg-config --libs pkg-config gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas)
#goocanvasmm-1.0 gtkmm-2.4  libglade-2.0  goocanvas
LIBS=`pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas`
#CFLAGS=$(shell pkg-config --cflags pkg-config gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas)
CFLAGS=`pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas`
#OPTS=-Wall -Wno-write-strings -g -export-dynamic -Wl,--export-dynamic
OPTS=-Wl,--export-dynamic -Wall -Wno-write-strings -g
SRCS=attribute.cpp canvas.cpp canvas_module.cpp checkin.cpp classification.cpp contest.cpp data.cpp filter.cpp general_classification.cpp glade.cpp main.cpp match.cpp module.cpp object.cpp player.cpp players_list.cpp pool_allocator.cpp pool.cpp pool_match_order.cpp pool_supervisor.cpp schedule.cpp score_collector.cpp score.cpp sensitivity_trigger.cpp splitting.cpp stage.cpp table.cpp tournament.cpp
OBJS=$(SRCS:.cpp=.o)
PROG=BellePoule

main: $(OBJS)
	@cd $(OBJDIR) && $(CC) $(OBJS) $(LIBS) $(OPTS) -o ../$(PROG)

%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	@$(CC) -c $< -o $(OBJDIR)/$@ $(CFLAGS) $(OPTS)

.PHONY: clean
clean:
	@mkdir -p $(OBJDIR)
	cd $(OBJDIR) && rm -f $(OBJS)
	cd $(SRCDIR) && rm -f *~
	rm -f $(PROG)

