# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC?=gcc

MARCO = ./marco.pl

# define the executable file
MAIN = tagfs

OPT?=-Og

ifdef COVERAGE
CFLAGS += -ftest-coverage -fprofile-arcs
export COVERAGE
endif

# define any compile-time flags
ifdef RELEASE
OPT=-O2
endif

ifdef ASAN
CFLAGS += -fsanitize=address
endif

CFLAGS += $(OPT) -std=c99 -Wall -g -gdwarf-2 -g3 -fstack-protector `pkg-config --cflags glib-2.0 fuse` -D_POSIX_C_SOURCE=201809 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED -DTAGFS_BUILD

# define any directories containing header files other than /usr/include
#
INCLUDES = `pkg-config --cflags-only-I glib-2.0 fuse`
# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = `pkg-config --libs glib-2.0 fuse` -lpthread -lsqlite3
# define the C source files
SRCS = \
$(MAIN).c \
abstract_file.c \
file.c \
log.c \
file_log.c \
trie.c \
key.c \
set_ops.c \
tag.c \
tagdb.c \
types.c \
stage.c \
util.c \
tagdb_util.c \
subfs.c \
path_util.c \
tagdb_fs.c \
fs_util.c \
sql.c \
file_cabinet.c \
lock.c
#query.c \
#search_fs.c \

CFLAGS+= -DSQLITE_DEFAULT_MMAP_SIZE=268435456

#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

#
# Targets
#

.PHONY: depend clean tests tags

%.c : %.lc $(MARCO)
	$(MARCO) $<

all: $(MAIN)
	@echo TagFS compiled.

$(MAIN): version.h $(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS) $(LIBS)

version.h: get_version.sh .git/HEAD
	echo '#define TAGFS_VERSION "'`./get_version.sh`'"'> version.h

cflags:
	@echo $(CFLAGS)

srcs::
	@echo $(SRCS)


tests: clean 
	make -C tests unit_test

acc-test: $(MAIN)
	make -C tests acceptance_test

lt: lt.c
	gcc `pkg-config --cflags-only-I glib-2.0` -o lt lt.c

ts: ts.c
	gcc -Og -g `pkg-config --cflags-only-I glib-2.0` -o ts ts.c

pcmanfm-tags-module.la: pcmanfm-tags-module.c
	make -f Makefile.pcmanfm-module all

install-pcmanfm-ext: pcmanfm-tags-module.la
	make -f Makefile.pcmanfm-module install

uninstall-pcmanfm-ext: pcmanfm-tags-module.la
	make -f Makefile.pcmanfm-module uninstall

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
#.c.o:
#$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN).c $(MAIN) *.gcov *.gcda *.gcno version.h
	make -f Makefile.pcmanfm-module clean

depend: $(SRCS)
	gcc -MM $(CFLAGS) -MF makefile.dep $( MAIN )

tags:
	ctags --langmap=c:.lc.c.h *.c *.h *.lc

#include makefile.dep
