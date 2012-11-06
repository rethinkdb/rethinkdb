# Copyright 2010-2012 RethinkDB, all rights reserved.

all:
	cd src ; $(MAKE) $(MFLAGS) ;

install:
	cd src ; $(MAKE) $(MFLAGS) install ;

clean:
	rm -rf build

build-deb-src-control:
	cd src ; $(MAKE) $(MFLAGS) build-deb-src-control ;

build-deb-src: build-deb-src-control
	yes | debuild -S -sa ;

deb:
	cd src ; $(MAKE) $(MFLAGS) deb ;

.PHONY: all install clean build-deb-src-control build-deb-src deb

