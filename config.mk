AR = ar
CC = cc
RANLIB= ranlib

CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_FILE_OFFSET_BITS=64
CFLAGS   = -Os -std=c99 -Wall -pedantic
LDFLAGS  = -g
LDLIBS   = -lcurl

# PATHS
PREFIX = /usr/local
