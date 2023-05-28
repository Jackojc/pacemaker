# beatgen
.POSIX:

include config.mk

all: options beatgen

config:
	mkdir -p build/

options:
	@printf "cxx \033[32m$(CXX)\033[0m | "
	@printf "dbg \033[32m$(DBG)\033[0m\n"

beatgen: config options
	$(CXX) $(PROJ_CXXFLAGS) src/beatgen.cpp -o build/beatgen $(PROJ_LDFLAGS)

clean:
	rm -rf build/beatgen beatgen-$(VERSION).tar.gz

dist: clean beatgen
	mkdir -p beatgen-$(VERSION)
	cp -R LICENSE Makefile README.md config.mk src/ doc/ modules/ beatgen-$(VERSION)
	tar -cf - beatgen-$(VERSION) | gzip > beatgen-$(VERSION).tar.gz
	rm -rf beatgen-$(VERSION)

install: beatgen
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f build/beatgen $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/beatgen
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < doc/beatgen.1 > $(DESTDIR)$(MANPREFIX)/man1/beatgen.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/beatgen.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/beatgen
	rm -f $(DESTDIR)$(MANPREFIX)/man1/beatgen.1

.PHONY: all config options clean dist install uninstall


