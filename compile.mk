# Common targets for c compilation

%.c : %.lc $(MARCO)
	$(MARCO) $<
