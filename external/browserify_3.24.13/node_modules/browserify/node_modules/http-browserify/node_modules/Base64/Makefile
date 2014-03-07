.PHONY: bytes clean setup test

bin = node_modules/.bin

base64.min.js: base64.js
	@$(bin)/uglifyjs $< --compress --mangle > $@

bytes: base64.min.js
	@gzip --best --stdout $< | wc -c | tr -d ' '

clean:
	@rm -rf node_modules
	@git checkout -- *.min.js

setup:
	@npm install

test:
	@$(bin)/mocha test --compilers coffee:coffee-script
