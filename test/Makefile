debug=0
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

CXXFLAGS=-c -std=c++11 -Wall -O2 -I../include
LDFLAGS=-O2
ifeq ($(platform),windows)
LDFLAGS+=-lws2_32
else ifeq ($(platform),haiku)
LDFLAGS+=-lnetwork
endif
SOURCES=main.cpp
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
	-del /f /q "$(EXECUTABLE).exe" "*.o"
else
	$(RM) $(EXECUTABLE) *.o $(EXECUTABLE).exe
endif