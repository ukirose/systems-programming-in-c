CC = cc
CFLAGS = -Wall -Wextra -O2

TARGET = httpd
CGI = docroot/dump.cgi
OBJS = main.o server.o io.o request.o response.o file.o env.o log.o cgi.o my_string.o my_malloc.o
TESTS = tests/test_my_string tests/test_my_malloc

.PHONY: all test clean

all: $(TARGET) $(CGI)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(CGI): cgi-bin/dump.c
	$(CC) $(CFLAGS) -o $@ $<

tests/test_my_string: tests/test_my_string.c my_string.c my_string.h
	$(CC) $(CFLAGS) -o $@ tests/test_my_string.c my_string.c

tests/test_my_malloc: tests/test_my_malloc.c my_malloc.c my_stdlib.h
	$(CC) $(CFLAGS) -o $@ tests/test_my_malloc.c my_malloc.c

test: all $(TESTS)
	tests/test_my_string
	tests/test_my_malloc
	tests/http.py

main.o: main.c server.h env.h log.h my_string.h
server.o: server.c server.h request.h response.h file.h log.h cgi.h my_stdlib.h
io.o: io.c io.h
my_string.o: my_string.c my_string.h
my_malloc.o: my_malloc.c my_stdlib.h
request.o: request.c request.h my_string.h
response.o: response.c response.h file.h io.h my_string.h
file.o: file.c file.h io.h my_string.h
env.o: env.c env.h log.h
log.o: log.c log.h
cgi.o: cgi.c cgi.h request.h response.h io.h

clean:
	rm -f $(TARGET) $(CGI) $(OBJS) $(TESTS)
