export PERLLIB=$(shell readlink -f ./lib/perl)
export PERL5LIB=$(PERLLIB)
BUILD=build

.PHONY: tags

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

-include tagfs.tar.bz2.d

$(BUILD)/tagfs.tar.bz2.d: build_source_tarball.sh
	@./build_source_tarball.sh -l > $(BUILD)/tagfs.tar.bz2.d

$(BUILD)/tagfs.tar.bz2: build_source_tarball.sh
	./build_source_tarball.sh

clean:
	$(RM) -r build
	make -C src/ clean

$(BUILD)/tagfs.deb: $(BUILD)/tagfs.tar.bz2 make_deb.sh 
	./make_deb.sh $<

%::
	make -C src/ $@
