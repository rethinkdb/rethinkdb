
.PHONY: ami
ami: build-ami-files/static-web
	./build-ami.sh --ami-name rethinkdb-`../../scripts/gen-version.sh`

.PHONY: web
web: build-ami-files/static-web

build-ami-files/static-web: static-web-jekyll
	jekyll build -s $< -d $@
