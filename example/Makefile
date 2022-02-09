DEBUG=0
ifeq ($(OS),Windows_NT)
platform=windows
else ifeq ($(shell uname -s),Linux)
platform=linux
endif
ifeq ($(shell uname -s),Darwin)
platform=macos
else ifeq ($(shell uname -s),Haiku)
platform=haiku
endif

CXXFLAGS=-std=c++11 -Wall -Wshadow -O2 -I../include
LDFLAGS=-O2
ifeq ($(platform),windows)
LDFLAGS+=-lws2_32
else ifeq ($(platform),haiku)
LDFLAGS+=-lnetwork
endif
SOURCES=main.cpp
BASE_NAMES=$(basename $(SOURCES))
OBJECTS=$(BASE_NAMES:=.o)
DEPENDENCIES=$(OBJECTS:.o=.d)
EXECUTABLE=example

all: $(EXECUTABLE)
ifeq ($(DEBUG),1)
all: CXXFLAGS+=-DDEBUG -g
endif

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

-include $(DEPENDENCIES)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -MMD -MP $< -o $@

.PHONY: clean
clean:
	$(RM) $(EXECUTABLE) $(OBJECTS) $(DEPENDENCIES) $(EXECUTABLE).exe