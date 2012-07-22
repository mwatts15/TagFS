CC = gcc

# define any compile-time flags
CFLAGS = -Wall -g `pkg-config --cflags glib-2.0` -I.. -I.
INCLUDES =`pkg-config --cflags-only-I glib-2.0` -I.. -I.
LIBS = `pkg-config --libs glib-2.0`

OBJS =\
../abstract_file.o \
../file_drawer.o \
../file_cabinet.o \
../file.o \
../log.o \
../scanner.o \
../set_ops.o \
../stream.o \
../tag.o \
../tagdb_priv.o \
../tagdb.o \
../types.o \
../util.o \
../tagdb_util.o \
../path_util.o \
../query.o \
../query_fs.o \
../query_fs_result_manager.o \
../result_to_fs.o \
fake_fuse.o \
test_util.o

TEST_SRCS = test_hash_sets.c test_query.c test_query_fs.c \
  test_tagdb.c test_scanner.c test_types.c test_util.c \
  fake_fuse.c

TESTS = t_hash_sets t_query t_tagdb t_scanner \
 t_types t_query_fs

.PHONY: tests clean testdb depend
include ../makefile.dep

t_%: $(OBJS) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) test_$*.c -o $@

testdb:
	perl generate_testdb.pl test.db 11 10 5 copies
# arguments : <database name> <number of files> 
#             <number of tags> <max tags per item> <copies directory>

clean:
	$(RM) *.o *~ t_* *_out

all : $(TESTS)

depend: $(TEST_SRCS)
	gcc -M $(INCLUDES) $(TEST_SRCS) -MF makefile.dep
