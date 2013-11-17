ifeq ($(CLANG), 1)
	CC=clang
	STDCPP=-lstdc++
else
	CC=g++
	STDCPP=
endif

INC = $(BOOST_ROOT)/
INC_PARAMS=$(foreach d, $(INC), -I$d)

LIB_DIRS = $(BOOST_ROOT)/stage/lib/

SOURCES = FHErrorTracker.cpp

LIBS = -lm

# Removing warnings on unused typedefs to quiet down boost
COMMON = -std=c++11 -Wall -Wno-unused-local-typedefs $(STDCPP) -L$(LIB_DIRS) $(LIBS) $(INC_PARAMS) -pthread -Wl,--no-as-needed

all:
	make release

debug:
	$(CC) -g -DDEBUG $(COMMON) $(SOURCES) main.cpp -o OrderBookProcessor

release:
	g++ -O3 -DNDEBUG $(COMMON) $(SOURCES) main.cpp -o OrderBookProcessor

test:
	$(CC) -g -DDEBUG $(COMMON) $(SOURCES) Tester.cpp -o UtilityTester

perftest:
	g++ -O3 -g -DDEBUG $(COMMON) $(SOURCES) Tester.cpp -o UtilityTester

profile:
	g++ -O3 -DNDEBUG -DPROFILE $(COMMON) $(SOURCES) main.cpp -o OrderBookProcessor

lib:
	g++ -O3 -DNDEBUG $(COMMON) $(SOURCES) -shared -o OrderBook.lib

grind:
	$(CC) -g -O3 -DNDEBUG -DPROFILE $(COMMON) $(SOURCES) main.cpp -o OrderBookProcessor

clean:
	rm -f OrderBookProcessor
	rm -f UtilityTester
	rm -f *.out
