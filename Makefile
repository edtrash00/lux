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
	src/pkg

# LIB SOURCE
LIBPKGSRC=\
	lib/pkg/db.c\
	lib/pkg/fs.c\
	lib/pkg/node.c

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
	$(CC) $(LDFLAGS) -o $@ $< $(LIB)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -I $(INC) -o $@ -c $<

# LIBRARIES RULES
$(LIBPKG): $(LIBPKGOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

# USER ACTIONS
clean:
	rm -f $(BIN) $(OBJ) $(LIB)

.PHONY:
	all clean
