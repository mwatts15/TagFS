export PERLLIB=$(shell readlink -f ./lib/perl)
export PERL5LIB=$(PERLLIB)

.PHONY: tags cscope.out

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

cscope.out:
	ls -1 src/*.c src/*.lc include/*.h | cscope -b

tagfs.tar.bz2::
	git ls-files --full-name . | tar cjf tagfs.tar.bz2 -T -

%::
	make -C src/ $@
