#!/bin/sh -e

# Copyright 2008 Lubomir Bourdev and Hailin Jin
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

if [ $# -lt 1 ]
    then
    $0 html
    $0 png
    exit
fi

echo $# extensions to process
for file_extension in $@
  do
  echo Processing extension $file_extension ...
  kk=0
  for long_file_name in *.$1
    do
    file_name_length=`gexpr length $long_file_name`
    if [ $file_name_length -gt 20 ]
        then
        kk=`gexpr $kk + 1`
        short_file_name=`printf "g_i_l_%04d.$1" $kk`
        echo \ \ Shortening $long_file_name to $short_file_name ...
        sed_string="s/\\\"$long_file_name/\\\"$short_file_name/g"
        grep -l $long_file_name *.htm* | xargs gsed -i $sed_string
        mv $long_file_name $short_file_name
    fi
  done
done
