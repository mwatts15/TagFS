# Common targets and variables shared by all makefiles
export PERLLIB=$(shell readlink -f $(PROJECT_ROOT)/lib/perl)
export PERL5LIB=$(PERLLIB)

$(BUILD):
	mkdir -p "$(BUILD)"
