#!/usr/bin/python
"""
Filters a documentation file for the appropriate language.

Usage:
    ./filter_docs.py RB < docs.md > ruby_docs.md
"""

import re
import sys

TYPES = ["PY", "RB", "JS"]

TYPE_REGEX = "(?:" + '|'.join(TYPES) + ")"

TAG_REGEX = re.compile("<(?:" + TYPE_REGEX + r"(?:\|\|" + TYPE_REGEX + ")*|/)>")
# after_tag_regex matches at least one whitespace character,
# disallowing text to run up right against the tag.
AFTER_TAG_REGEX = re.compile(r"\n| +\n?")

CLEAR_TAG = "</>"

def main():
    "The main function..."

    mytype = sys.argv[1]
    assert(mytype in TYPES)

    text = sys.stdin.read()

    # Used to force the last tag to be </>.
    last_tag_was_clear_tag = True

    # True when the next segment should be output (because the previous
    # tag was ours or the clear tag).
    output_on = True
    pos = 0
    while True:
        match = TAG_REGEX.search(text, pos)
        if not match:
            break

        # Output text we skipped, perhaps.
        if output_on:
            sys.stdout.write(text[pos:match.start()])

        # Figure out whether to make the subsequent segment output,
        # i.e. after a clear tag or a tag containing our language type.
        output_on = (match.group(0) == CLEAR_TAG or
                     match.group(0).find(mytype) != -1)

        # Update whether the last tag was a clear tag.
        last_tag_was_clear_tag = (match.group(0) == CLEAR_TAG)

        # Update pos to the appropriate value.  Yes, we skip some
        # whitespace characters after the tag!
        after_tag_match = AFTER_TAG_REGEX.match(text, match.end(0))
        if after_tag_match:
            pos = after_tag_match.end(0)
        elif len(text) == match.end(0):
            pos = match.end(0)
        elif text[match.end(0)] == ' ':
            pos = match.end(0) + 1
        else:
            # We expect tags to be followed by a whitespace character that
            # they may ignore.
            assert(False)

    # Output the last segment.

    # This assertion could fail with bad user input.
    assert(last_tag_was_clear_tag)

    # This assertion can never fail.
    assert(output_on)

    sys.stdout.write(text[pos:])

if __name__ == "__main__":
    main()

