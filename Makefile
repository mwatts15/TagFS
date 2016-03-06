# define the executable file
export PERLLIB=$(shell readlink -f ./lib/perl)
export PERL5LIB=$(PERLLIB)

.PHONY: tags

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

%::
	make -C src/ $@
