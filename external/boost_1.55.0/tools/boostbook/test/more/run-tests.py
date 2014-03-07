#!/usr/bin/env python

# Copyright 2010 Daniel James.
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Boostbook tests

Usage: python build_docs.py [--generate-gold]
"""

import difflib, getopt, os, re, sys
import lxml.ElementInclude
from lxml import etree
from collections import defaultdict

# Globals

def usage_and_exit():
    print __doc__
    sys.exit(2)

def main(argv):
    script_directory = os.path.dirname(sys.argv[0])
    boostbook_directory = os.path.join(script_directory, "../../xsl")

    try:
        opts, args = getopt.getopt(argv, "",
            ["generate-gold"])
        if(len(args)): usage_and_exit()
    except getopt.GetoptError:
        usage_and_exit()

    generate_gold = False

    for opt, arg in opts:
        if opt == '--generate-gold':
            generate_gold = True

    # Walk the test directory

    parser = etree.XMLParser()
    
    try:
        boostbook_xsl = etree.XSLT(
            etree.parse(os.path.join(boostbook_directory, "docbook.xsl"), parser)
        )
    except lxml.etree.XMLSyntaxError, error:
        print "Error parsing boostbook xsl:"
        print error
        sys.exit(1)
    
    for root, dirs, files in os.walk(os.path.join(script_directory, 'tests')):
        for filename in files:
            (base, ext) = os.path.splitext(filename)
            if (ext == '.xml'):
                src_path = os.path.join(root, filename)
                gold_path = os.path.join(root, base + '.gold')
                try:
                    doc_text = run_boostbook(parser, boostbook_xsl, src_path)
                except:
                    # TODO: Need better error reporting here:
                    print "Error running boostbook for " + src_path
                    continue

                if (generate_gold):
                    file = open(gold_path, 'w')
                    try:
                        file.write(doc_text)
                    finally: file.close()
                else:
                    file = open(gold_path, 'r')
                    try:
                        gold_text = file.read()
                    finally:
                        file.close()
                    compare_xml(src_path, doc_text, gold_text)

def run_boostbook(parser, boostbook_xsl, file):
    doc = boostbook_xsl(etree.parse(file, parser))
    normalize_boostbook_ids(doc)
    return etree.tostring(doc)

def normalize_boostbook_ids(doc):
    ids = {}
    id_bases = defaultdict(int)

    for node in doc.xpath("//*[starts-with(@id, 'id') or contains(@id, '_id')]"):
        id = node.get('id')
        
        if(id in ids):
            print 'Duplicate id: ' + id
        
        match = re.match("(.+_id|id)([mp]?\d+)((?:-bb)?)", id)
        if(match):
            # Truncate id name, as it sometimes has different lengths...
            match2 = re.match("(.*?)([^.]*?)(_?id)", match.group(1))
            base = match2.group(1) + match2.group(2)[:14] + match2.group(3)
            count = id_bases[base] + 1
            id_bases[base] = count
            ids[id] = base + str(count) + match.group(3)

    for node in doc.xpath("//*[@linkend or @id]"):
        x = node.get('linkend')
        if(x in ids): node.set('linkend', ids[x])
        x = node.get('id')
        if(x in ids): node.set('id', ids[x])

def compare_xml(file, doc_text, gold_text):
    # Had hoped to use xmldiff but it turned out to be a pain to install.
    # So instead just do a text diff.
    
    if (doc_text != gold_text):
        print "Error: " + file
        print
        sys.stdout.writelines(
            difflib.unified_diff(
                gold_text.splitlines(True),
                doc_text.splitlines(True)
            )
        )
        print
        print

if __name__ == "__main__":
    main(sys.argv[1:])
