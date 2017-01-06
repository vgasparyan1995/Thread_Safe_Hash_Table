CC=g++
CPPFLAGS= -O3 -std=c++14
HEADERS= Bucket.h HashMap.h IteratorHelper.h Reference.h unit_test.h
SOURCES= main.cpp
OBJECTS= $(SOURCES:.cpp=.o)
EXECUTABLE=hash_map_unit_test

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(HEADERS)
	$(CC) $(OBJECTS) -o $@

$(OBJECTS): $(HEADERS)

%.o : %.cpp
	$(CC) $(CPPFLAGS) $(SOURCES) -c

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
