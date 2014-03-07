#  (C) Copyright David Abrahams 2002. Permission to copy, use, modify, sell and
#  distribute this software is granted provided this copyright notice appears in
#  all copies. This software is provided "as is" without express or implied
#  warranty, and with no claim as to its suitability for any purpose.

import operator

def unique (values, stable=False):
    if stable:
        s = set()
        r = []
        for v in values:
            if not v in s:
                r.append(v)
                s.add(v)
        return r
    else:
        return list(set(values))

def max_element (elements, ordered = None):
    """ Returns the maximum number in 'elements'. Uses 'ordered' for comparisons,
        or '<' is none is provided.
    """
    if not ordered: ordered = operator.lt

    max = elements [0]
    for e in elements [1:]:
        if ordered (max, e):
            max = e

    return max

def select_highest_ranked (elements, ranks):
    """ Returns all of 'elements' for which corresponding element in parallel
        list 'rank' is equal to the maximum value in 'rank'.
    """
    if not elements:
        return []

    max_rank = max_element (ranks)

    result = []
    while elements:
        if ranks [0] == max_rank:
            result.append (elements [0])

        elements = elements [1:]
        ranks = ranks [1:]

    return result
