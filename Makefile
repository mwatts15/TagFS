# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = gcc
LEX = ./lex.pl

# define any compile-time flags
CFLAGS = -Wall -g `pkg-config --cflags glib-2.0 fuse` -DTAGFS_BUILD

# define any directories containing header files other than /usr/include
#
INCLUDES = `pkg-config --cflags-only-I glib-2.0 fuse`
# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = `pkg-config --libs glib-2.0 fuse`

# define the C source files
SRCS = \
abstract_file.c \
file_drawer.c \
file_cabinet.c \
file.c \
log.c \
path_util.c \
result_queue.c \
trie.c \
scanner.c \
set_ops.c \
stream.c \
tag.c \
tagdb_priv.c \
tagdb.c \
tagfs.c \
types.c \
stage.c \
util.c \
tagdb_util.c \
result_to_fs.c \
query.c

#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

# define the executable file
MAIN = tagfs
#
# Targets
#

.PHONY: depend clean tests tags

%.c : %.l ; $(LEX) $<

all: $(MAIN) install
	@echo TagFS compiled.

install: $(MAIN)
	@echo installed

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS) $(LIBS)

tests:
	cd tests/; make all; ./do_tests.sh

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
#.c.o:
#$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ tagfs.c

depend: $(SRCS)
	gcc -MM $(CFLAGS) -MF makefile.dep tagfs.c

tags:
	ctags --langmap=c:.l.c.h *.c *.h *.l 

testdb:
	tests/generate_testdb.pl test.db 100 50 5 copies

include makefile.dep

