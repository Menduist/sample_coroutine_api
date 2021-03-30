#find . -name "*.h" | cut -c 3- | xargs -n 1 -I "{}" echo "    {} \\"
CC=gcc
NAME=sample_api
LDLIBS=-ltca -lre -lyaml -lpq -lm
LDLOCS=-Lsrc/libtca
LDINCS=-Isrc/libtca
CFLAGS=-O0 -g -I. $(LDINCS) $(LDLIBS) $(LDLOCS)
SRCS= \
    src/main.c \
    src/coro.c \
    src/config.c \
    src/api.c \
    src/http.c \
    src/db/re_pgsql.c \
    src/db/db.c \

DEPS= \
    Makefile \
    src/coro.h \
    src/config.h \
    src/main.h \
    src/api.h \
    src/http.h \
    src/db/db.h \
    src/libtca/libtca.a \


all: $(NAME)

src/libtca/libtca.a: src/libtca
	make -C src/libtca

$(NAME): $(SRCS) $(DEPS)
	$(CC) $(SRCS) -o $(NAME) $(CFLAGS)

install: $(NAME)
	install -m 0755 $(NAME) $(prefix)/bin
	install -m 0644 $(NAME).service /lib/systemd/system/

clean:
	rm $(NAME)
