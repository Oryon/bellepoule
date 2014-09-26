EXEC = livepoule

CC = arm-linux-gnueabihf-gcc

CFLAGS  =
#CFLAGS += -DWIRING_PI
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
CFLAGS += $(shell pkg-config --cflags gtk+-3.0 libxml-2.0 libcurl libmicrohttpd libqrencode libssl)
CFLAGS += -I../../sources/common
CFLAGS += -I../../sources/LivePoule

LDFLAGS  = -lstdc++
LDFLAGS += $(shell pkg-config --libs gtk+-3.0 libxml-2.0 libcurl libmicrohttpd libqrencode libssl)
LDFLAGS += -lwiringPi
LDFLAGS += -rdynamic

SRC   = $(wildcard ../../sources/common/*/*.cpp)
SRC  += $(wildcard ../../sources/LivePoule/*.cpp)
SRC  := $(filter-out ../../sources/common/util/canvas_module.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/util/drop_zone.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/util/canvas.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/util/tinyxml2.cpp, $(SRC))
OBJ   = $(SRC:.cpp=.o)
#OBJ = $(subst sources,$(TARGET),$(SRC:.cpp=.o))

##
## Compilation section
##
all: $(EXEC)
	cp $(EXEC) ~/bin

$(EXEC): $(OBJ)
	@echo "\033[1;36m" $< "\033[0m"
	@$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo "\033[1;34m" $@ "\033[0m"
	@$(CC) -o $@ -c $< $(CFLAGS)

sync:
	touch $(SRC)

clean:
	rm $(OBJ)
	rm $(EXEC)
