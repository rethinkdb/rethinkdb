# Copyright 2010-2012 RethinkDB, all rights reserved.

DEBUG?=0

all:
	cd src ; $(MAKE) STRIP_ON_INSTALL=0 DEBUG=$(DEBUG) PACKAGING=1 WEBRESDIR=/usr/share/rethinkdb/web ;

install: all
	cd src ; $(MAKE) STRIP_ON_INSTALL=0 DEBUG=$(DEBUG) PACKAGING=1 install ;

clean:
	rm -rf build

distclean: clean

build-deb-src-control:
	cd src ; $(MAKE) DEBUG=$(DEBUG) ALLOW_INTERNAL_TOOLS=1 FETCH_INTERNAL_TOOLS=1 build-deb-src-control ;

build-deb-src: build-deb-src-control
#	$(shell scripts/gen-version.sh > VERSION)
	cd src ; $(MAKE) DEBUG=$(DEBUG) ALLOW_INTERNAL_TOOLS=1 FETCH_INTERNAL_TOOLS=1 PACKAGING=1 build-deb-support ;
	rm -rf build support/build support/usr ;
	yes | debuild -S -sa ;

deb:
	cd src ; $(MAKE) DEBUG=$(DEBUG) deb ;

osx:
	cd src && $(MAKE) DEBUG=0 BUILD_PORTABLE=1 STATIC=1 FETCH_INTERNAL_TOOLS=1 WEBRESDIR=/usr/local/share/rethinkdb/web BUILD_DRIVERS=0
	rm -rf build/packaging/osx/
	mkdir -p build/packaging/osx/pkg/usr/local/bin build/packaging/osx/pkg/usr/local/share/rethinkdb build/packaging/osx/dmg build/packaging/osx/install
	cp build/release/rethinkdb build/packaging/osx/pkg/usr/local/bin/rethinkdb
	cp -R build/release/web build/packaging/osx/pkg/usr/local/share/rethinkdb/web
	pkgbuild --root build/packaging/osx/pkg --identifier rethinkdb build/packaging/osx/install/rethinkdb.pkg
	productbuild --distribution packaging/osx/Distribution.xml --package-path build/packaging/osx/install/ build/packaging/osx/dmg/rethinkdb.pkg
	cp packaging/osx/uninstall-rethinkdb.sh build/packaging/osx/dmg/uninstall-rethinkdb.sh
	chmod +x build/packaging/osx/dmg/uninstall-rethinkdb.sh
	hdiutil create -volname RethinkDB -srcfolder build/packaging/osx/dmg -ov build/packaging/osx/rethinkdb.dmg

.PHONY: all install clean build-deb-src-control build-deb-src deb

