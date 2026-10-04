# make format        reformat every header in place with ../etc/style.yaml
# make check-format  list the headers which are not formatted; changes nothing
#
# A header is replaced only if clang-format succeeded, wrote something, and
# changed nothing but whitespace and comments (the code tokens are compared).
# Formatting is run twice, since a rewrapped comment can need a second pass.

STYLE = ../etc/style.yaml
HEADERS = $(filter-out __temp.h, $(wildcard *.h))
# the code of a file without comments and whitespace
TOKENS = gcc -fpreprocessed -dD -E -P -x c

.PHONY: all format check-format clean

all: check-format

format:
	@for pass in 1 2; do \
	  for f in $(HEADERS); do \
	    clang-format --style=file:$(STYLE) $$f > __temp.h \
	      || { echo "$$f: clang-format failed, not changed"; rm -f __temp.h; exit 1; }; \
	    [ -s __temp.h ] \
	      || { echo "$$f: empty output, not changed"; rm -f __temp.h; exit 1; }; \
	    [ "`$(TOKENS) $$f | tr -d ' \t\n\\\\'`" = "`$(TOKENS) __temp.h | tr -d ' \t\n\\\\'`" ] \
	      || { echo "$$f: formatting would change the code, not changed"; rm -f __temp.h; exit 1; }; \
	    cmp -s $$f __temp.h || { mv __temp.h $$f; echo "formatted $$f"; }; \
	    rm -f __temp.h; \
	  done; \
	done

check-format:
	@clang-format --style=file:$(STYLE) --dry-run -Werror $(HEADERS) \
	  && echo "all headers are formatted"

clean:
	rm -f __temp.h *~
