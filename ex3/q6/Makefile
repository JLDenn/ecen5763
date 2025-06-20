EXEC     = track
CC       = g++

CFLAGS   = 
LDFLAGS  = 

SRC      = $(wildcard *.cpp)
OBJ      = $(SRC:.cpp=.o)

all: $(EXEC)

${EXEC}: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

%.o: %.cpp img.hpp
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean
clean:
	@rm -rf *.o ${EXEC}

