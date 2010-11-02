.PHONY: all test realclean

all: test

test:
	if [ `whoami` = root ]; then\
		./master_test;\
	else\
		echo "You must run test as root";\
	fi

realclean:
	rm -Rf ../log/
