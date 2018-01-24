include config.mk

.SUFFIXES:
.SUFFIXES: .o .c

INC= inc

HDR=\
	inc/arg.h\
	inc/compat.h\
	inc/pkg.h

# SOURCE
BIN=\
	src/lux

# LIB SOURCE
LIBBINSRC=\
	src/add.c\
	src/del.c\
	src/fetch.c\
	src/info.c\
	src/shared.c

LIBPKGSRC=\
	lib/pkg/db.c\
	lib/pkg/fs.c\
	lib/pkg/net.c\
	lib/pkg/node.c\
	lib/pkg/util.c

# LIB PATH
LIBBIN=   lib/libbin.a
LIBPKG=   lib/libpkg.a
LIBFETCH= lib/libfetch.a

# LIB OBJS
LIBBINOBJ=   $(LIBBINSRC:.c=.o)
LIBPKGOBJ=   $(LIBPKGSRC:.c=.o)
LIBFETCHOBJ= $(LIBFETCHSRC:.c=.o)

# ALL
LIB= $(LIBBIN) $(LIBPKG)
OBJ= $(BIN:=.o) $(LIBBINOBJ) $(LIBPKGOBJ)
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

# LIBRARIES RULES
$(LIBBIN): $(LIBBINOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBPKG): $(LIBPKGOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

# USER ACTIONS
clean:
	rm -f $(BIN) $(OBJ) $(LIB)

.PHONY:
	all clean
