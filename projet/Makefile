.PHONY: all test clean realclean

all:
	@(cd module && $(MAKE))
	if [ ! -d bin ]; then \
		mkdir bin; \
	fi; 
	cp module/*.ko bin/

test:
	@(cd script && $(MAKE) -s)

clean: 
	@(cd module && $(MAKE) clean)

realclean:
	@(cd module && $(MAKE) realclean)
	@(cd script && $(MAKE) realclean)
	rm -Rf bin/
