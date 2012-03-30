tagfs: tagfs.c tagdb.c util.c code_table.c tokenizer.c stream.c set_ops.c
	gcc -o tagfs `pkg-config --libs --cflags glib-2.0 fuse` tagfs.c tagdb.c util.c code_table.c tokenizer.c stream.c set_ops.c
ttfs: test_tagfs.c tagdb.c util.c tokenizer.c code_table.c
	gcc -g -o ttfs `pkg-config --libs --cflags glib-2.0` test_tagfs.c tagdb.c util.c tokenizer.c code_table.c
ttdb: test_tagdb.c tagdb.c code_table.c util.c tokenizer.c set_ops.c stream.c
	gcc -g -o ttdb `pkg-config --libs --cflags glib-2.0` test_tagdb.c tagdb.c code_table.c util.c tokenizer.c set_ops.c stream.c
tcmd: test_cmd.c tagdb.c util.c
	gcc -g -o tcmd `pkg-config --libs --cflags glib-2.0` test_cmd.c tagdb.c util.c
tct: test_code_table.c util.c code_table.c
	gcc -g -o tct `pkg-config --libs --cflags glib-2.0` test_code_table.c util.c code_table.c
ths: util.c test_hash_sets.c set_ops.c
	gcc -g -o ths `pkg-config --libs --cflags glib-2.0` test_hash_sets.c util.c set_ops.c
ttk: test_tokenizer.c tokenizer.c stream.c
	gcc -g -o ttk `pkg-config --libs --cflags glib-2.0` test_tokenizer.c tokenizer.c stream.c
tq: query.c test_query.c tagdb.c tokenizer.c stream.c util.c code_table.c tagdb_priv.c set_ops.c
	gcc -o tq -g `pkg-config --libs --cflags glib-2.0` test_query.c query.c tagdb.c tokenizer.c stream.c util.c code_table.c tagdb_priv.c set_ops.c
testdb:
	./generate_testdb.pl test.db 10 50 10 copies
clean:
	rm *.o
