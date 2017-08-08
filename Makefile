include config.mk

.SUFFIXES:
.SUFFIXES: .o .c

INC= inc

HDR=\
	inc/pkg.h\
	inc/db.h\
	inc/fs.h

# SOURCE
BIN=\
	src/pkg_add\
	src/pkg_del\
	src/pkg_info

# LIB SOURCE
LIBDBSRC=\
	lib/db/db.c\
	lib/db/node.c

LIBFSSRC=\
	lib/fs/cp.c\
	lib/fs/pcat.c\
	lib/fs/rm.c

# LIB PATH
LIBDB= lib/libdb.a
LIBFS= lib/libfs.a

# LIB OBJS
LIBDBOBJ= $(LIBDBSRC:.c=.o)
LIBFSOBJ= $(LIBFSSRC:.c=.o)

# ALL
LIB= $(LIBDB) $(LIBFS)
OBJ= $(BIN:=.o) $(LIBDBOBJ) $(LIBFSOBJ)
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
$(LIBDB): $(LIBDBOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBFS): $(LIBFSOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

# USER ACTIONS
clean:
	rm -f $(BIN) $(OBJ) $(LIB) utilchest

.PHONY:
	all clean
