
.PHONY: all clean

all:
	@(cd plan && $(MAKE) $@)
	@(cd docs && $(MAKE) $@)
	@(cd projet && $(MAKE) $@)
	@(cd projet && sudo $(MAKE) test)


clean:
	@(cd plan && $(MAKE) $@)
	@(cd docs && $(MAKE) $@)
	@(cd projet && $(MAKE) $@)

realclean:
	@(cd plan && $(MAKE) clean)
	@(cd docs && $(MAKE) clean)
	@(cd projet && sudo $(MAKE) $@)
