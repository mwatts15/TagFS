tagfs: tagfs.c tagdb.c util.c cmd.c
	gcc -o tagfs `pkg-config --libs --cflags glib-2.0 fuse` tagfs.c tagdb.c util.c cmd.c
ttfs: test_tagfs.c tagdb.c util.c cmd.c
	gcc -g -o ttfs `pkg-config --libs --cflags glib-2.0` test_tagfs.c tagdb.c util.c cmd.c
ttdb: test_tagdb.c tagdb.c
	gcc -g -o ttdb `pkg-config --libs --cflags glib-2.0` test_tagdb.c tagdb.c
tcmd: test_cmd.c tagdb.c util.c
	gcc -g -o tcmd `pkg-config --libs --cflags glib-2.0` test_cmd.c tagdb.c util.c
testdb:
	./generate_testdb.pl test.db 10 50 10
