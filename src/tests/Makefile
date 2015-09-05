CC=gcc

MARCO = ../marco.pl
# define any compile-time flags

ifdef COVERAGE
CFLAGS=-ftest-coverage -fprofile-arcs
LIBS=-lgcov
export COVERAGE
endif

CFLAGS += -std=c99 -Wall -g -gdwarf-2 -g3 `pkg-config --cflags glib-2.0` -D_POSIX_C_SOURCE=201809 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
INCLUDES =`pkg-config --cflags-only-I glib-2.0` -I.. -I. -I../../include
LIBS += -lcunit -lpthread `pkg-config --libs glib-2.0`

# Almost all of the tests need these
OBJS = test.o ../util.o ../log.o
TEST_PREREQS = $(TEST_OBJS) test.h $(OBJS)

FCAB:=../file_cabinet.o
LIBS+= -lsqlite3

TESTS ?= test_mmap test_log test_trie test_key test_sqlite3 test_abstract_file test_stage test_file_cabinet test_file test_tag test_tagdb test_sql test_fc_cache test_command

.PHONY: tests clean testdb depend

%.c : %.lc $(MARCO)
	$(MARCO) $<

all: test.o $(TEST_PREREQS) $(TESTS)
acceptance_test: acceptance_test.pl
	./acceptance_test.pl

unit_test: $(TESTS)
ifdef COVERAGE
	rm -rf BASE OUT FILTERED RESULT test-coverage 2>/dev/null
	lcov --no-external --capture --initial --directory `readlink -f ..` -o BASE
	./do_tests.sh
	lcov --no-external --capture --directory `readlink -f ..` -o OUT
	lcov -a BASE -a OUT -o RESULT
	lcov --remove RESULT '/usr/include/*' -o FILTERED
	mkdir test-coverage 2>/dev/null
	genhtml --prefix `readlink -f ..` --output-directory="test-coverage" FILTERED
else
	./do_tests.sh
endif

test.o: test.c

test_trie: OBJS += ../trie.o ../key.o
test_trie: test_trie.c 
test_key: OBJS += ../key.o
test_key: test_key.c ../key.o

test_file_cabinet: OBJS += $(FCAB) ../file.o ../key.o ../abstract_file.o ../types.o ../set_ops.o ../sql.o  ../lock.o
test_file_cabinet: OBJS += ../tagdb.o ../tag.o ../tagdb_util.o
test_file_cabinet: test_file_cabinet.c

test_abstract_file: LIBS += -lpthread
test_abstract_file: OBJS += ../abstract_file.o ../key.o ../lock.o
test_abstract_file: test_abstract_file.c 

test_tag: LIBS += -lpthread
test_tag: OBJS += ../tag.o ../set_ops.o ../types.o ../abstract_file.o ../key.o ../lock.o
test_tag: test_tag.c

test_file: LIBS += -lpthread
test_file: OBJS += ../file.o ../set_ops.o ../types.o ../abstract_file.o ../key.o ../lock.o
test_file: test_file.c

test_fc_cache: OBJS += ../fc_cache.o
test_fc_cache: test_fc_cache.c

test_stage: LIBS += -lpthread
test_stage: OBJS += ../stage.o ../abstract_file.o ../key.o ../trie.o ../lock.o
test_stage: test_stage.c 

test_log: test_log.c

test_sqlite3: LIBS += `pkg-config --libs sqlite3`
test_sqlite3: INCLUDES += `pkg-config --cflags sqlite3`
test_sqlite3: test_sqlite3.c

test_sql: LIBS += `pkg-config --libs sqlite3`
test_sql: INCLUDES += `pkg-config --cflags sqlite3`
test_sql: OBJS += ../sql.o
test_sql: test_sqlite3.c

test_mmap: test_mmap.c

test_tagdb: OBJS += ../file_cabinet.o ../file.o ../key.o ../abstract_file.o ../types.o \
	 ../set_ops.o ../tagdb.o ../tag.o ../lock.o \
	../tagdb_util.o ../path_util.o ../sql.o
test_tagdb: test_tagdb.c

test_command: OBJS += ../command.o
test_command: test_command.c

# This makes $(OBJS) work the way we want it to, updating the prereqs
.SECONDEXPANSION:

test_% : test_%.c $(TEST_PREREQS) $$(OBJS)
	$(CC) -o $@ $@.c $(INCLUDES) $(CFLAGS) $(OBJS) $(LIBS)
# arguments : <database name> <number of files> 
#             <number of tags> <max tags per item> <copies directory>

../%.o: ../%.c
	COVERAGE=$(COVERAGE) make -C .. $*.o

clean:
	$(RM) *.o *~ *_out
	$(RM) $(TESTS:=.c) $(TESTS) $(TESTS:=.gcda) $(TESTS:=.gcno)

cflags:
	@echo $(INCLUDES) $(CFLAGS)
