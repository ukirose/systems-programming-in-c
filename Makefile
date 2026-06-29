CC = cc
CFLAGS = -Wall -Wextra -O2 -Imylibc

# 成果物はソースと混ぜず、build/ 以下にまとめる
BUILD = build

TARGET = $(BUILD)/httpd/httpd
# CGI は配信対象なので docroot に置く。ここだけ build/ の外
CGI    = httpd/docroot/dump.cgi

HTTPD_OBJS  = $(addprefix $(BUILD)/httpd/, main.o server.o io.o request.o response.o file.o env.o log.o cgi.o)
MYLIBC_OBJS = $(addprefix $(BUILD)/mylibc/, my_string.o my_malloc.o my_snprintf.o)
OBJS  = $(HTTPD_OBJS) $(MYLIBC_OBJS)
TESTS = $(BUILD)/mylibc/tests/test_my_string $(BUILD)/mylibc/tests/test_my_malloc \
        $(BUILD)/mylibc/tests/test_my_snprintf

.PHONY: all test clean

all: $(TARGET) $(CGI)

$(TARGET): $(OBJS) | $(BUILD)/httpd
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD)/httpd/%.o: httpd/%.c | $(BUILD)/httpd
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/mylibc/%.o: mylibc/%.c | $(BUILD)/mylibc
	$(CC) $(CFLAGS) -c -o $@ $<

$(CGI): httpd/cgi-bin/dump.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/mylibc/tests/test_my_string: mylibc/tests/test_my_string.c mylibc/my_string.c mylibc/my_string.h | $(BUILD)/mylibc/tests
	$(CC) $(CFLAGS) -o $@ mylibc/tests/test_my_string.c mylibc/my_string.c

$(BUILD)/mylibc/tests/test_my_malloc: mylibc/tests/test_my_malloc.c mylibc/my_malloc.c mylibc/my_stdlib.h | $(BUILD)/mylibc/tests
	$(CC) $(CFLAGS) -o $@ mylibc/tests/test_my_malloc.c mylibc/my_malloc.c

$(BUILD)/mylibc/tests/test_my_snprintf: mylibc/tests/test_my_snprintf.c mylibc/my_snprintf.c mylibc/my_stdio.h | $(BUILD)/mylibc/tests
	$(CC) $(CFLAGS) -o $@ mylibc/tests/test_my_snprintf.c mylibc/my_snprintf.c

$(BUILD)/httpd $(BUILD)/mylibc $(BUILD)/mylibc/tests:
	mkdir -p $@

test: all $(TESTS)
	$(BUILD)/mylibc/tests/test_my_string
	$(BUILD)/mylibc/tests/test_my_malloc
	$(BUILD)/mylibc/tests/test_my_snprintf
	httpd/tests/http.py

$(BUILD)/httpd/main.o:     httpd/server.h httpd/env.h httpd/log.h mylibc/my_string.h
$(BUILD)/httpd/server.o:   httpd/server.h httpd/request.h httpd/response.h httpd/file.h httpd/log.h httpd/cgi.h mylibc/my_stdlib.h
$(BUILD)/httpd/io.o:       httpd/io.h
$(BUILD)/httpd/request.o:  httpd/request.h mylibc/my_string.h
$(BUILD)/httpd/response.o: httpd/response.h httpd/file.h httpd/io.h mylibc/my_string.h mylibc/my_stdio.h
$(BUILD)/httpd/file.o:     httpd/file.h httpd/io.h mylibc/my_string.h mylibc/my_stdio.h
$(BUILD)/httpd/env.o:      httpd/env.h httpd/log.h
$(BUILD)/httpd/log.o:      httpd/log.h mylibc/my_stdio.h
$(BUILD)/httpd/cgi.o:      httpd/cgi.h httpd/request.h httpd/response.h httpd/io.h
$(BUILD)/mylibc/my_string.o: mylibc/my_string.h
$(BUILD)/mylibc/my_malloc.o: mylibc/my_stdlib.h
$(BUILD)/mylibc/my_snprintf.o: mylibc/my_stdio.h

clean:
	rm -rf $(BUILD) $(CGI)
