.PHONY: clean tar

CFLAGS = -Wall -Wextra -Werror
LIBS = -lm
tarname = tgen.tar.gz

tgen:

tar:
	git archive --prefix=tgen/ -o $(tarname) master

clean:
	-rm tgen
