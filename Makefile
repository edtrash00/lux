include config.mk

.SUFFIXES:
.SUFFIXES: .o .c

INC= inc

HDR=\
	lib/fetch/ftperr.h\
	lib/fetch/httperr.h\
	inc/arg.h\
	inc/compat.h\
	inc/fetch.h\
	inc/pkg.h

# SOURCE
BIN=\
	src/lux

# LIB SOURCE
LIBBINSRC=\
	src/add.c\
	src/del.c\
	src/fetch.c\
	src/info.c

LIBPKGSRC=\
	lib/pkg/db.c\
	lib/pkg/download.c\
	lib/pkg/fs.c\
	lib/pkg/node.c\
	lib/pkg/util.c

LIBFETCHSRC=\
	lib/fetch/common.c\
	lib/fetch/fetch.c\
	lib/fetch/file.c\
	lib/fetch/ftp.c\
	lib/fetch/http.c

# LIB PATH
LIBBIN=   lib/libbin.a
LIBPKG=   lib/libpkg.a
LIBFETCH= lib/libfetch.a

# LIB OBJS
LIBBINOBJ=   $(LIBBINSRC:.c=.o)
LIBPKGOBJ=   $(LIBPKGSRC:.c=.o)
LIBFETCHOBJ= $(LIBFETCHSRC:.c=.o)

# ALL
LIB= $(LIBBIN) $(LIBPKG) $(LIBFETCH)
OBJ= $(BIN:=.o) $(LIBBINOBJ) $(LIBPKGOBJ) $(LIBFETCHOBJ)
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
$(LIBBIN): $(LIBBINOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBFETCH): $(LIBFETCHOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBPKG): $(LIBPKGOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

# USER ACTIONS
clean:
	rm -f $(BIN) $(OBJ) $(LIB)
	rm -f lib/fetch/*err.h

.PHONY:
	all clean
