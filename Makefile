.PHONY: clean tar

CFLAGS = -Wall -Wextra -Werror
tarname = tgen.tar.gz

tgen:

tar:
	git archive --prefix=tgen/ -o $(tarname) master

clean:
	-rm tgen
