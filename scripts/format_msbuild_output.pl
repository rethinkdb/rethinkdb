#!/usr/bin/perl -n

s|([a-zA-Z]):\\|/cygdrive/\1/|g;              # convert to cygwin path
s|\\|/|g;                                     # mirror slashes
s|( [^ ]+\.cc){10,}| ...|;                    # do not list all cc files on a single line
print unless m{^         .*/[0-9]/.*\.obj.?$} # do not list all object files when linking
