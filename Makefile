
.PHONY: all clean

all:
	@(cd plan && $(MAKE) $@)
	@(cd docs && $(MAKE) $@)

clean:
	@(cd plan && $(MAKE) $@)
	@(cd docs && $(MAKE) $@)
