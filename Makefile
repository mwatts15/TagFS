# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS = -Wall -g `pkg-config --cflags glib-2.0 fuse`

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
SRCS = code_table.c log.c query.c result_queue.c set_ops.c stream.c \
tagdb.c tagdb_priv.c tagfs.c \
tokenizer.c types.c util.c 

# define the C object files 
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

.PHONY: depend clean

all: $(MAIN)
	@echo TagFS compiled.

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS) $(LIBS)

testdb:
	./generate_testdb.pl test.db 26 50 10 copies

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
#.c.o:
	#$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it

code_table.o: code_table.h /usr/include/glib-2.0/glib.h
code_table.o: /usr/include/glib-2.0/glib/galloca.h
code_table.o: /usr/include/glib-2.0/glib/gtypes.h
code_table.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
code_table.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
code_table.o: /usr/include/features.h
code_table.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
code_table.o: /usr/include/xlocale.h /usr/include/glib-2.0/glib/garray.h
code_table.o: /usr/include/glib-2.0/glib/gasyncqueue.h
code_table.o: /usr/include/glib-2.0/glib/gthread.h
code_table.o: /usr/include/glib-2.0/glib/gatomic.h
code_table.o: /usr/include/glib-2.0/glib/gerror.h
code_table.o: /usr/include/glib-2.0/glib/gquark.h
code_table.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
code_table.o: /usr/include/glib-2.0/glib/gbase64.h
code_table.o: /usr/include/glib-2.0/glib/gbitlock.h
code_table.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
code_table.o: /usr/include/glib-2.0/glib/gbytes.h
code_table.o: /usr/include/glib-2.0/glib/gcharset.h
code_table.o: /usr/include/glib-2.0/glib/gchecksum.h
code_table.o: /usr/include/glib-2.0/glib/gconvert.h
code_table.o: /usr/include/glib-2.0/glib/gdataset.h
code_table.o: /usr/include/glib-2.0/glib/gdate.h
code_table.o: /usr/include/glib-2.0/glib/gdatetime.h
code_table.o: /usr/include/glib-2.0/glib/gtimezone.h
code_table.o: /usr/include/glib-2.0/glib/gdir.h
code_table.o: /usr/include/glib-2.0/glib/genviron.h
code_table.o: /usr/include/glib-2.0/glib/gfileutils.h
code_table.o: /usr/include/glib-2.0/glib/ggettext.h
code_table.o: /usr/include/glib-2.0/glib/ghash.h
code_table.o: /usr/include/glib-2.0/glib/glist.h
code_table.o: /usr/include/glib-2.0/glib/gmem.h
code_table.o: /usr/include/glib-2.0/glib/ghmac.h
code_table.o: /usr/include/glib-2.0/glib/gchecksum.h
code_table.o: /usr/include/glib-2.0/glib/ghook.h
code_table.o: /usr/include/glib-2.0/glib/ghostutils.h
code_table.o: /usr/include/glib-2.0/glib/giochannel.h
code_table.o: /usr/include/glib-2.0/glib/gmain.h
code_table.o: /usr/include/glib-2.0/glib/gpoll.h
code_table.o: /usr/include/glib-2.0/glib/gslist.h
code_table.o: /usr/include/glib-2.0/glib/gstring.h
code_table.o: /usr/include/glib-2.0/glib/gunicode.h
code_table.o: /usr/include/glib-2.0/glib/gutils.h
code_table.o: /usr/include/glib-2.0/glib/gkeyfile.h
code_table.o: /usr/include/glib-2.0/glib/gmappedfile.h
code_table.o: /usr/include/glib-2.0/glib/gmarkup.h
code_table.o: /usr/include/glib-2.0/glib/gmessages.h
code_table.o: /usr/include/glib-2.0/glib/gnode.h
code_table.o: /usr/include/glib-2.0/glib/goption.h
code_table.o: /usr/include/glib-2.0/glib/gpattern.h
code_table.o: /usr/include/glib-2.0/glib/gprimes.h
code_table.o: /usr/include/glib-2.0/glib/gqsort.h
code_table.o: /usr/include/glib-2.0/glib/gqueue.h
code_table.o: /usr/include/glib-2.0/glib/grand.h
code_table.o: /usr/include/glib-2.0/glib/gregex.h
code_table.o: /usr/include/glib-2.0/glib/gscanner.h
code_table.o: /usr/include/glib-2.0/glib/gsequence.h
code_table.o: /usr/include/glib-2.0/glib/gshell.h
code_table.o: /usr/include/glib-2.0/glib/gslice.h
code_table.o: /usr/include/glib-2.0/glib/gspawn.h
code_table.o: /usr/include/glib-2.0/glib/gstrfuncs.h
code_table.o: /usr/include/glib-2.0/glib/gstringchunk.h
code_table.o: /usr/include/glib-2.0/glib/gtestutils.h
code_table.o: /usr/include/glib-2.0/glib/gthreadpool.h
code_table.o: /usr/include/glib-2.0/glib/gtimer.h
code_table.o: /usr/include/glib-2.0/glib/gtrashstack.h
code_table.o: /usr/include/glib-2.0/glib/gtree.h
code_table.o: /usr/include/glib-2.0/glib/gurifuncs.h
code_table.o: /usr/include/glib-2.0/glib/gvarianttype.h
code_table.o: /usr/include/glib-2.0/glib/gvariant.h
code_table.o: /usr/include/glib-2.0/glib/gversion.h
code_table.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
code_table.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
code_table.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
code_table.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
code_table.o: /usr/include/glib-2.0/glib/deprecated/grel.h
code_table.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
code_table.o: /usr/include/pthread.h /usr/include/endian.h
code_table.o: /usr/include/sched.h /usr/include/stdlib.h
code_table.o: /usr/include/alloca.h
log.o: params.h /usr/include/stdio.h /usr/include/features.h
log.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
log.o: tagdb.h /usr/include/glib-2.0/glib.h
log.o: /usr/include/glib-2.0/glib/galloca.h
log.o: /usr/include/glib-2.0/glib/gtypes.h
log.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
log.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
log.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
log.o: /usr/include/xlocale.h /usr/include/glib-2.0/glib/garray.h
log.o: /usr/include/glib-2.0/glib/gasyncqueue.h
log.o: /usr/include/glib-2.0/glib/gthread.h
log.o: /usr/include/glib-2.0/glib/gatomic.h
log.o: /usr/include/glib-2.0/glib/gerror.h
log.o: /usr/include/glib-2.0/glib/gquark.h
log.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
log.o: /usr/include/glib-2.0/glib/gbase64.h
log.o: /usr/include/glib-2.0/glib/gbitlock.h
log.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
log.o: /usr/include/glib-2.0/glib/gbytes.h
log.o: /usr/include/glib-2.0/glib/gcharset.h
log.o: /usr/include/glib-2.0/glib/gchecksum.h
log.o: /usr/include/glib-2.0/glib/gconvert.h
log.o: /usr/include/glib-2.0/glib/gdataset.h
log.o: /usr/include/glib-2.0/glib/gdate.h
log.o: /usr/include/glib-2.0/glib/gdatetime.h
log.o: /usr/include/glib-2.0/glib/gtimezone.h
log.o: /usr/include/glib-2.0/glib/gdir.h
log.o: /usr/include/glib-2.0/glib/genviron.h
log.o: /usr/include/glib-2.0/glib/gfileutils.h
log.o: /usr/include/glib-2.0/glib/ggettext.h
log.o: /usr/include/glib-2.0/glib/ghash.h /usr/include/glib-2.0/glib/glist.h
log.o: /usr/include/glib-2.0/glib/gmem.h /usr/include/glib-2.0/glib/ghmac.h
log.o: /usr/include/glib-2.0/glib/gchecksum.h
log.o: /usr/include/glib-2.0/glib/ghook.h
log.o: /usr/include/glib-2.0/glib/ghostutils.h
log.o: /usr/include/glib-2.0/glib/giochannel.h
log.o: /usr/include/glib-2.0/glib/gmain.h /usr/include/glib-2.0/glib/gpoll.h
log.o: /usr/include/glib-2.0/glib/gslist.h
log.o: /usr/include/glib-2.0/glib/gstring.h
log.o: /usr/include/glib-2.0/glib/gunicode.h
log.o: /usr/include/glib-2.0/glib/gutils.h
log.o: /usr/include/glib-2.0/glib/gkeyfile.h
log.o: /usr/include/glib-2.0/glib/gmappedfile.h
log.o: /usr/include/glib-2.0/glib/gmarkup.h
log.o: /usr/include/glib-2.0/glib/gmessages.h
log.o: /usr/include/glib-2.0/glib/gnode.h
log.o: /usr/include/glib-2.0/glib/goption.h
log.o: /usr/include/glib-2.0/glib/gpattern.h
log.o: /usr/include/glib-2.0/glib/gprimes.h
log.o: /usr/include/glib-2.0/glib/gqsort.h
log.o: /usr/include/glib-2.0/glib/gqueue.h /usr/include/glib-2.0/glib/grand.h
log.o: /usr/include/glib-2.0/glib/gregex.h
log.o: /usr/include/glib-2.0/glib/gscanner.h
log.o: /usr/include/glib-2.0/glib/gsequence.h
log.o: /usr/include/glib-2.0/glib/gshell.h
log.o: /usr/include/glib-2.0/glib/gslice.h
log.o: /usr/include/glib-2.0/glib/gspawn.h
log.o: /usr/include/glib-2.0/glib/gstrfuncs.h
log.o: /usr/include/glib-2.0/glib/gstringchunk.h
log.o: /usr/include/glib-2.0/glib/gtestutils.h
log.o: /usr/include/glib-2.0/glib/gthreadpool.h
log.o: /usr/include/glib-2.0/glib/gtimer.h
log.o: /usr/include/glib-2.0/glib/gtrashstack.h
log.o: /usr/include/glib-2.0/glib/gtree.h
log.o: /usr/include/glib-2.0/glib/gurifuncs.h
log.o: /usr/include/glib-2.0/glib/gvarianttype.h
log.o: /usr/include/glib-2.0/glib/gvariant.h
log.o: /usr/include/glib-2.0/glib/gversion.h
log.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
log.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
log.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
log.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
log.o: /usr/include/glib-2.0/glib/deprecated/grel.h
log.o: /usr/include/glib-2.0/glib/deprecated/gthread.h /usr/include/pthread.h
log.o: /usr/include/endian.h /usr/include/sched.h code_table.h
log.o: /usr/include/stdlib.h /usr/include/alloca.h types.h result_queue.h
log.o: query.h /usr/include/unistd.h /usr/include/getopt.h
log.o: /usr/include/fuse/fuse.h /usr/include/fuse/fuse_common.h
log.o: /usr/include/fuse/fuse_opt.h /usr/include/stdint.h
log.o: /usr/include/fuse/fuse_common_compat.h /usr/include/fcntl.h
log.o: /usr/include/utime.h /usr/include/fuse/fuse_compat.h log.h
query.o: /usr/include/malloc.h /usr/include/features.h /usr/include/stdio.h
query.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
query.o: query.h tagdb.h /usr/include/glib-2.0/glib.h
query.o: /usr/include/glib-2.0/glib/galloca.h
query.o: /usr/include/glib-2.0/glib/gtypes.h
query.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
query.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
query.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
query.o: /usr/include/xlocale.h /usr/include/glib-2.0/glib/garray.h
query.o: /usr/include/glib-2.0/glib/gasyncqueue.h
query.o: /usr/include/glib-2.0/glib/gthread.h
query.o: /usr/include/glib-2.0/glib/gatomic.h
query.o: /usr/include/glib-2.0/glib/gerror.h
query.o: /usr/include/glib-2.0/glib/gquark.h
query.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
query.o: /usr/include/glib-2.0/glib/gbase64.h
query.o: /usr/include/glib-2.0/glib/gbitlock.h
query.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
query.o: /usr/include/glib-2.0/glib/gbytes.h
query.o: /usr/include/glib-2.0/glib/gcharset.h
query.o: /usr/include/glib-2.0/glib/gchecksum.h
query.o: /usr/include/glib-2.0/glib/gconvert.h
query.o: /usr/include/glib-2.0/glib/gdataset.h
query.o: /usr/include/glib-2.0/glib/gdate.h
query.o: /usr/include/glib-2.0/glib/gdatetime.h
query.o: /usr/include/glib-2.0/glib/gtimezone.h
query.o: /usr/include/glib-2.0/glib/gdir.h
query.o: /usr/include/glib-2.0/glib/genviron.h
query.o: /usr/include/glib-2.0/glib/gfileutils.h
query.o: /usr/include/glib-2.0/glib/ggettext.h
query.o: /usr/include/glib-2.0/glib/ghash.h
query.o: /usr/include/glib-2.0/glib/glist.h /usr/include/glib-2.0/glib/gmem.h
query.o: /usr/include/glib-2.0/glib/ghmac.h
query.o: /usr/include/glib-2.0/glib/gchecksum.h
query.o: /usr/include/glib-2.0/glib/ghook.h
query.o: /usr/include/glib-2.0/glib/ghostutils.h
query.o: /usr/include/glib-2.0/glib/giochannel.h
query.o: /usr/include/glib-2.0/glib/gmain.h
query.o: /usr/include/glib-2.0/glib/gpoll.h
query.o: /usr/include/glib-2.0/glib/gslist.h
query.o: /usr/include/glib-2.0/glib/gstring.h
query.o: /usr/include/glib-2.0/glib/gunicode.h
query.o: /usr/include/glib-2.0/glib/gutils.h
query.o: /usr/include/glib-2.0/glib/gkeyfile.h
query.o: /usr/include/glib-2.0/glib/gmappedfile.h
query.o: /usr/include/glib-2.0/glib/gmarkup.h
query.o: /usr/include/glib-2.0/glib/gmessages.h
query.o: /usr/include/glib-2.0/glib/gnode.h
query.o: /usr/include/glib-2.0/glib/goption.h
query.o: /usr/include/glib-2.0/glib/gpattern.h
query.o: /usr/include/glib-2.0/glib/gprimes.h
query.o: /usr/include/glib-2.0/glib/gqsort.h
query.o: /usr/include/glib-2.0/glib/gqueue.h
query.o: /usr/include/glib-2.0/glib/grand.h
query.o: /usr/include/glib-2.0/glib/gregex.h
query.o: /usr/include/glib-2.0/glib/gscanner.h
query.o: /usr/include/glib-2.0/glib/gsequence.h
query.o: /usr/include/glib-2.0/glib/gshell.h
query.o: /usr/include/glib-2.0/glib/gslice.h
query.o: /usr/include/glib-2.0/glib/gspawn.h
query.o: /usr/include/glib-2.0/glib/gstrfuncs.h
query.o: /usr/include/glib-2.0/glib/gstringchunk.h
query.o: /usr/include/glib-2.0/glib/gtestutils.h
query.o: /usr/include/glib-2.0/glib/gthreadpool.h
query.o: /usr/include/glib-2.0/glib/gtimer.h
query.o: /usr/include/glib-2.0/glib/gtrashstack.h
query.o: /usr/include/glib-2.0/glib/gtree.h
query.o: /usr/include/glib-2.0/glib/gurifuncs.h
query.o: /usr/include/glib-2.0/glib/gvarianttype.h
query.o: /usr/include/glib-2.0/glib/gvariant.h
query.o: /usr/include/glib-2.0/glib/gversion.h
query.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
query.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
query.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
query.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
query.o: /usr/include/glib-2.0/glib/deprecated/grel.h
query.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
query.o: /usr/include/pthread.h /usr/include/endian.h /usr/include/sched.h
query.o: code_table.h /usr/include/stdlib.h /usr/include/alloca.h types.h
query.o: util.h tokenizer.h stream.h set_ops.h
result_queue.o: /usr/include/glib-2.0/glib.h
result_queue.o: /usr/include/glib-2.0/glib/galloca.h
result_queue.o: /usr/include/glib-2.0/glib/gtypes.h
result_queue.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
result_queue.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
result_queue.o: /usr/include/features.h
result_queue.o: /usr/include/glib-2.0/glib/gversionmacros.h
result_queue.o: /usr/include/time.h /usr/include/xlocale.h
result_queue.o: /usr/include/glib-2.0/glib/garray.h
result_queue.o: /usr/include/glib-2.0/glib/gasyncqueue.h
result_queue.o: /usr/include/glib-2.0/glib/gthread.h
result_queue.o: /usr/include/glib-2.0/glib/gatomic.h
result_queue.o: /usr/include/glib-2.0/glib/gerror.h
result_queue.o: /usr/include/glib-2.0/glib/gquark.h
result_queue.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
result_queue.o: /usr/include/glib-2.0/glib/gbase64.h
result_queue.o: /usr/include/glib-2.0/glib/gbitlock.h
result_queue.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
result_queue.o: /usr/include/glib-2.0/glib/gbytes.h
result_queue.o: /usr/include/glib-2.0/glib/gcharset.h
result_queue.o: /usr/include/glib-2.0/glib/gchecksum.h
result_queue.o: /usr/include/glib-2.0/glib/gconvert.h
result_queue.o: /usr/include/glib-2.0/glib/gdataset.h
result_queue.o: /usr/include/glib-2.0/glib/gdate.h
result_queue.o: /usr/include/glib-2.0/glib/gdatetime.h
result_queue.o: /usr/include/glib-2.0/glib/gtimezone.h
result_queue.o: /usr/include/glib-2.0/glib/gdir.h
result_queue.o: /usr/include/glib-2.0/glib/genviron.h
result_queue.o: /usr/include/glib-2.0/glib/gfileutils.h
result_queue.o: /usr/include/glib-2.0/glib/ggettext.h
result_queue.o: /usr/include/glib-2.0/glib/ghash.h
result_queue.o: /usr/include/glib-2.0/glib/glist.h
result_queue.o: /usr/include/glib-2.0/glib/gmem.h
result_queue.o: /usr/include/glib-2.0/glib/ghmac.h
result_queue.o: /usr/include/glib-2.0/glib/gchecksum.h
result_queue.o: /usr/include/glib-2.0/glib/ghook.h
result_queue.o: /usr/include/glib-2.0/glib/ghostutils.h
result_queue.o: /usr/include/glib-2.0/glib/giochannel.h
result_queue.o: /usr/include/glib-2.0/glib/gmain.h
result_queue.o: /usr/include/glib-2.0/glib/gpoll.h
result_queue.o: /usr/include/glib-2.0/glib/gslist.h
result_queue.o: /usr/include/glib-2.0/glib/gstring.h
result_queue.o: /usr/include/glib-2.0/glib/gunicode.h
result_queue.o: /usr/include/glib-2.0/glib/gutils.h
result_queue.o: /usr/include/glib-2.0/glib/gkeyfile.h
result_queue.o: /usr/include/glib-2.0/glib/gmappedfile.h
result_queue.o: /usr/include/glib-2.0/glib/gmarkup.h
result_queue.o: /usr/include/glib-2.0/glib/gmessages.h
result_queue.o: /usr/include/glib-2.0/glib/gnode.h
result_queue.o: /usr/include/glib-2.0/glib/goption.h
result_queue.o: /usr/include/glib-2.0/glib/gpattern.h
result_queue.o: /usr/include/glib-2.0/glib/gprimes.h
result_queue.o: /usr/include/glib-2.0/glib/gqsort.h
result_queue.o: /usr/include/glib-2.0/glib/gqueue.h
result_queue.o: /usr/include/glib-2.0/glib/grand.h
result_queue.o: /usr/include/glib-2.0/glib/gregex.h
result_queue.o: /usr/include/glib-2.0/glib/gscanner.h
result_queue.o: /usr/include/glib-2.0/glib/gsequence.h
result_queue.o: /usr/include/glib-2.0/glib/gshell.h
result_queue.o: /usr/include/glib-2.0/glib/gslice.h
result_queue.o: /usr/include/glib-2.0/glib/gspawn.h
result_queue.o: /usr/include/glib-2.0/glib/gstrfuncs.h
result_queue.o: /usr/include/glib-2.0/glib/gstringchunk.h
result_queue.o: /usr/include/glib-2.0/glib/gtestutils.h
result_queue.o: /usr/include/glib-2.0/glib/gthreadpool.h
result_queue.o: /usr/include/glib-2.0/glib/gtimer.h
result_queue.o: /usr/include/glib-2.0/glib/gtrashstack.h
result_queue.o: /usr/include/glib-2.0/glib/gtree.h
result_queue.o: /usr/include/glib-2.0/glib/gurifuncs.h
result_queue.o: /usr/include/glib-2.0/glib/gvarianttype.h
result_queue.o: /usr/include/glib-2.0/glib/gvariant.h
result_queue.o: /usr/include/glib-2.0/glib/gversion.h
result_queue.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
result_queue.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
result_queue.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
result_queue.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
result_queue.o: /usr/include/glib-2.0/glib/deprecated/grel.h
result_queue.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
result_queue.o: /usr/include/pthread.h /usr/include/endian.h
result_queue.o: /usr/include/sched.h result_queue.h types.h
set_ops.o: util.h /usr/include/glib-2.0/glib.h
set_ops.o: /usr/include/glib-2.0/glib/galloca.h
set_ops.o: /usr/include/glib-2.0/glib/gtypes.h
set_ops.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
set_ops.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
set_ops.o: /usr/include/features.h
set_ops.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
set_ops.o: /usr/include/xlocale.h /usr/include/glib-2.0/glib/garray.h
set_ops.o: /usr/include/glib-2.0/glib/gasyncqueue.h
set_ops.o: /usr/include/glib-2.0/glib/gthread.h
set_ops.o: /usr/include/glib-2.0/glib/gatomic.h
set_ops.o: /usr/include/glib-2.0/glib/gerror.h
set_ops.o: /usr/include/glib-2.0/glib/gquark.h
set_ops.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
set_ops.o: /usr/include/glib-2.0/glib/gbase64.h
set_ops.o: /usr/include/glib-2.0/glib/gbitlock.h
set_ops.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
set_ops.o: /usr/include/glib-2.0/glib/gbytes.h
set_ops.o: /usr/include/glib-2.0/glib/gcharset.h
set_ops.o: /usr/include/glib-2.0/glib/gchecksum.h
set_ops.o: /usr/include/glib-2.0/glib/gconvert.h
set_ops.o: /usr/include/glib-2.0/glib/gdataset.h
set_ops.o: /usr/include/glib-2.0/glib/gdate.h
set_ops.o: /usr/include/glib-2.0/glib/gdatetime.h
set_ops.o: /usr/include/glib-2.0/glib/gtimezone.h
set_ops.o: /usr/include/glib-2.0/glib/gdir.h
set_ops.o: /usr/include/glib-2.0/glib/genviron.h
set_ops.o: /usr/include/glib-2.0/glib/gfileutils.h
set_ops.o: /usr/include/glib-2.0/glib/ggettext.h
set_ops.o: /usr/include/glib-2.0/glib/ghash.h
set_ops.o: /usr/include/glib-2.0/glib/glist.h
set_ops.o: /usr/include/glib-2.0/glib/gmem.h
set_ops.o: /usr/include/glib-2.0/glib/ghmac.h
set_ops.o: /usr/include/glib-2.0/glib/gchecksum.h
set_ops.o: /usr/include/glib-2.0/glib/ghook.h
set_ops.o: /usr/include/glib-2.0/glib/ghostutils.h
set_ops.o: /usr/include/glib-2.0/glib/giochannel.h
set_ops.o: /usr/include/glib-2.0/glib/gmain.h
set_ops.o: /usr/include/glib-2.0/glib/gpoll.h
set_ops.o: /usr/include/glib-2.0/glib/gslist.h
set_ops.o: /usr/include/glib-2.0/glib/gstring.h
set_ops.o: /usr/include/glib-2.0/glib/gunicode.h
set_ops.o: /usr/include/glib-2.0/glib/gutils.h
set_ops.o: /usr/include/glib-2.0/glib/gkeyfile.h
set_ops.o: /usr/include/glib-2.0/glib/gmappedfile.h
set_ops.o: /usr/include/glib-2.0/glib/gmarkup.h
set_ops.o: /usr/include/glib-2.0/glib/gmessages.h
set_ops.o: /usr/include/glib-2.0/glib/gnode.h
set_ops.o: /usr/include/glib-2.0/glib/goption.h
set_ops.o: /usr/include/glib-2.0/glib/gpattern.h
set_ops.o: /usr/include/glib-2.0/glib/gprimes.h
set_ops.o: /usr/include/glib-2.0/glib/gqsort.h
set_ops.o: /usr/include/glib-2.0/glib/gqueue.h
set_ops.o: /usr/include/glib-2.0/glib/grand.h
set_ops.o: /usr/include/glib-2.0/glib/gregex.h
set_ops.o: /usr/include/glib-2.0/glib/gscanner.h
set_ops.o: /usr/include/glib-2.0/glib/gsequence.h
set_ops.o: /usr/include/glib-2.0/glib/gshell.h
set_ops.o: /usr/include/glib-2.0/glib/gslice.h
set_ops.o: /usr/include/glib-2.0/glib/gspawn.h
set_ops.o: /usr/include/glib-2.0/glib/gstrfuncs.h
set_ops.o: /usr/include/glib-2.0/glib/gstringchunk.h
set_ops.o: /usr/include/glib-2.0/glib/gtestutils.h
set_ops.o: /usr/include/glib-2.0/glib/gthreadpool.h
set_ops.o: /usr/include/glib-2.0/glib/gtimer.h
set_ops.o: /usr/include/glib-2.0/glib/gtrashstack.h
set_ops.o: /usr/include/glib-2.0/glib/gtree.h
set_ops.o: /usr/include/glib-2.0/glib/gurifuncs.h
set_ops.o: /usr/include/glib-2.0/glib/gvarianttype.h
set_ops.o: /usr/include/glib-2.0/glib/gvariant.h
set_ops.o: /usr/include/glib-2.0/glib/gversion.h
set_ops.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
set_ops.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
set_ops.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
set_ops.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
set_ops.o: /usr/include/glib-2.0/glib/deprecated/grel.h
set_ops.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
set_ops.o: /usr/include/pthread.h /usr/include/endian.h /usr/include/sched.h
set_ops.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
set_ops.o: /usr/include/wchar.h set_ops.h /usr/include/stdlib.h
set_ops.o: /usr/include/alloca.h
stream.o: /usr/include/stdlib.h /usr/include/features.h /usr/include/alloca.h
stream.o: /usr/include/string.h /usr/include/xlocale.h stream.h
stream.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
stream.o: /usr/include/wchar.h /usr/include/glib-2.0/glib.h
stream.o: /usr/include/glib-2.0/glib/galloca.h
stream.o: /usr/include/glib-2.0/glib/gtypes.h
stream.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
stream.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
stream.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
stream.o: /usr/include/glib-2.0/glib/garray.h
stream.o: /usr/include/glib-2.0/glib/gasyncqueue.h
stream.o: /usr/include/glib-2.0/glib/gthread.h
stream.o: /usr/include/glib-2.0/glib/gatomic.h
stream.o: /usr/include/glib-2.0/glib/gerror.h
stream.o: /usr/include/glib-2.0/glib/gquark.h
stream.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
stream.o: /usr/include/glib-2.0/glib/gbase64.h
stream.o: /usr/include/glib-2.0/glib/gbitlock.h
stream.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
stream.o: /usr/include/glib-2.0/glib/gbytes.h
stream.o: /usr/include/glib-2.0/glib/gcharset.h
stream.o: /usr/include/glib-2.0/glib/gchecksum.h
stream.o: /usr/include/glib-2.0/glib/gconvert.h
stream.o: /usr/include/glib-2.0/glib/gdataset.h
stream.o: /usr/include/glib-2.0/glib/gdate.h
stream.o: /usr/include/glib-2.0/glib/gdatetime.h
stream.o: /usr/include/glib-2.0/glib/gtimezone.h
stream.o: /usr/include/glib-2.0/glib/gdir.h
stream.o: /usr/include/glib-2.0/glib/genviron.h
stream.o: /usr/include/glib-2.0/glib/gfileutils.h
stream.o: /usr/include/glib-2.0/glib/ggettext.h
stream.o: /usr/include/glib-2.0/glib/ghash.h
stream.o: /usr/include/glib-2.0/glib/glist.h
stream.o: /usr/include/glib-2.0/glib/gmem.h
stream.o: /usr/include/glib-2.0/glib/ghmac.h
stream.o: /usr/include/glib-2.0/glib/gchecksum.h
stream.o: /usr/include/glib-2.0/glib/ghook.h
stream.o: /usr/include/glib-2.0/glib/ghostutils.h
stream.o: /usr/include/glib-2.0/glib/giochannel.h
stream.o: /usr/include/glib-2.0/glib/gmain.h
stream.o: /usr/include/glib-2.0/glib/gpoll.h
stream.o: /usr/include/glib-2.0/glib/gslist.h
stream.o: /usr/include/glib-2.0/glib/gstring.h
stream.o: /usr/include/glib-2.0/glib/gunicode.h
stream.o: /usr/include/glib-2.0/glib/gutils.h
stream.o: /usr/include/glib-2.0/glib/gkeyfile.h
stream.o: /usr/include/glib-2.0/glib/gmappedfile.h
stream.o: /usr/include/glib-2.0/glib/gmarkup.h
stream.o: /usr/include/glib-2.0/glib/gmessages.h
stream.o: /usr/include/glib-2.0/glib/gnode.h
stream.o: /usr/include/glib-2.0/glib/goption.h
stream.o: /usr/include/glib-2.0/glib/gpattern.h
stream.o: /usr/include/glib-2.0/glib/gprimes.h
stream.o: /usr/include/glib-2.0/glib/gqsort.h
stream.o: /usr/include/glib-2.0/glib/gqueue.h
stream.o: /usr/include/glib-2.0/glib/grand.h
stream.o: /usr/include/glib-2.0/glib/gregex.h
stream.o: /usr/include/glib-2.0/glib/gscanner.h
stream.o: /usr/include/glib-2.0/glib/gsequence.h
stream.o: /usr/include/glib-2.0/glib/gshell.h
stream.o: /usr/include/glib-2.0/glib/gslice.h
stream.o: /usr/include/glib-2.0/glib/gspawn.h
stream.o: /usr/include/glib-2.0/glib/gstrfuncs.h
stream.o: /usr/include/glib-2.0/glib/gstringchunk.h
stream.o: /usr/include/glib-2.0/glib/gtestutils.h
stream.o: /usr/include/glib-2.0/glib/gthreadpool.h
stream.o: /usr/include/glib-2.0/glib/gtimer.h
stream.o: /usr/include/glib-2.0/glib/gtrashstack.h
stream.o: /usr/include/glib-2.0/glib/gtree.h
stream.o: /usr/include/glib-2.0/glib/gurifuncs.h
stream.o: /usr/include/glib-2.0/glib/gvarianttype.h
stream.o: /usr/include/glib-2.0/glib/gvariant.h
stream.o: /usr/include/glib-2.0/glib/gversion.h
stream.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
stream.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
stream.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
stream.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
stream.o: /usr/include/glib-2.0/glib/deprecated/grel.h
stream.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
stream.o: /usr/include/pthread.h /usr/include/endian.h /usr/include/sched.h
tagdb.o: /usr/include/stdlib.h /usr/include/features.h /usr/include/alloca.h
tagdb.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
tagdb.o: /usr/include/wchar.h /usr/include/string.h /usr/include/xlocale.h
tagdb.o: /usr/include/unistd.h /usr/include/getopt.h tagdb.h
tagdb.o: /usr/include/glib-2.0/glib.h /usr/include/glib-2.0/glib/galloca.h
tagdb.o: /usr/include/glib-2.0/glib/gtypes.h
tagdb.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
tagdb.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
tagdb.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
tagdb.o: /usr/include/glib-2.0/glib/garray.h
tagdb.o: /usr/include/glib-2.0/glib/gasyncqueue.h
tagdb.o: /usr/include/glib-2.0/glib/gthread.h
tagdb.o: /usr/include/glib-2.0/glib/gatomic.h
tagdb.o: /usr/include/glib-2.0/glib/gerror.h
tagdb.o: /usr/include/glib-2.0/glib/gquark.h
tagdb.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
tagdb.o: /usr/include/glib-2.0/glib/gbase64.h
tagdb.o: /usr/include/glib-2.0/glib/gbitlock.h
tagdb.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
tagdb.o: /usr/include/glib-2.0/glib/gbytes.h
tagdb.o: /usr/include/glib-2.0/glib/gcharset.h
tagdb.o: /usr/include/glib-2.0/glib/gchecksum.h
tagdb.o: /usr/include/glib-2.0/glib/gconvert.h
tagdb.o: /usr/include/glib-2.0/glib/gdataset.h
tagdb.o: /usr/include/glib-2.0/glib/gdate.h
tagdb.o: /usr/include/glib-2.0/glib/gdatetime.h
tagdb.o: /usr/include/glib-2.0/glib/gtimezone.h
tagdb.o: /usr/include/glib-2.0/glib/gdir.h
tagdb.o: /usr/include/glib-2.0/glib/genviron.h
tagdb.o: /usr/include/glib-2.0/glib/gfileutils.h
tagdb.o: /usr/include/glib-2.0/glib/ggettext.h
tagdb.o: /usr/include/glib-2.0/glib/ghash.h
tagdb.o: /usr/include/glib-2.0/glib/glist.h /usr/include/glib-2.0/glib/gmem.h
tagdb.o: /usr/include/glib-2.0/glib/ghmac.h
tagdb.o: /usr/include/glib-2.0/glib/gchecksum.h
tagdb.o: /usr/include/glib-2.0/glib/ghook.h
tagdb.o: /usr/include/glib-2.0/glib/ghostutils.h
tagdb.o: /usr/include/glib-2.0/glib/giochannel.h
tagdb.o: /usr/include/glib-2.0/glib/gmain.h
tagdb.o: /usr/include/glib-2.0/glib/gpoll.h
tagdb.o: /usr/include/glib-2.0/glib/gslist.h
tagdb.o: /usr/include/glib-2.0/glib/gstring.h
tagdb.o: /usr/include/glib-2.0/glib/gunicode.h
tagdb.o: /usr/include/glib-2.0/glib/gutils.h
tagdb.o: /usr/include/glib-2.0/glib/gkeyfile.h
tagdb.o: /usr/include/glib-2.0/glib/gmappedfile.h
tagdb.o: /usr/include/glib-2.0/glib/gmarkup.h
tagdb.o: /usr/include/glib-2.0/glib/gmessages.h
tagdb.o: /usr/include/glib-2.0/glib/gnode.h
tagdb.o: /usr/include/glib-2.0/glib/goption.h
tagdb.o: /usr/include/glib-2.0/glib/gpattern.h
tagdb.o: /usr/include/glib-2.0/glib/gprimes.h
tagdb.o: /usr/include/glib-2.0/glib/gqsort.h
tagdb.o: /usr/include/glib-2.0/glib/gqueue.h
tagdb.o: /usr/include/glib-2.0/glib/grand.h
tagdb.o: /usr/include/glib-2.0/glib/gregex.h
tagdb.o: /usr/include/glib-2.0/glib/gscanner.h
tagdb.o: /usr/include/glib-2.0/glib/gsequence.h
tagdb.o: /usr/include/glib-2.0/glib/gshell.h
tagdb.o: /usr/include/glib-2.0/glib/gslice.h
tagdb.o: /usr/include/glib-2.0/glib/gspawn.h
tagdb.o: /usr/include/glib-2.0/glib/gstrfuncs.h
tagdb.o: /usr/include/glib-2.0/glib/gstringchunk.h
tagdb.o: /usr/include/glib-2.0/glib/gtestutils.h
tagdb.o: /usr/include/glib-2.0/glib/gthreadpool.h
tagdb.o: /usr/include/glib-2.0/glib/gtimer.h
tagdb.o: /usr/include/glib-2.0/glib/gtrashstack.h
tagdb.o: /usr/include/glib-2.0/glib/gtree.h
tagdb.o: /usr/include/glib-2.0/glib/gurifuncs.h
tagdb.o: /usr/include/glib-2.0/glib/gvarianttype.h
tagdb.o: /usr/include/glib-2.0/glib/gvariant.h
tagdb.o: /usr/include/glib-2.0/glib/gversion.h
tagdb.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
tagdb.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
tagdb.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
tagdb.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
tagdb.o: /usr/include/glib-2.0/glib/deprecated/grel.h
tagdb.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
tagdb.o: /usr/include/pthread.h /usr/include/endian.h /usr/include/sched.h
tagdb.o: code_table.h types.h tokenizer.h stream.h set_ops.h util.h query.h
tagdb_priv.o: /usr/include/stdio.h /usr/include/features.h
tagdb_priv.o: /usr/include/libio.h /usr/include/_G_config.h
tagdb_priv.o: /usr/include/wchar.h /usr/include/stdlib.h
tagdb_priv.o: /usr/include/alloca.h /usr/include/errno.h tagdb.h
tagdb_priv.o: /usr/include/glib-2.0/glib.h
tagdb_priv.o: /usr/include/glib-2.0/glib/galloca.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gtypes.h
tagdb_priv.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
tagdb_priv.o: /usr/include/xlocale.h /usr/include/glib-2.0/glib/garray.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gasyncqueue.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gthread.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gatomic.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gerror.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gquark.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gbase64.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gbitlock.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gbytes.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gcharset.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gchecksum.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gconvert.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gdataset.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gdate.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gdatetime.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gtimezone.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gdir.h
tagdb_priv.o: /usr/include/glib-2.0/glib/genviron.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gfileutils.h
tagdb_priv.o: /usr/include/glib-2.0/glib/ggettext.h
tagdb_priv.o: /usr/include/glib-2.0/glib/ghash.h
tagdb_priv.o: /usr/include/glib-2.0/glib/glist.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gmem.h
tagdb_priv.o: /usr/include/glib-2.0/glib/ghmac.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gchecksum.h
tagdb_priv.o: /usr/include/glib-2.0/glib/ghook.h
tagdb_priv.o: /usr/include/glib-2.0/glib/ghostutils.h
tagdb_priv.o: /usr/include/glib-2.0/glib/giochannel.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gmain.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gpoll.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gslist.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gstring.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gunicode.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gutils.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gkeyfile.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gmappedfile.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gmarkup.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gmessages.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gnode.h
tagdb_priv.o: /usr/include/glib-2.0/glib/goption.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gpattern.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gprimes.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gqsort.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gqueue.h
tagdb_priv.o: /usr/include/glib-2.0/glib/grand.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gregex.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gscanner.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gsequence.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gshell.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gslice.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gspawn.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gstrfuncs.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gstringchunk.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gtestutils.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gthreadpool.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gtimer.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gtrashstack.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gtree.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gurifuncs.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gvarianttype.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gvariant.h
tagdb_priv.o: /usr/include/glib-2.0/glib/gversion.h
tagdb_priv.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
tagdb_priv.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
tagdb_priv.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
tagdb_priv.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
tagdb_priv.o: /usr/include/glib-2.0/glib/deprecated/grel.h
tagdb_priv.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
tagdb_priv.o: /usr/include/pthread.h /usr/include/endian.h
tagdb_priv.o: /usr/include/sched.h code_table.h types.h set_ops.h tokenizer.h
tagdb_priv.o: stream.h util.h
tagfs.o: tagfs.h /usr/include/fuse/fuse.h /usr/include/fuse/fuse_common.h
tagfs.o: /usr/include/fuse/fuse_opt.h /usr/include/stdint.h
tagfs.o: /usr/include/features.h /usr/include/fuse/fuse_common_compat.h
tagfs.o: /usr/include/fcntl.h /usr/include/time.h /usr/include/xlocale.h
tagfs.o: /usr/include/utime.h /usr/include/fuse/fuse_compat.h
tagfs.o: /usr/include/ctype.h /usr/include/endian.h /usr/include/dirent.h
tagfs.o: /usr/include/errno.h /usr/include/libgen.h /usr/include/limits.h
tagfs.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/stdio.h
tagfs.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
tagfs.o: /usr/include/string.h /usr/include/unistd.h /usr/include/getopt.h
tagfs.o: util.h /usr/include/glib-2.0/glib.h
tagfs.o: /usr/include/glib-2.0/glib/galloca.h
tagfs.o: /usr/include/glib-2.0/glib/gtypes.h
tagfs.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
tagfs.o: /usr/include/glib-2.0/glib/gmacros.h
tagfs.o: /usr/include/glib-2.0/glib/gversionmacros.h
tagfs.o: /usr/include/glib-2.0/glib/garray.h
tagfs.o: /usr/include/glib-2.0/glib/gasyncqueue.h
tagfs.o: /usr/include/glib-2.0/glib/gthread.h
tagfs.o: /usr/include/glib-2.0/glib/gatomic.h
tagfs.o: /usr/include/glib-2.0/glib/gerror.h
tagfs.o: /usr/include/glib-2.0/glib/gquark.h
tagfs.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
tagfs.o: /usr/include/glib-2.0/glib/gbase64.h
tagfs.o: /usr/include/glib-2.0/glib/gbitlock.h
tagfs.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
tagfs.o: /usr/include/glib-2.0/glib/gbytes.h
tagfs.o: /usr/include/glib-2.0/glib/gcharset.h
tagfs.o: /usr/include/glib-2.0/glib/gchecksum.h
tagfs.o: /usr/include/glib-2.0/glib/gconvert.h
tagfs.o: /usr/include/glib-2.0/glib/gdataset.h
tagfs.o: /usr/include/glib-2.0/glib/gdate.h
tagfs.o: /usr/include/glib-2.0/glib/gdatetime.h
tagfs.o: /usr/include/glib-2.0/glib/gtimezone.h
tagfs.o: /usr/include/glib-2.0/glib/gdir.h
tagfs.o: /usr/include/glib-2.0/glib/genviron.h
tagfs.o: /usr/include/glib-2.0/glib/gfileutils.h
tagfs.o: /usr/include/glib-2.0/glib/ggettext.h
tagfs.o: /usr/include/glib-2.0/glib/ghash.h
tagfs.o: /usr/include/glib-2.0/glib/glist.h /usr/include/glib-2.0/glib/gmem.h
tagfs.o: /usr/include/glib-2.0/glib/ghmac.h
tagfs.o: /usr/include/glib-2.0/glib/gchecksum.h
tagfs.o: /usr/include/glib-2.0/glib/ghook.h
tagfs.o: /usr/include/glib-2.0/glib/ghostutils.h
tagfs.o: /usr/include/glib-2.0/glib/giochannel.h
tagfs.o: /usr/include/glib-2.0/glib/gmain.h
tagfs.o: /usr/include/glib-2.0/glib/gpoll.h
tagfs.o: /usr/include/glib-2.0/glib/gslist.h
tagfs.o: /usr/include/glib-2.0/glib/gstring.h
tagfs.o: /usr/include/glib-2.0/glib/gunicode.h
tagfs.o: /usr/include/glib-2.0/glib/gutils.h
tagfs.o: /usr/include/glib-2.0/glib/gkeyfile.h
tagfs.o: /usr/include/glib-2.0/glib/gmappedfile.h
tagfs.o: /usr/include/glib-2.0/glib/gmarkup.h
tagfs.o: /usr/include/glib-2.0/glib/gmessages.h
tagfs.o: /usr/include/glib-2.0/glib/gnode.h
tagfs.o: /usr/include/glib-2.0/glib/goption.h
tagfs.o: /usr/include/glib-2.0/glib/gpattern.h
tagfs.o: /usr/include/glib-2.0/glib/gprimes.h
tagfs.o: /usr/include/glib-2.0/glib/gqsort.h
tagfs.o: /usr/include/glib-2.0/glib/gqueue.h
tagfs.o: /usr/include/glib-2.0/glib/grand.h
tagfs.o: /usr/include/glib-2.0/glib/gregex.h
tagfs.o: /usr/include/glib-2.0/glib/gscanner.h
tagfs.o: /usr/include/glib-2.0/glib/gsequence.h
tagfs.o: /usr/include/glib-2.0/glib/gshell.h
tagfs.o: /usr/include/glib-2.0/glib/gslice.h
tagfs.o: /usr/include/glib-2.0/glib/gspawn.h
tagfs.o: /usr/include/glib-2.0/glib/gstrfuncs.h
tagfs.o: /usr/include/glib-2.0/glib/gstringchunk.h
tagfs.o: /usr/include/glib-2.0/glib/gtestutils.h
tagfs.o: /usr/include/glib-2.0/glib/gthreadpool.h
tagfs.o: /usr/include/glib-2.0/glib/gtimer.h
tagfs.o: /usr/include/glib-2.0/glib/gtrashstack.h
tagfs.o: /usr/include/glib-2.0/glib/gtree.h
tagfs.o: /usr/include/glib-2.0/glib/gurifuncs.h
tagfs.o: /usr/include/glib-2.0/glib/gvarianttype.h
tagfs.o: /usr/include/glib-2.0/glib/gvariant.h
tagfs.o: /usr/include/glib-2.0/glib/gversion.h
tagfs.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
tagfs.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
tagfs.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
tagfs.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
tagfs.o: /usr/include/glib-2.0/glib/deprecated/grel.h
tagfs.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
tagfs.o: /usr/include/pthread.h /usr/include/sched.h params.h tagdb.h
tagfs.o: code_table.h types.h result_queue.h log.h set_ops.h query.h
tokenizer.o: tokenizer.h /usr/include/stdlib.h /usr/include/features.h
tokenizer.o: /usr/include/alloca.h /usr/include/stdio.h /usr/include/libio.h
tokenizer.o: /usr/include/_G_config.h /usr/include/wchar.h
tokenizer.o: /usr/include/glib-2.0/glib.h
tokenizer.o: /usr/include/glib-2.0/glib/galloca.h
tokenizer.o: /usr/include/glib-2.0/glib/gtypes.h
tokenizer.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
tokenizer.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
tokenizer.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
tokenizer.o: /usr/include/xlocale.h /usr/include/glib-2.0/glib/garray.h
tokenizer.o: /usr/include/glib-2.0/glib/gasyncqueue.h
tokenizer.o: /usr/include/glib-2.0/glib/gthread.h
tokenizer.o: /usr/include/glib-2.0/glib/gatomic.h
tokenizer.o: /usr/include/glib-2.0/glib/gerror.h
tokenizer.o: /usr/include/glib-2.0/glib/gquark.h
tokenizer.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
tokenizer.o: /usr/include/glib-2.0/glib/gbase64.h
tokenizer.o: /usr/include/glib-2.0/glib/gbitlock.h
tokenizer.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
tokenizer.o: /usr/include/glib-2.0/glib/gbytes.h
tokenizer.o: /usr/include/glib-2.0/glib/gcharset.h
tokenizer.o: /usr/include/glib-2.0/glib/gchecksum.h
tokenizer.o: /usr/include/glib-2.0/glib/gconvert.h
tokenizer.o: /usr/include/glib-2.0/glib/gdataset.h
tokenizer.o: /usr/include/glib-2.0/glib/gdate.h
tokenizer.o: /usr/include/glib-2.0/glib/gdatetime.h
tokenizer.o: /usr/include/glib-2.0/glib/gtimezone.h
tokenizer.o: /usr/include/glib-2.0/glib/gdir.h
tokenizer.o: /usr/include/glib-2.0/glib/genviron.h
tokenizer.o: /usr/include/glib-2.0/glib/gfileutils.h
tokenizer.o: /usr/include/glib-2.0/glib/ggettext.h
tokenizer.o: /usr/include/glib-2.0/glib/ghash.h
tokenizer.o: /usr/include/glib-2.0/glib/glist.h
tokenizer.o: /usr/include/glib-2.0/glib/gmem.h
tokenizer.o: /usr/include/glib-2.0/glib/ghmac.h
tokenizer.o: /usr/include/glib-2.0/glib/gchecksum.h
tokenizer.o: /usr/include/glib-2.0/glib/ghook.h
tokenizer.o: /usr/include/glib-2.0/glib/ghostutils.h
tokenizer.o: /usr/include/glib-2.0/glib/giochannel.h
tokenizer.o: /usr/include/glib-2.0/glib/gmain.h
tokenizer.o: /usr/include/glib-2.0/glib/gpoll.h
tokenizer.o: /usr/include/glib-2.0/glib/gslist.h
tokenizer.o: /usr/include/glib-2.0/glib/gstring.h
tokenizer.o: /usr/include/glib-2.0/glib/gunicode.h
tokenizer.o: /usr/include/glib-2.0/glib/gutils.h
tokenizer.o: /usr/include/glib-2.0/glib/gkeyfile.h
tokenizer.o: /usr/include/glib-2.0/glib/gmappedfile.h
tokenizer.o: /usr/include/glib-2.0/glib/gmarkup.h
tokenizer.o: /usr/include/glib-2.0/glib/gmessages.h
tokenizer.o: /usr/include/glib-2.0/glib/gnode.h
tokenizer.o: /usr/include/glib-2.0/glib/goption.h
tokenizer.o: /usr/include/glib-2.0/glib/gpattern.h
tokenizer.o: /usr/include/glib-2.0/glib/gprimes.h
tokenizer.o: /usr/include/glib-2.0/glib/gqsort.h
tokenizer.o: /usr/include/glib-2.0/glib/gqueue.h
tokenizer.o: /usr/include/glib-2.0/glib/grand.h
tokenizer.o: /usr/include/glib-2.0/glib/gregex.h
tokenizer.o: /usr/include/glib-2.0/glib/gscanner.h
tokenizer.o: /usr/include/glib-2.0/glib/gsequence.h
tokenizer.o: /usr/include/glib-2.0/glib/gshell.h
tokenizer.o: /usr/include/glib-2.0/glib/gslice.h
tokenizer.o: /usr/include/glib-2.0/glib/gspawn.h
tokenizer.o: /usr/include/glib-2.0/glib/gstrfuncs.h
tokenizer.o: /usr/include/glib-2.0/glib/gstringchunk.h
tokenizer.o: /usr/include/glib-2.0/glib/gtestutils.h
tokenizer.o: /usr/include/glib-2.0/glib/gthreadpool.h
tokenizer.o: /usr/include/glib-2.0/glib/gtimer.h
tokenizer.o: /usr/include/glib-2.0/glib/gtrashstack.h
tokenizer.o: /usr/include/glib-2.0/glib/gtree.h
tokenizer.o: /usr/include/glib-2.0/glib/gurifuncs.h
tokenizer.o: /usr/include/glib-2.0/glib/gvarianttype.h
tokenizer.o: /usr/include/glib-2.0/glib/gvariant.h
tokenizer.o: /usr/include/glib-2.0/glib/gversion.h
tokenizer.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
tokenizer.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
tokenizer.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
tokenizer.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
tokenizer.o: /usr/include/glib-2.0/glib/deprecated/grel.h
tokenizer.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
tokenizer.o: /usr/include/pthread.h /usr/include/endian.h
tokenizer.o: /usr/include/sched.h stream.h
types.o: /usr/include/stdlib.h /usr/include/features.h /usr/include/alloca.h
types.o: types.h /usr/include/glib-2.0/glib.h
types.o: /usr/include/glib-2.0/glib/galloca.h
types.o: /usr/include/glib-2.0/glib/gtypes.h
types.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
types.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
types.o: /usr/include/glib-2.0/glib/gversionmacros.h /usr/include/time.h
types.o: /usr/include/xlocale.h /usr/include/glib-2.0/glib/garray.h
types.o: /usr/include/glib-2.0/glib/gasyncqueue.h
types.o: /usr/include/glib-2.0/glib/gthread.h
types.o: /usr/include/glib-2.0/glib/gatomic.h
types.o: /usr/include/glib-2.0/glib/gerror.h
types.o: /usr/include/glib-2.0/glib/gquark.h
types.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
types.o: /usr/include/glib-2.0/glib/gbase64.h
types.o: /usr/include/glib-2.0/glib/gbitlock.h
types.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
types.o: /usr/include/glib-2.0/glib/gbytes.h
types.o: /usr/include/glib-2.0/glib/gcharset.h
types.o: /usr/include/glib-2.0/glib/gchecksum.h
types.o: /usr/include/glib-2.0/glib/gconvert.h
types.o: /usr/include/glib-2.0/glib/gdataset.h
types.o: /usr/include/glib-2.0/glib/gdate.h
types.o: /usr/include/glib-2.0/glib/gdatetime.h
types.o: /usr/include/glib-2.0/glib/gtimezone.h
types.o: /usr/include/glib-2.0/glib/gdir.h
types.o: /usr/include/glib-2.0/glib/genviron.h
types.o: /usr/include/glib-2.0/glib/gfileutils.h
types.o: /usr/include/glib-2.0/glib/ggettext.h
types.o: /usr/include/glib-2.0/glib/ghash.h
types.o: /usr/include/glib-2.0/glib/glist.h /usr/include/glib-2.0/glib/gmem.h
types.o: /usr/include/glib-2.0/glib/ghmac.h
types.o: /usr/include/glib-2.0/glib/gchecksum.h
types.o: /usr/include/glib-2.0/glib/ghook.h
types.o: /usr/include/glib-2.0/glib/ghostutils.h
types.o: /usr/include/glib-2.0/glib/giochannel.h
types.o: /usr/include/glib-2.0/glib/gmain.h
types.o: /usr/include/glib-2.0/glib/gpoll.h
types.o: /usr/include/glib-2.0/glib/gslist.h
types.o: /usr/include/glib-2.0/glib/gstring.h
types.o: /usr/include/glib-2.0/glib/gunicode.h
types.o: /usr/include/glib-2.0/glib/gutils.h
types.o: /usr/include/glib-2.0/glib/gkeyfile.h
types.o: /usr/include/glib-2.0/glib/gmappedfile.h
types.o: /usr/include/glib-2.0/glib/gmarkup.h
types.o: /usr/include/glib-2.0/glib/gmessages.h
types.o: /usr/include/glib-2.0/glib/gnode.h
types.o: /usr/include/glib-2.0/glib/goption.h
types.o: /usr/include/glib-2.0/glib/gpattern.h
types.o: /usr/include/glib-2.0/glib/gprimes.h
types.o: /usr/include/glib-2.0/glib/gqsort.h
types.o: /usr/include/glib-2.0/glib/gqueue.h
types.o: /usr/include/glib-2.0/glib/grand.h
types.o: /usr/include/glib-2.0/glib/gregex.h
types.o: /usr/include/glib-2.0/glib/gscanner.h
types.o: /usr/include/glib-2.0/glib/gsequence.h
types.o: /usr/include/glib-2.0/glib/gshell.h
types.o: /usr/include/glib-2.0/glib/gslice.h
types.o: /usr/include/glib-2.0/glib/gspawn.h
types.o: /usr/include/glib-2.0/glib/gstrfuncs.h
types.o: /usr/include/glib-2.0/glib/gstringchunk.h
types.o: /usr/include/glib-2.0/glib/gtestutils.h
types.o: /usr/include/glib-2.0/glib/gthreadpool.h
types.o: /usr/include/glib-2.0/glib/gtimer.h
types.o: /usr/include/glib-2.0/glib/gtrashstack.h
types.o: /usr/include/glib-2.0/glib/gtree.h
types.o: /usr/include/glib-2.0/glib/gurifuncs.h
types.o: /usr/include/glib-2.0/glib/gvarianttype.h
types.o: /usr/include/glib-2.0/glib/gvariant.h
types.o: /usr/include/glib-2.0/glib/gversion.h
types.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
types.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
types.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
types.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
types.o: /usr/include/glib-2.0/glib/deprecated/grel.h
types.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
types.o: /usr/include/pthread.h /usr/include/endian.h /usr/include/sched.h
util.o: util.h /usr/include/glib-2.0/glib.h
util.o: /usr/include/glib-2.0/glib/galloca.h
util.o: /usr/include/glib-2.0/glib/gtypes.h
util.o: /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h
util.o: /usr/include/glib-2.0/glib/gmacros.h /usr/include/limits.h
util.o: /usr/include/features.h /usr/include/glib-2.0/glib/gversionmacros.h
util.o: /usr/include/time.h /usr/include/xlocale.h
util.o: /usr/include/glib-2.0/glib/garray.h
util.o: /usr/include/glib-2.0/glib/gasyncqueue.h
util.o: /usr/include/glib-2.0/glib/gthread.h
util.o: /usr/include/glib-2.0/glib/gatomic.h
util.o: /usr/include/glib-2.0/glib/gerror.h
util.o: /usr/include/glib-2.0/glib/gquark.h
util.o: /usr/include/glib-2.0/glib/gbacktrace.h /usr/include/signal.h
util.o: /usr/include/glib-2.0/glib/gbase64.h
util.o: /usr/include/glib-2.0/glib/gbitlock.h
util.o: /usr/include/glib-2.0/glib/gbookmarkfile.h
util.o: /usr/include/glib-2.0/glib/gbytes.h
util.o: /usr/include/glib-2.0/glib/gcharset.h
util.o: /usr/include/glib-2.0/glib/gchecksum.h
util.o: /usr/include/glib-2.0/glib/gconvert.h
util.o: /usr/include/glib-2.0/glib/gdataset.h
util.o: /usr/include/glib-2.0/glib/gdate.h
util.o: /usr/include/glib-2.0/glib/gdatetime.h
util.o: /usr/include/glib-2.0/glib/gtimezone.h
util.o: /usr/include/glib-2.0/glib/gdir.h
util.o: /usr/include/glib-2.0/glib/genviron.h
util.o: /usr/include/glib-2.0/glib/gfileutils.h
util.o: /usr/include/glib-2.0/glib/ggettext.h
util.o: /usr/include/glib-2.0/glib/ghash.h /usr/include/glib-2.0/glib/glist.h
util.o: /usr/include/glib-2.0/glib/gmem.h /usr/include/glib-2.0/glib/ghmac.h
util.o: /usr/include/glib-2.0/glib/gchecksum.h
util.o: /usr/include/glib-2.0/glib/ghook.h
util.o: /usr/include/glib-2.0/glib/ghostutils.h
util.o: /usr/include/glib-2.0/glib/giochannel.h
util.o: /usr/include/glib-2.0/glib/gmain.h /usr/include/glib-2.0/glib/gpoll.h
util.o: /usr/include/glib-2.0/glib/gslist.h
util.o: /usr/include/glib-2.0/glib/gstring.h
util.o: /usr/include/glib-2.0/glib/gunicode.h
util.o: /usr/include/glib-2.0/glib/gutils.h
util.o: /usr/include/glib-2.0/glib/gkeyfile.h
util.o: /usr/include/glib-2.0/glib/gmappedfile.h
util.o: /usr/include/glib-2.0/glib/gmarkup.h
util.o: /usr/include/glib-2.0/glib/gmessages.h
util.o: /usr/include/glib-2.0/glib/gnode.h
util.o: /usr/include/glib-2.0/glib/goption.h
util.o: /usr/include/glib-2.0/glib/gpattern.h
util.o: /usr/include/glib-2.0/glib/gprimes.h
util.o: /usr/include/glib-2.0/glib/gqsort.h
util.o: /usr/include/glib-2.0/glib/gqueue.h
util.o: /usr/include/glib-2.0/glib/grand.h
util.o: /usr/include/glib-2.0/glib/gregex.h
util.o: /usr/include/glib-2.0/glib/gscanner.h
util.o: /usr/include/glib-2.0/glib/gsequence.h
util.o: /usr/include/glib-2.0/glib/gshell.h
util.o: /usr/include/glib-2.0/glib/gslice.h
util.o: /usr/include/glib-2.0/glib/gspawn.h
util.o: /usr/include/glib-2.0/glib/gstrfuncs.h
util.o: /usr/include/glib-2.0/glib/gstringchunk.h
util.o: /usr/include/glib-2.0/glib/gtestutils.h
util.o: /usr/include/glib-2.0/glib/gthreadpool.h
util.o: /usr/include/glib-2.0/glib/gtimer.h
util.o: /usr/include/glib-2.0/glib/gtrashstack.h
util.o: /usr/include/glib-2.0/glib/gtree.h
util.o: /usr/include/glib-2.0/glib/gurifuncs.h
util.o: /usr/include/glib-2.0/glib/gvarianttype.h
util.o: /usr/include/glib-2.0/glib/gvariant.h
util.o: /usr/include/glib-2.0/glib/gversion.h
util.o: /usr/include/glib-2.0/glib/deprecated/gallocator.h
util.o: /usr/include/glib-2.0/glib/deprecated/gcache.h
util.o: /usr/include/glib-2.0/glib/deprecated/gcompletion.h
util.o: /usr/include/glib-2.0/glib/deprecated/gmain.h
util.o: /usr/include/glib-2.0/glib/deprecated/grel.h
util.o: /usr/include/glib-2.0/glib/deprecated/gthread.h
util.o: /usr/include/pthread.h /usr/include/endian.h /usr/include/sched.h
util.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
util.o: /usr/include/wchar.h /usr/include/string.h
