debug=0
ifeq ($(OS),Windows_NT)
platform=windows
else
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

CXXFLAGS=-std=c++11 -Wall -O2 -I../include
LDFLAGS=-O2
ifeq ($(platform),haiku)
LDFLAGS+=-lnetwork
endif
SOURCES=main.cpp
BASE_NAMES=$(basename $(SOURCES))
OBJECTS=$(BASE_NAMES:=.o)
DEPENDENCIES=$(OBJECTS:.o=.d)
EXECUTABLE=test

all: $(EXECUTABLE)
ifeq ($(debug),1)
all: CXXFLAGS+=-DDEBUG -g
endif

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

-include $(DEPENDENCIES)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -MMD -MP $< -o $@

.PHONY: clean
clean:
ifeq ($(platform),windows)
	-del /f /q "$(EXECUTABLE).exe" "*.o"
else
	$(RM) $(EXECUTABLE) *.o $(EXECUTABLE).exe
endif