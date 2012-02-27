tagfs: tagfs.c tagdb.c util.c cmd.c code_table.c tokenizer.c
	gcc -o tagfs `pkg-config --libs --cflags glib-2.0 fuse` tagfs.c tagdb.c util.c cmd.c code_table.c tokenizer.c
ttfs: test_tagfs.c tagdb.c util.c cmd.c
	gcc -g -o ttfs `pkg-config --libs --cflags glib-2.0` test_tagfs.c tagdb.c util.c cmd.c
ttdb: test_tagdb.c tagdb.c code_table.c util.c tokenizer.c
	gcc -g -o ttdb `pkg-config --libs --cflags glib-2.0` test_tagdb.c tagdb.c code_table.c util.c tokenizer.c
tcmd: test_cmd.c tagdb.c util.c
	gcc -g -o tcmd `pkg-config --libs --cflags glib-2.0` test_cmd.c tagdb.c util.c
tct: test_code_table.c util.c code_table.c
	gcc -g -o tct `pkg-config --libs --cflags glib-2.0` test_code_table.c util.c code_table.c
ttk: test_tokenizer.c tokenizer.c
	gcc -g -o ttk `pkg-config --libs --cflags glib-2.0` test_tokenizer.c tokenizer.c
testdb:
	./generate_testdb.pl test.db 10 50 10
