CC = cc
CFLAGS = -Wall -Wextra -O2

TARGET = httpd
OBJS = main.o server.o io.o request.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c server.h
server.o: server.c server.h io.h request.h
io.o: io.c io.h
request.o: request.c request.h

clean:
	rm -f $(TARGET) $(OBJS)
