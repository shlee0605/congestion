DEBUG_MODE = no

# Debug malloc support (http://dmalloc.com).  Comment out if you don't
# have dmalloc, but it is highly recommended.
#
#DMALLOC_CFLAGS = -I/usr/local/include -DDMALLOC=1
#DMALLOC_LIBS = -L/usr/local/lib -ldmalloc
#
# On Stanford machines, you need these paths for dmalloc:
#
#DMALLOC_CFLAGS = -I/afs/ir/class/cs144/dmalloc -DDMALLOC=1
#DMALLOC_LIBS = -L/afs/ir/class/cs144/dmalloc -ldmalloc

ifeq ($(OS),Windows_NT)
	LIBRT =
else
	UNAME := $(shell uname)
	ifeq ($(UNAME),Linux)
		LIBRT = -lrt
	else
		LIBRT =
	endif
endif

CC = gcc
CFLAGS_RELEASE = -g -Wall $(DMALLOC_CFLAGS) -DNDEBUG
CFLAGS_DEBUG = -g -Wall $(DMALLOC_CFLAGS)
ifeq ($(DEBUG_MODE), yes)
	CFLAGS = $(CFLAGS_DEBUG)
else
	CFLAGS = $(CFLAGS_RELEASE)
endif
LIBS = $(DMALLOC_LIBS)

all: reliable

.c.o:
	$(CC) $(CFLAGS) -c $<

rlib.o reliable.o: rlib.h sliding_window.h

reliable: reliable.o rlib.o sliding_window.o
	$(CC) $(CFLAGS) -std=c99 -o $@ reliable.o rlib.o sliding_window.o $(LIBS) $(LIBRT)

.PHONY: tester reference
tester reference:
	cd tester-src && $(MAKE) Examples/reliable/$@
	cp tester-src/Examples/reliable/$@ .
	strip $@

TAR = reliable.tar.gz

SUBMIT = reliable/Makefile reliable/*.[ch]

.PHONY: submit
submit: clean
	ln -s . reliable
	tar -czf $(TAR) $(SUBMIT)
	rm -f reliable
	@echo '************************************************************'
	@echo '                                                            '
	@echo '  Please submit file $(TAR) to the following URL:  '
	@echo '                                                            '
	@echo '   https://www.stanford.edu/class/cs144/cgi-bin/submit/     '
	@echo '                                                            '
	@echo '************************************************************'

.PHONY: dist
dist: clean tester reference
	cd tester-src && $(MAKE) clean
	./stripsol reliable.c > reliable.c-dist
	ln -s . reliable
	tar -czf $(TAR) \
		reliable/reliable.c-dist \
		reliable/Makefile reliable/uc.c reliable/rlib.[ch] \
		reliable/stripsol \
		reliable/tester reliable/reference
	rm -f reliable

.PHONY: clean
clean:
	@find . \( -name '*~' -o -name '*.o' -o -name '*.hi' \) \
		-print0 > .clean~
	@xargs -0 echo rm -f -- < .clean~
	@xargs -0 rm -f -- < .clean~
	rm -f uc reliable $(TAR)

.PHONY: clobber
clobber: clean
	cd tester-src && $(MAKE) clean
	rm -f tester reference reliable.c-dist
