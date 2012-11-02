all:

build-deb-src-control:
	cd src ; make $(MFLAGS) build-deb-src-control ;

build-deb-src: build-deb-src-control
	yes | debuild -S -sa ;

deb:
	cd src ; make $(MFLAGS) deb ;

