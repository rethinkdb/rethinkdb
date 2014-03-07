#  (C) Copyright David Abrahams 2001. Permission to copy, use, modify, sell and
#  distribute this software is granted provided this copyright notice appears in
#  all copies. This software is provided "as is" without express or implied
#  warranty, and with no claim as to its suitability for any purpose.

from utility import to_seq

def difference (b, a):
    """ Returns the elements of B that are not in A.
    """
    result = []
    for element in b:
        if not element in a:
            result.append (element)

    return result

def intersection (set1, set2):
    """ Removes from set1 any items which don't appear in set2 and returns the result.
    """
    result = []
    for v in set1:
        if v in set2:
            result.append (v)
    return result

def contains (small, large):
    """ Returns true iff all elements of 'small' exist in 'large'.
    """
    small = to_seq (small)
    large = to_seq (large)

    for s in small:
        if not s in large:
            return False
    return True

def equal (a, b):
    """ Returns True iff 'a' contains the same elements as 'b', irrespective of their order.
        # TODO: Python 2.4 has a proper set class.
    """
    return contains (a, b) and contains (b, a)
