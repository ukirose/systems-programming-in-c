CC = cc
CFLAGS = -Wall -Wextra -O2

TARGET = httpd
OBJS = main.o server.o io.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c server.h
server.o: server.c server.h io.h
io.o: io.c io.h

clean:
	rm -f $(TARGET) $(OBJS)
