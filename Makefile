export PERLLIB=$(shell readlink -f ./lib/perl)
export PERL5LIB=$(PERLLIB)

.PHONY: tags

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

dist:
	git ls-files --full-name . | tar cjf tagfs.tar.bz2 -T -

%::
	make -C src/ $@
