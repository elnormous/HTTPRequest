DEBUG=0
CXXFLAGS=-std=c++17 -Wall -Wextra -Wshadow -I../external/Catch2/single_include -I../include
SOURCES=encoding.cpp main.cpp parsing.cpp
BASE_NAMES=$(basename $(SOURCES))
OBJECTS=$(BASE_NAMES:=.o)
DEPENDENCIES=$(OBJECTS:.o=.d)
EXECUTABLE=tests

.PHONY: all
all: $(EXECUTABLE)
ifeq ($(DEBUG),1)
all: CXXFLAGS+=-DDEBUG -g
else
all: CXXFLAGS+=-O3
all: LDFLAGS+=-O3
endif

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@ -fprofile-arcs -ftest-coverage

-include $(DEPENDENCIES)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -MMD -MP $< -o $@ -fprofile-arcs -ftest-coverage

.PHONY: clean
clean:
	$(RM) $(EXECUTABLE) $(OBJECTS) $(DEPENDENCIES) $(EXECUTABLE).exe *.gcda *.gcno