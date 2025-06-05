EXEC     = face_detect
CC       = g++

CFLAGS   = -I/usr/include/opencv4 
LDFLAGS  = -L/usr/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_objdetect

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

