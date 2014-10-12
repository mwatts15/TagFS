CC ?= gcc

MARCO = ../marco.pl

# define any compile-time flags
CFLAGS = -std=c99 -Wall -g -gdwarf-2 -g3 `pkg-config --cflags glib-2.0` -D_POSIX_C_SOURCE=201809 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
INCLUDES =`pkg-config --cflags-only-I glib-2.0` -I.. -I.
LIBS = -lcunit -lpthread `pkg-config --libs glib-2.0`

# Almost all of the tests need these
OBJS = test.o ../util.o ../log.o
TEST_PREREQS = $(TEST_OBJS) test.h

FCAB:=../file_cabinet.o
LIBS+= -lsqlite3

TESTS = test_mmap test_log test_trie test_key test_sqlite3 test_abstract_file test_stage test_file_cabinet test_file test_tagdb

.PHONY: tests clean testdb depend

%.c : %.lc $(MARCO)
	$(MARCO) $<

all: test.o $(TEST_PREREQS) $(TESTS)
acceptance_test: acceptance_test.pl
	./acceptance_test.pl

test.o: test.c

test_trie: OBJS += ../trie.o ../key.o
test_trie: test_trie.c 
test_key: OBJS += ../key.o
test_key: test_key.c ../key.o

test_file_cabinet: OBJS += $(FCAB) ../file_drawer.o ../file.o ../key.o ../abstract_file.o ../types.o ../scanner.o ../stream.o ../set_ops.o
test_file_cabinet: test_file_cabinet.c

test_abstract_file: LIBS += -lpthread
test_abstract_file: OBJS += ../abstract_file.o ../key.o
test_abstract_file: test_abstract_file.c 

test_file: LIBS += -lpthread
test_file: OBJS += ../file.o ../scanner.o ../set_ops.o ../types.o ../abstract_file.o ../key.o ../stream.o
test_file: test_file.c

test_stage: LIBS += -lpthread
test_stage: OBJS += ../stage.o ../abstract_file.o ../key.o
test_stage: test_stage.c 

test_log: test_log.c

test_sqlite3: LIBS += `pkg-config --libs sqlite3`
test_sqlite3: INCLUDES += `pkg-config --cflags sqlite3`
test_sqlite3: test_sqlite3.c

test_mmap: test_mmap.c

test_tagdb: OBJS += ../file_cabinet.o ../file_drawer.o ../file.o ../key.o ../abstract_file.o ../types.o \
	../scanner.o ../stream.o ../set_ops.o ../tagdb.o ../tag.o ../tagdb_priv.o \
	../tagdb_util.o ../path_util.o
test_tagdb: test_tagdb.c 

test_% : test_%.c $(TEST_PREREQS)
	$(CC) -o $@ $@.c $(INCLUDES) $(CFLAGS) $(OBJS) $(LIBS)

testdb:
	perl generate_testdb.pl test.db 11 10 5 copies
# arguments : <database name> <number of files> 
#             <number of tags> <max tags per item> <copies directory>

clean:
	$(RM) *.o *~ *_out
	$(RM) $(TESTS:=.c)
	$(RM) $(TESTS)

cflags:
	echo $(INCLUDES) $(CFLAGS)

#%: force
	#@$(MAKE) -f ../Makefile $@
#force: ;
