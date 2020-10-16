EXEC = livepoule

CC = arm-linux-gnueabihf-gcc

CFLAGS  =
CFLAGS += -DWIRING_PI
CFLAGS += -DLIVE_POOL
CFLAGS += -Wshadow
CFLAGS += -Winit-self
CFLAGS += -Wredundant-decls
CFLAGS += -Wcast-align
CFLAGS += -Wundef
CFLAGS += -Wunreachable-code
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wswitch-enum
CFLAGS += -Wswitch-default
CFLAGS += -Wsuggest-override
CFLAGS += -pedantic
CFLAGS += -std=c++0x
CFLAGS += -Wextra
CFLAGS += -Wall
CFLAGS += -Wno-int-to-pointer-cast
CFLAGS += -Wno-long-long
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-shadow
CFLAGS += -Wno-vla
CFLAGS += -Wno-variadic-macros
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Wno-cast-function-type
CFLAGS += $(shell pkg-config --cflags gtk+-3.0 libxml-2.0 libcurl libmicrohttpd libqrencode libssl json-glib-1.0)
CFLAGS += -I../../sources/common
CFLAGS += -I../../sources/common/network
CFLAGS += -I../../sources/LivePoule

LDFLAGS  = -lstdc++
LDFLAGS += $(shell pkg-config --libs gtk+-3.0 libxml-2.0 libcurl libmicrohttpd libqrencode libssl json-glib-1.0)
LDFLAGS += -lssl
LDFLAGS += -lcrypto
LDFLAGS += -lwiringPi
LDFLAGS += -rdynamic

RESOURCES = ../../resources
EXAMPLE   = ../../Exemples

SRC   = $(wildcard ../../sources/common/*/*.cpp)
SRC  += $(wildcard ../../sources/common/network/console/*.cpp)
SRC  += $(wildcard ../../sources/LivePoule/*.cpp)
SRC  := $(filter-out ../../sources/common/util/canvas_module.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/util/drop_zone.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/util/canvas.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/util/player.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/network/web_server.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/network/greg_uploader.cpp, $(SRC))
SRC  := $(filter-out ../../sources/common/network/advertiser.cpp, $(SRC))
OBJ   = $(SRC:.cpp=.o)
HPP   = $(SRC:.cpp=.hpp)
HPP  := $(filter-out ../../sources/LivePoule/main.hpp, $(HPP))
#OBJ = $(subst sources,$(TARGET),$(SRC:.cpp=.o))

##
## Compilation section
##
all: $(EXEC)

install:
	mkdir -p $(HOME)/livepoule/bin
	mv $(EXEC) $(HOME)/livepoule/bin
	mkdir -p $(HOME)/livepoule/share/livepoule
	cp -r ../../Exemples $(HOME)/livepoule/share/livepoule
	cp -r ../../resources $(HOME)/livepoule/share/livepoule

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

extract:
	cd ../..; tar xzf lp.tar.gz; cd -

push_share:
	@tar czf share.tar.gz $(EXAMPLE) $(RESOURCES)
	scp share.tar.gz pi@192.168.0.13:/home/pi/livepoule/share/livepoule
	rm share.tar.gz

push_src:
	@tar czf lp.tar.gz $(SRC) $(HPP)
	scp lp.tar.gz pi@192.168.0.13:/home/pi/livepoule/build
	scp Makefile  pi@192.168.0.13:/home/pi/livepoule/build/build/LivePoule
	rm lp.tar.gz
