#!/bin/bash

./node_modules/.bin/browserify --no-detect-globals -r ./ > bundle.js
echo ';module.exports=require("./").Buffer;' >> bundle.js
