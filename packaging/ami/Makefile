
VERSION = $(error please set the VERSION variable)

.PHONY: ami
ami: build-ami-files/static-web
	./build-ami.sh --ami-name rethinkdb-$(VERSION)

.PHONY: web
web: build-ami-files/static-web

build-ami-files/static-web: static-web-jekyll
	jekyll build -s $< -d $@
