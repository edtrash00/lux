include config.mk

.SUFFIXES:
.SUFFIXES: .o .c

INC= inc

HDR=\
	lib/fetch/ftperr.h\
	lib/fetch/httperr.h\
	inc/arg.h\
	inc/compat.h\
	inc/config.h\
	inc/fetch.h\
	inc/pkg.h

# SOURCE
BIN=\
	src/lux

MAN1=\
	man/lux.1

MAN5=\
	man/pkg_dbfile.5

# LIB SOURCE
LIBPKGSRC=\
	lib/fetch/common.c\
	lib/fetch/fetch.c\
	lib/fetch/file.c\
	lib/fetch/ftp.c\
	lib/fetch/http.c\
	lib/pkg/alloc.c\
	lib/pkg/ar.c\
	lib/pkg/db.c\
	lib/pkg/fs.c\
	lib/pkg/net.c\
	lib/pkg/str.c\
	lib/pkg/util.c

# LIB PATH
LIBPKG= lib/libpkg.a

# LIB OBJS
LIBPKGOBJ= $(LIBPKGSRC:.c=.o)

# ALL
LIB= $(LIBPKG)
OBJ= $(BIN:=.o) $(LIBPKGOBJ)
SRC= $(BIN:=.c)

# VAR RULES
all: $(BIN)

$(BIN): $(LIB) $(@:=.o)
$(OBJ): $(HDR) config.mk

# SUFFIX RULES
.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LIB) $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -I $(INC) -o $@ -c $<

# LIBRAY FETCH HDR
lib/fetch/ftperr.h: lib/fetch/ftp.errors
	lib/fetch/errlist.sh ftp_errlist FTP lib/fetch/ftp.errors > $@

lib/fetch/httperr.h: lib/fetch/http.errors
	lib/fetch/errlist.sh http_errlist HTTP lib/fetch/http.errors > $@

# LIBRARIES RULES
$(LIBPKG): $(LIBPKGOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBFETCH): $(LIBFETCHOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

# USER ACTIONS
install:
	install -dm 755 $(DESTDIR)/$(PREFIX)/bin
	install -csm 755 $(BIN) $(DESTDIR)/$(PREFIX)/bin

install-man:
	install -dm 755 $(DESTDIR)/$(MANPREFIX)/man1
	install -dm 755 $(DESTDIR)/$(MANPREFIX)/man5
	install -cm 644 $(MAN1) $(DESTDIR)/$(MANPREFIX)/man1
	install -cm 644 $(MAN5) $(DESTDIR)/$(MANPREFIX)/man5

clean:
	rm -f $(BIN) $(OBJ) $(LIB)
	rm -f lib/fetch/*err.h

.PHONY:
	all install install-man clean
