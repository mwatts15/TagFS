# define the executable file
export PERLLIB=$(shell readlink -f ./lib/perl)

tagfs:
	make -C src/ tagfs
%:
	make -C src/ $@
