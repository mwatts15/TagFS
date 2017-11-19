# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
PROJECT_ROOT=..
BUILD=$(PROJECT_ROOT)/build
MARCO = $(PROJECT_ROOT)/src/marco.pl

PROJECT_INC=$(PROJECT_ROOT)/include

include $(PROJECT_ROOT)/common.mk
include $(PROJECT_ROOT)/compile.mk

CC?=gcc

# define the executable file
MAIN = tagfs

OPT?=-Og

PREFIX?=/usr/local

export PERLLIB=$(shell readlink -f ../lib/perl)

LCS:=$(wildcard *.lc)
LC_C_INTERMEDIATES:=$(LCS:.lc=.c)


ifdef COVERAGE
CFLAGS += -fprofile-arcs -ftest-coverage
LIBS=-lgcov
export COVERAGE
endif

ifdef PROFILE
CFLAGS += -fprofile-arcs -pg
endif

# define any compile-time flags
ifdef RELEASE
OPT=-O2
endif

ifdef ASAN
CFLAGS += -fsanitize=address
endif

CFLAGS += $(OPT) -std=c99 -Wall -Wshadow -Wunreachable-code \
		  -g -gdwarf-2 -g3 -fstack-protector \
		  `pkg-config --cflags glib-2.0 gio-2.0 dbus-1` \
		  -D_POSIX_C_SOURCE=201809 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED

ifdef FUZZER_BUILD
CFLAGS += -DFAKE_FUSE
else
CFLAGS += -DTAGFS_BUILD `pkg-config --cflags fuse`
endif

# define any directories containing header files other than /usr/include
#
INCLUDES = `pkg-config --cflags-only-I glib-2.0 gio-2.0` -I$(PROJECT_INC) -I.

ifndef FUZZER_BUILD
INCLUDES+= `pkg-config --cflags-only-I fuse`
endif

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = `pkg-config --libs glib-2.0 gio-2.0 dbus-1` -lpthread -lsqlite3

ifndef FUZZER_BUILD
LIBS+= `pkg-config --libs fuse`
endif

# define the C source files
SRCS = \
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
message.c \
message_dbus.c \
tagdb_fs.c \
fs_util.c \
sql.c \
file_cabinet.c \
lock.c \
command_default.c \
tagdb_commands.c \
command.c \
command_fs.c \
plugin_manager.c \
glib_future.c \
plugin_commands.c \
glib_log.c \
tagfs_operations.c \
tagfs_common.c

ifdef FUZZER_BUILD
SRCS += fuzzer.c fake_fuse.c
else
SRCS += $(MAIN).c
endif

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

all: $(MAIN)
	@echo TagFS compiled.

$(MAIN): $(PROJECT_INC)/tagfs.h version.h $(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS) $(LIBS)

fuzzer: $(PROJECT_INC)/tagfs.h version.h $(OBJS)
	$(CC) $(CFLAGS) -o fuzzer $(OBJS) $(LIBS)

version.h::
	if [ -d $(PROJECT_ROOT)/.git ] ; then \
	echo '#define TAGFS_VERSION "'`git describe --abbrev=10 --dirty `'"'> version.h ; \
	fi

cflags:
	@echo $(CFLAGS)

srcs::
	@echo $(SRCS)

plugins/%.h: plugins/%.xml
	cat $< | xxd -i $< $@

tests: clean 
	make -C tests unit_test

acc-test: $(MAIN)
	make -C tests acceptance_test

lt: lt.c
	gcc -Og -g $(INCLUDES) -o lt lt.c

ts: ts.c
	gcc -Og -g $(INCLUDES) -o ts ts.c

rt: rt.c
	gcc -Og -g $(INCLUDES) -o rt rt.c

install: $(MAIN) rt ts lt
	install --directory $(PREFIX)/usr/bin
	install $? $(PREFIX)/usr/bin

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
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

plugin_manager.o: plugins/cc.markw.tagfs.TagListPopulator1.h

clean:
	make -C tests clean
	$(RM) *.o *~ $(MAIN).c $(MAIN) *.gcov *.gcda *.gcno gmon.out lt rt ts marco.log 
	$(RM) plugins/*.h
	make -f Makefile.pcmanfm-module clean

depend: $(SRCS)
	gcc -MM $(CFLAGS) -MF makefile.dep $(MAIN)

tags:
	ctags --langmap=c:.lc.c.h *.c *.h *.lc

#$(PROJECT_ROOT)/%::
	#make -C $(PROJECT_ROOT) $*
#include makefile.dep
