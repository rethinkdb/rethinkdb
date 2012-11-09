# Copyright 2010-2012 RethinkDB, all rights reserved.

DEBUG?=0

all:
	cd src ; $(MAKE) DEBUG=$(DEBUG) PACKAGING=1 WEBRESDIR=/usr/share/rethinkdb/web ;

install: all
	cd src ; $(MAKE) DEBUG=$(DEBUG) PACKAGING=1 install ;

clean:
	rm -rf build

distclean: clean

build-deb-src-control:
	cd src ; $(MAKE) DEBUG=$(DEBUG) ALLOW_INTERNAL_TOOLS=1 INSTALL_INTERNAL_TOOLS=1 build-deb-src-control ;

build-deb-src: build-deb-src-control
#	$(shell scripts/gen-version.sh > VERSION)
	cd src ; $(MAKE) DEBUG=$(DEBUG) ALLOW_INTERNAL_TOOLS=1 INSTALL_INTERNAL_TOOLS=1 PACKAGING=1 build-deb-support ;
	yes | debuild -S -sa ;

deb:
	cd src ; $(MAKE) DEBUG=$(DEBUG) deb ;

.PHONY: all install clean build-deb-src-control build-deb-src deb

