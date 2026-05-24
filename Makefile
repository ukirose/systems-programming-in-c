CC = cc
CFLAGS = -Wall -Wextra -O2

TARGET = httpd
OBJS = main.o server.o io.o request.o response.o file.o env.o log.o

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

test: all
	tests/http.py

main.o: main.c server.h env.h log.h
server.o: server.c server.h request.h response.h file.h log.h
io.o: io.c io.h
request.o: request.c request.h
response.o: response.c response.h file.h io.h
file.o: file.c file.h io.h
env.o: env.c env.h log.h
log.o: log.c log.h

clean:
	rm -f $(TARGET) $(OBJS)
