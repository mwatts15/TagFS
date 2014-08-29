CC = gcc

LEX = ../lex2.pl

# define any compile-time flags
CFLAGS = -std=c99 -Wall -g -gdwarf-2 -g3 `pkg-config --cflags glib-2.0` -D_POSIX_C_SOURCE=201809 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
INCLUDES =`pkg-config --cflags-only-I glib-2.0` -I.. -I.
LIBS = -lcunit -lpthread `pkg-config --libs glib-2.0`

# Almost all of the tests need these
TEST_PREREQS = test.o test.h ../util.o ../log.o

TESTS = test_trie test_key test_sqlite3 test_abstract_file test_stage test_file_cabinet

.PHONY: tests clean testdb depend

%.c : %.lc $(LEX)
	$(LEX) $<

all: $(TESTS)
acceptance_test: acceptance_test.pl
	./acceptance_test.pl

test_trie: test_trie.c ../trie.o ../key.o test.o ../util.o ../log.o test.h

test_key: test_key.c ../key.o test.o ../log.o test.h

test_file_cabinet: test_file_cabinet.c ../file_cabinet.o ../file_drawer.o ../file_drawer.h ../file.o ../file_drawer.o ../key.o ../abstract_file.o ../types.o ../scanner.o ../stream.o ../set_ops.o $(TEST_PREREQS)
test_abstract_file: LIBS += -lpthread
test_abstract_file: test_abstract_file.c ../abstract_file.o test.o ../key.o ../log.o test.h
test_stage: LIBS += -lpthread
test_stage: test_stage.c ../stage.o ../abstract_file.o ../util.o test.o ../key.o ../log.o test.h

test_sqlite3: LIBS += `pkg-config --libs sqlite3`
test_sqlite3: INCLUDES += `pkg-config --cflags sqlite3`
test_sqlite3: test.o test.h

% : %.c
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ $^ $(LIBS) 

testdb:
	perl generate_testdb.pl test.db 11 10 5 copies
# arguments : <database name> <number of files> 
#             <number of tags> <max tags per item> <copies directory>

clean:
	$(RM) *.o *~ *_out

cflags:
	echo $(INCLUDES) $(CFLAGS)

#%: force
	#@$(MAKE) -f ../Makefile $@
#force: ;
