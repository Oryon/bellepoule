TARGET = Debug
CC     = gcc -g -DDEBUG=1

CFLAGS  = -W -Wall -ansi -pedantic -Wno-unused
CFLAGS += $(shell pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd)
CFLAGS += -Isources/common
CFLAGS += -Isources/network
CFLAGS += -Isources/people_management
CFLAGS += -Isources/pool_round
CFLAGS += -Isources/table_round
CFLAGS += -Isources/util
CFLAGS += -DDEBUG=1

LDFLAGS  = -lstdc++
LDFLAGS += $(shell pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0 goocanvas libcurl libmicrohttpd)

EXEC    = bellepoule
SRC     = $(wildcard sources/*/*.cpp)
OBJ     = $(SRC:.cpp=.o)

all: linux/$(TARGET)/$(EXEC)

linux/Debug/bellepoule: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo $@
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm sources/*/*.o
