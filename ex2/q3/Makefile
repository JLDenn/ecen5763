EXEC     = canny
CC       = g++

CFLAGS   = -I/usr/include/opencv4 
LDFLAGS  = 

SRC      = $(wildcard *.cpp)
OBJ      = $(SRC:.cpp=.o)

all: $(EXEC)

${EXEC}: $(OBJ)
	$(CC) -o $@ $^ `pkg-config --libs opencv4` $(LDFLAGS) 

%.o: %.cpp 
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean
clean:
	@rm -rf *.o ${EXEC}

