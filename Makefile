CC = cc
CFLAGS = -Wall -Wextra -O2

TARGET = httpd
CGI = docroot/dump.cgi
OBJS = main.o server.o io.o request.o response.o file.o env.o log.o cgi.o

.PHONY: all test clean

all: $(TARGET) $(CGI)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(CGI): cgi-bin/dump.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	tests/http.py

main.o: main.c server.h env.h log.h
server.o: server.c server.h request.h response.h file.h log.h cgi.h
io.o: io.c io.h
request.o: request.c request.h
response.o: response.c response.h file.h io.h
file.o: file.c file.h io.h
env.o: env.c env.h log.h
log.o: log.c log.h
cgi.o: cgi.c cgi.h request.h response.h io.h

clean:
	rm -f $(TARGET) $(CGI) $(OBJS)
