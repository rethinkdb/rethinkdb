#!/bin/sh
# Copyright David Abrahams 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
PYTHONPATH="c:/src/docutils;c:/src/docutils/extras"
export PYTHONPATH
rst2html.py -gs --link-stylesheet --traceback --trim-footnote-reference-space --footnote-references=superscript --stylesheet=../../../rst.css $1 `echo $1 | sed 's/\(.*\)\..*/\1.html/'`



