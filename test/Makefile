MAKEFILE_PATH:=$(abspath $(lastword $(MAKEFILE_LIST)))
ROOT_DIR:=$(realpath $(dir $(MAKEFILE_PATH)))
debug=0
ifeq ($(OS),Windows_NT)
	platform=windows
else
architecture=$(shell uname -m)

ifeq ($(shell uname -s),Linux)
platform=linux
endif
ifeq ($(shell uname -s),Darwin)
platform=macos
endif
ifeq ($(shell uname -s),Haiku)
platform=haiku
endif
endif

CXXFLAGS=-c -std=c++11 -Wall -O2 -I$(ROOT_DIR)/../include
LDFLAGS=-O2
ifeq ($(platform),haiku)
LDFLAGS+=-lnetwork
endif
SOURCES=$(ROOT_DIR)/main.cpp
BASE_NAMES=$(basename $(SOURCES))
OBJECTS=$(BASE_NAMES:=.o)
EXECUTABLE=test

all: $(EXECUTABLE)
ifeq ($(debug),1)
all: CXXFLAGS+=-DDEBUG -g
endif

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
ifeq ($(platform),windows)
	-del /f /q "$(ROOT_DIR)\$(EXECUTABLE).exe" "$(ROOT_DIR)\*.o"
else
	$(RM) $(ROOT_DIR)/$(EXECUTABLE) $(ROOT_DIR)/*.o $(ROOT_DIR)/$(EXECUTABLE).exe
endif
