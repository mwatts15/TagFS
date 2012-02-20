tagfs: tagfs.c tagdb.c util.c cmd.c
	gcc -o tagfs `pkg-config --libs --cflags glib-2.0 fuse` tagfs.c tagdb.c util.c cmd.c
ttfs: test_tagfs.c tagdb.c util.c
	gcc -g -o ttfs `pkg-config --libs --cflags glib-2.0` test_tagfs.c tagdb.c util.c
ttdb: test_tagdb.c tagdb.c util.c
	gcc -g -o ttdb `pkg-config --libs --cflags glib-2.0` test_tagdb.c tagdb.c
