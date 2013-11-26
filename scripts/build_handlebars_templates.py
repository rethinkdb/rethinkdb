#!/usr/bin/python

import re
import shutil
import subprocess
import sys
import os
import fnmatch
import io

HTML_INPUT_DIR = sys.argv[1]
BUILD_DIR = sys.argv[2]
OUTPUT_DIR = sys.argv[3]

# For some reason the handlebars executable name is passed via the
# TC_HANDLEBARS_EXE environment variable.
HANDLEBARS_EXE = os.environ['TC_HANDLEBARS_EXE'] if os.environ['TC_HANDLEBARS_EXE'] else 'handlebars'


WEB_TEMP_DIR = BUILD_DIR + "/webtemp"


def split_by_script_tags(text):
    pos = 0
    parts = []
    search_string = '<script'
    while pos < len(text):
        findres = text.find(search_string, pos + 1)
        if findres == -1:
            parts += [text[pos:]]
            pos = len(text)
        else:
            parts += [text[pos:findres]]
            pos = findres
    return parts

# We want to look through all the .html files, split them at the
# beginning of any "<script" match, and then do some business cleaning
# up the parts.
try:
    # Make a temporary directory for these chopped up template files.
    os.makedirs(WEB_TEMP_DIR)

    parts = []

    # Split all the html files by script tags.
    for root, dirnames, filenames in os.walk(HTML_INPUT_DIR):
        for filename in fnmatch.filter(filenames, "*.html"):
            path = os.path.join(root,filename)
            with io.open(path, encoding='utf-8') as f:
                text = f.read()
                tmp_parts = split_by_script_tags(text)
                parts += tmp_parts

    # Now build parts corresponding to script tags.
    named_parts = { }
    for part in parts:
        # We use the first line of the file to create the filename for
        # non-script-tag-led matches?  You could ask Michel why we do
        # that.
        if re.search('<script ((?!id=)[^>])*>', part):
            raise RuntimeError("Found script tag without id.")
        searchres = re.search('<script [^>]*id="([^"]+)"', part)
        name = searchres.group(1) if searchres else part.split('\n', 1)[0]
        part1 = re.sub('</script>|<script[^>]*>', '', part)
        part2 = re.sub('\s+', ' ', part1)
        named_parts[name] = part2

    # Write the parts to files to the temporary directory, so that we can pass them to handlebars.
    for name in named_parts:
        with open(WEB_TEMP_DIR + '/' + name + '.handlebars', 'w') as f:
            f.write(named_parts[name])

    # Now, we call handlebars.
    subprocess.call([HANDLEBARS_EXE, '-m', WEB_TEMP_DIR, '-f', OUTPUT_DIR + '/template.js']);

finally:
    shutil.rmtree(WEB_TEMP_DIR)
