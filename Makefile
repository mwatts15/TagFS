export PERLLIB=$(shell readlink -f ./lib/perl)
export PERL5LIB=$(PERLLIB)

.PHONY: tags

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

include tagfs.tar.bz2.d
tagfs.tar.bz2.d: build_source_tarball.sh
	@./build_source_tarball.sh -l > tagfs.tar.bz2.d

tagfs.tar.bz2:
	./build_source_tarball.sh

clean:
	$(RM) tagfs.tar.bz2.d
	make -C src/ clean

tagfs.deb: tagfs.tar.bz2
	./make_deb.sh $<


%::
	make -C src/ $@
