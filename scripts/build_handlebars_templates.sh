#!/bin/bash

WEB_TEMP_DIR=$2/webtemp
WEB_SPLIT_DIR=$WEB_TEMP_DIR/raw_split
WEB_HANDLEBARS_DIR=$WEB_TEMP_DIR/handlebars


mkdir -p $WEB_TEMP_DIR
mkdir -p $WEB_SPLIT_DIR
mkdir -p $WEB_HANDLEBARS_DIR

# Takes about 2 seconds to split files
for file in $(find $1 -name '*.html');
    do
    sed 's/<script/\n&/g;s/^\n\(<script\)/\1/' $file | csplit -zf $WEB_SPLIT_DIR/template - '/^<script/' {*} &> /dev/null
    for splitted_file in $(find $WEB_SPLIT_DIR/ -name 'template*');
        do
        mv "$splitted_file" $WEB_HANDLEBARS_DIR/`head -1 "$splitted_file" | cut -d '"' -f 2`.handlebars;
    done # Rename the file with a proper name
done
# Takes about one second to minify the templates

for file in $WEB_HANDLEBARS_DIR/*.handlebars; do
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

handlebars -m $WEB_HANDLEBARS_DIR -f $3/template.js
rm -R $WEB_TEMP_DIR
