CC = gcc

# define any compile-time flags
CFLAGS = -Wall -g `pkg-config --cflags glib-2.0` -I..
INCLUDES =`pkg-config --cflags-only-I glib-2.0` -I..
LIBS = `pkg-config --libs glib-2.0`
OBJS = ../code_table.o ../query.o ../result_queue.o ../set_ops.o \
  ../stream.o ../tagdb.o ../tagdb_priv.o ../tokenizer.o ../types.o ../util.o test_util.o

TEST_SRCS = test_code_table.c test_hash_sets.c test_query.c \
  test_result_queue.c test_tagdb.c test_tokenizer.c test_util.c

TESTS = t_code_table t_hash_sets t_query t_result_queue t_tagdb t_tokenizer 

.PHONY: tests clean depend
include ../makefile.dep

t_%: $(OBJS) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) test_$*.c -o $@

clean:
	$(RM) *.o *~ $(MAIN)

all : $(TESTS)

depend: $(TEST_SRCS)
	gcc -M $(INCLUDES) $(TEST_SRCS) -MF makefile.dep
