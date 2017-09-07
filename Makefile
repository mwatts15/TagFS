PROJECT_ROOT=.
BUILD=build
include common.mk 

.PHONY: tags cscope.out

.DEFAULT_GOAL := tagfs 

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

include tagfs.tar.bz2.d
include tagfs.orig.tar.bz2.d

tagfs.tar.bz2.d: build_source_tarball.sh distfiles
	./build_source_tarball.sh -l > tagfs.tar.bz2.d

tagfs.orig.tar.bz2.d: build_source_tarball.sh distfiles
	./build_source_tarball.sh -d -l > tagfs.orig.tar.bz2.d 

$(BUILD)/tagfs.tar.bz2: $(BUILD) build_source_tarball.sh distfiles version
	./build_source_tarball.sh

$(BUILD)/tagfs.debian.tar.bz2 $(BUILD)/tagfs.orig.tar.bz2: $(BUILD) build_source_tarball.sh distfiles version
	BUILDDIR=$(BUILD) ./build_source_tarball.sh -d
	cp $(BUILD)/tagfs_*.orig.tar.bz2 $(BUILD)/tagfs.orig.tar.bz2
	cp $(BUILD)/tagfs_*.debian.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2

version:
	git describe --abbrev=0 --match="version_*" > version

distfiles:
	git ls-files . > $@
	echo "src/version.h" >> $@
	echo tagfs.tar.bz2.d >> $@
	echo tagfs.orig.tar.bz2.d >> $@
	echo version >> $@
	echo distfiles >> $@

src/version.h:
	make -C src/ version.h

distclean:
	make -C src/ clean

clean: distclean
	$(RM) -r $(BUILD)

$(BUILD)/tagfs.deb: $(BUILD)/tagfs.orig.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2 make_deb.sh 
	./make_deb.sh $(BUILD)/tagfs.orig.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2

cscope.out:
	ls -1 src/*.c src/*.lc include/*.h | cscope -b

%::
	make -C src/ $@
