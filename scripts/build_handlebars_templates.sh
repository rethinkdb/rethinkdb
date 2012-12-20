#!/bin/bash
mkdir -p ../build/debug/webtemp/
mkdir -p ../build/debug/webtemp/raw_split
mkdir -p ../build/debug/webtemp/handlebars
# Takes about 2 seconds to split files
for file in $(find ../admin/static/handlebars -name '*.html');
    do
    sed 's/<script/\n&/g;s/^\n\(<script\)/\1/' $file | csplit -zf ../build/debug/webtemp/raw_split/template - '/^<script/' {*} &> /dev/null
    for splitted_file in $(find ../build/debug/webtemp/raw_split/ -name 'template*');
        do
        mv "$splitted_file" ../build/debug/webtemp/handlebars/`head -1 "$splitted_file" | cut -d '"' -f 2`.handlebars;
    done # Rename the file with a proper name
done
# Takes about one second to minify the templates
for file in ../build/debug/webtemp/handlebars/*.handlebars; do

    sed -i 's/<\/script>//;s/<script.*>//;' $file; # Remove </script> and <script id="...">
    sed -i -e :a -e N -e 's/\n/ /' -e ta $file; # Concat lines
    sed -i 's/^ *//;s/ *$//;s/ \{1,\}/ /g;/^$/d' $file; # Concat spaces and remove empty lines

    # This thing doesn't make the script faster
    #cat file | sed 's/<\/script>//;s/<script.*>//;' | sed -e :a -e N -e 's/\n/ /' -e ta | sed 's/^ *//;s/ *$//;s/ \{1,\}/ /g;/^$/d' > file

    # Separate commands
    #sed -i 's/<\/script>//' $file; # Remove </script>
    #sed -i 's/<script.*>//' $file; # Remove <script id="...">
    #sed -i -e :a -e N -e 's/\n/ /' -e ta $file; # Concat lines
    #sed -i 's/^ *//;s/ *$//;s/ \{1,\}/ /g' $file; # Concat spaces
    #sed -i '/^$/d' $file; # Remove empty lines
done
# -m minify, -f is the output, it takes about 2 seconds
handlebars -m ../build/debug/webtemp/handlebars -f ../build/debug/web/js/template.js
rm -R ../build/debug/webtemp/
