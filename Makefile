
CC=clang
FLAGS=-lm -lSDL2
SOURCES=main.c
EXECUTABLE=snake

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(FLAGS) $(SOURCES) -o $@


