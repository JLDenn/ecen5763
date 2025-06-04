EXEC     = version
CC       = g++

CFLAGS   = -I/usr/include/opencv4
LDFLAGS  = -lopencv_core

SRC      = $(wildcard *.cpp)
OBJ      = $(SRC:.cpp=.o)

all: $(EXEC)

${EXEC}: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

%.o: %.cpp 
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean
clean:
	@rm -rf *.o ${EXEC}

