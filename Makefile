export PERLLIB=$(shell readlink -f ./lib/perl)
export PERL5LIB=$(PERLLIB)
PROJECT_ROOT=.
BUILD=build

include common.mk 

.PHONY: tags

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

-include $(BUILD)/tagfs.tar.bz2.d
-include $(BUILD)/tagfs.orig.tar.bz2.d

$(BUILD)/tagfs.tar.bz2.d: $(BUILD) build_source_tarball.sh
	@./build_source_tarball.sh -l > $(BUILD)/tagfs.tar.bz2.d

$(BUILD)/tagfs.orig.tar.bz2.d: $(BUILD) build_source_tarball.sh
	@./build_source_tarball.sh -d -l > $(BUILD)/tagfs.orig.tar.bz2.d 

$(BUILD)/tagfs.tar.bz2: $(BUILD) build_source_tarball.sh
	./build_source_tarball.sh

$(BUILD)/tagfs.debian.tar.bz2 $(BUILD)/tagfs.orig.tar.bz2: $(BUILD) build_source_tarball.sh
	BUILDDIR=$(BUILD) ./build_source_tarball.sh -d
	cp $(BUILD)/tagfs_*.orig.tar.bz2 $(BUILD)/tagfs.orig.tar.bz2
	cp $(BUILD)/tagfs_*.debian.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2

distclean:
	make -C src/ clean

clean: distclean
	$(RM) -r $(BUILD)

$(BUILD)/tagfs.deb: $(BUILD)/tagfs.orig.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2 make_deb.sh 
	./make_deb.sh $(BUILD)/tagfs.orig.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2

%::
	make -C src/ $@
