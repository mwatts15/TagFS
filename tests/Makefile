CC = gcc

# define any compile-time flags
CFLAGS = -Wall -g `pkg-config --cflags glib-2.0` -I..
INCLUDES =`pkg-config --cflags-only-I glib-2.0` -I..
LIBS = `pkg-config --libs glib-2.0`
OBJS = ../code_table.o ../query.o ../result_queue.o ../set_ops.o \
  ../stream.o ../tagdb.o ../tagdb_priv.o ../tokenizer.o ../types.o ../util.o test_util.o \
  ../log.o

TEST_SRCS = test_code_table.c test_hash_sets.c test_query.c \
  test_result_queue.c test_tagdb.c test_tokenizer.c test_util.c

TESTS = t_code_table t_hash_sets t_query t_result_queue t_tagdb t_tokenizer 

.PHONY: tests clean testdb depend
include ../makefile.dep

t_tokenizer: $(OBJS) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) test_tokenizer.c -o $@

MY_OBJS := ../tagdb.o ../tagdb_priv.o ../util.o ../types.o test_util.o ../tokenizer.o ../log.o \
../set_ops.o ../stream.o
t_tagdb: $(MY_OBJS) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(MY_OBJS) $(LIBS) test_tagdb.c -o $@

t_%: $(OBJS) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) test_$*.c -o $@

testdb:
	./generate_testdb.pl test.db 11 10 5 copies
# arguments : <database name> <number of files> 
#             <number of tags> <max tags per item> <copies directory>

clean:
	$(RM) *.o *~ t_* *_out

all : $(TESTS)

depend: $(TEST_SRCS)
	gcc -M $(INCLUDES) $(TEST_SRCS) -MF makefile.dep
