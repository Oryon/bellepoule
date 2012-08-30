EXEC   = bellepoule
TARGET = Debug

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

CFLAGS  = -W -Wall -ansi -pedantic -Wno-unused
CFLAGS += $(shell pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd)
CFLAGS += $(addprefix -Isources/,$(MODULE))
ifeq ($(TARGET), Debug)
CFLAGS += -DDEBUG=1
endif

LDFLAGS  = -lstdc++
LDFLAGS += $(shell pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd)
LDFLAGS += $(addprefix -Llinux/$(TARGET)/,$(MODULE))

SRC = $(wildcard sources/*/*.cpp)
OBJ = $(SRC:.cpp=.o)
#OBJ = $(subst sources,linux/$(TARGET),$(SRC:.cpp=.o))

all: subdir linux/$(TARGET)/$(EXEC)

subdir:
	mkdir -p linux/$(TARGET)

linux/$(TARGET)/$(EXEC): $(OBJ)
	@echo $@
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo $@
	$(CC) -o $@ -c $< $(CFLAGS)

valgrind:
	G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --suppressions=suppressions --log-file=memory_leak.log --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 linux/$(TARGET)/$(EXEC)
	#G_SLICE=always-malloc G_DEBUG=gc-friendly valkyrie linux/$(TARGET)/$(EXEC)

.PHONY: clean

clean:
	rm -f sources/*/*.o
	rm -rf linux/$(TARGET)/*
	rmdir linux/$(TARGET)
