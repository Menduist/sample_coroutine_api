#find . -name "*.h" | cut -c 3- | xargs -n 1 -I "{}" echo "    {} \\"
CC=gcc
NAME=sample_api
LDLIBS=-ltca -lre -lyaml -lpq -lm
LDLOCS=-Llibtca
LDINCS=-Ilibtca
CFLAGS=-O0 -g -I. $(LDINCS) $(LDLIBS) $(LDLOCS)
SRCS= \
    main.c \
    coro.c \
    config.c \
    api.c \
    http.c \
    db/re_pgsql.c \
    db/db.c \

DEPS= \
    Makefile \
    coro.h \
    config.h \
    main.h \
    api.h \
    http.h \
    db/db.h \
    libtca/libtca.a \


all: $(NAME)

libtca/libtca.a: libtca
	make -C libtca

$(NAME): $(SRCS) $(DEPS)
	$(CC) $(SRCS) -o $(NAME) $(CFLAGS)

install: $(NAME)
	install -m 0755 $(NAME) $(prefix)/bin
	install -m 0644 $(NAME).service /lib/systemd/system/

clean:
	rm $(NAME)
