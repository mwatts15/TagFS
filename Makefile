include common.mk 

PROJECT_ROOT=.
BUILD=build


.PHONY: tags

tagfs:
	make -C src/ tagfs

tags:
	ctags --langmap=c:.lc.c.h src/*.c src/*.lc include/*.h 

-include tagfs.tar.bz2.d
-include tagfs.orig.tar.bz2.d

tagfs.tar.bz2.d: build_source_tarball.sh distfiles
	@./build_source_tarball.sh -l > tagfs.tar.bz2.d

tagfs.orig.tar.bz2.d: build_source_tarball.sh distfiles
	@./build_source_tarball.sh -d -l > tagfs.orig.tar.bz2.d 

$(BUILD)/tagfs.tar.bz2: $(BUILD) build_source_tarball.sh distfiles version
	./build_source_tarball.sh

$(BUILD)/tagfs.debian.tar.bz2 $(BUILD)/tagfs.orig.tar.bz2: $(BUILD) build_source_tarball.sh distfiles version
	BUILDDIR=$(BUILD) ./build_source_tarball.sh -d
	cp $(BUILD)/tagfs_*.orig.tar.bz2 $(BUILD)/tagfs.orig.tar.bz2
	cp $(BUILD)/tagfs_*.debian.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2

version: .git
	git describe --abbrev=0 --match="version_*" 2>/dev/null > version

distfiles: .git
	git ls-files . > $@
	echo "src/version.h" >> $@
	echo tagfs.tar.bz2.d >> $@
	echo tagfs.orig.tar.bz2.d >> $@

src/version.h:
	make -C src/ version.h

distclean:
	make -C src/ clean

clean: distclean
	$(RM) -r $(BUILD)
	$(RM) -r *.d

$(BUILD)/tagfs.deb: $(BUILD)/tagfs.orig.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2 make_deb.sh 
	./make_deb.sh $(BUILD)/tagfs.orig.tar.bz2 $(BUILD)/tagfs.debian.tar.bz2

%::
	make -C src/ $@
