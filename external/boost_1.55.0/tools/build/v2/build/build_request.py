# Status: being ported by Vladimir Prus
# TODO: need to re-compare with mainline of .jam
# Base revision: 40480
#
#  (C) Copyright David Abrahams 2002. Permission to copy, use, modify, sell and
#  distribute this software is granted provided this copyright notice appears in
#  all copies. This software is provided "as is" without express or implied
#  warranty, and with no claim as to its suitability for any purpose.

import b2.build.feature
feature = b2.build.feature

from b2.util.utility import *
import b2.build.property_set as property_set

def expand_no_defaults (property_sets):
    """ Expand the given build request by combining all property_sets which don't
        specify conflicting non-free features.
    """
    # First make all features and subfeatures explicit
    expanded_property_sets = [ps.expand_subfeatures() for ps in property_sets]
    
    # Now combine all of the expanded property_sets
    product = __x_product (expanded_property_sets)
    
    return [property_set.create(p) for p in product]


def __x_product (property_sets):
    """ Return the cross-product of all elements of property_sets, less any
        that would contain conflicting values for single-valued features.
    """
    x_product_seen = set()
    return __x_product_aux (property_sets, x_product_seen)[0]

def __x_product_aux (property_sets, seen_features):
    """Returns non-conflicting combinations of property sets.

    property_sets is a list of PropertySet instances. seen_features is a set of Property
    instances.

    Returns a tuple of:
    - list of lists of Property instances, such that within each list, no two Property instance
    have the same feature, and no Property is for feature in seen_features.
    - set of features we saw in property_sets      
    """
    if not property_sets:
        return ([], set())

    properties = property_sets[0].all()

    these_features = set()
    for p in property_sets[0].non_free():
        these_features.add(p.feature())

    # Note: the algorithm as implemented here, as in original Jam code, appears to
    # detect conflicts based on features, not properties. For example, if command
    # line build request say:
    #
    # <a>1/<b>1 c<1>/<b>1
    #
    # It will decide that those two property sets conflict, because they both specify
    # a value for 'b' and will not try building "<a>1 <c1> <b1>", but rather two
    # different property sets. This is a topic for future fixing, maybe.
    if these_features & seen_features:

        (inner_result, inner_seen) = __x_product_aux(property_sets[1:], seen_features)
        return (inner_result, inner_seen | these_features)

    else:

        result = []
        (inner_result, inner_seen) = __x_product_aux(property_sets[1:], seen_features | these_features)
        if inner_result:
            for inner in inner_result:
                result.append(properties + inner)
        else:
            result.append(properties)
        
        if inner_seen & these_features:
            # Some of elements in property_sets[1:] conflict with elements of property_sets[0],
            # Try again, this time omitting elements of property_sets[0]
            (inner_result2, inner_seen2) = __x_product_aux(property_sets[1:], seen_features)
            result.extend(inner_result2)

        return (result, inner_seen | these_features)

    

def looks_like_implicit_value(v):
    """Returns true if 'v' is either implicit value, or
    the part before the first '-' symbol is implicit value."""
    if feature.is_implicit_value(v):
        return 1
    else:
        split = v.split("-")
        if feature.is_implicit_value(split[0]):
            return 1

    return 0

def from_command_line(command_line):
    """Takes the command line tokens (such as taken from ARGV rule)
    and constructs build request from it. Returns a list of two
    lists. First is the set of targets specified in the command line,
    and second is the set of requested build properties."""

    targets = []
    properties = []

    for e in command_line:
        if e[:1] != "-":
            # Build request spec either has "=" in it, or completely
            # consists of implicit feature values.
            if e.find("=") != -1 or looks_like_implicit_value(e.split("/")[0]):                
                properties += convert_command_line_element(e)
            elif e:
                targets.append(e)

    return [targets, properties]
 
# Converts one element of command line build request specification into
# internal form.
def convert_command_line_element(e):

    result = None
    parts = e.split("/")
    for p in parts:
        m = p.split("=")
        if len(m) > 1:
            feature = m[0]
            values = m[1].split(",")
            lresult = [("<%s>%s" % (feature, v)) for v in values]
        else:
            lresult = p.split(",")
            
        if p.find('-') == -1:
            # FIXME: first port property.validate
            # property.validate cannot handle subfeatures,
            # so we avoid the check here.
            #for p in lresult:
            #    property.validate(p)
            pass

        if not result:
            result = lresult
        else:
            result = [e1 + "/" + e2 for e1 in result for e2 in lresult]

    return [property_set.create(b2.build.feature.split(r)) for r in result]

### 
### rule __test__ ( )
### {
###     import assert feature ;
###     
###     feature.prepare-test build-request-test-temp ;
###     
###     import build-request ;
###     import build-request : expand_no_defaults : build-request.expand_no_defaults ;
###     import errors : try catch ;
###     import feature : feature subfeature ;
### 
###     feature toolset : gcc msvc borland : implicit ;
###     subfeature toolset gcc : version : 2.95.2 2.95.3 2.95.4
###       3.0 3.0.1 3.0.2 : optional ;
### 
###     feature variant : debug release : implicit composite ;
###     feature inlining : on off ;
###     feature "include" : : free ;
### 
###     feature stdlib : native stlport : implicit ;
### 
###     feature runtime-link : dynamic static : symmetric ;
### 
### 
###     local r ;
### 
###     r = [ build-request.from-command-line bjam debug runtime-link=dynamic ] ;              
###     assert.equal [ $(r).get-at 1 ] : ;
###     assert.equal [ $(r).get-at 2 ] : debug <runtime-link>dynamic ;
### 
###     try ;
###     {
### 
###         build-request.from-command-line bjam gcc/debug runtime-link=dynamic/static ;
###     }
###     catch \"static\" is not a value of an implicit feature ;
### 
### 
###     r = [ build-request.from-command-line bjam -d2 --debug debug target runtime-link=dynamic ] ;
###     assert.equal [ $(r).get-at 1 ] : target ;
###     assert.equal [ $(r).get-at 2 ] : debug <runtime-link>dynamic ;
### 
###     r = [ build-request.from-command-line bjam debug runtime-link=dynamic,static ] ;
###     assert.equal [ $(r).get-at 1 ] : ;
###     assert.equal [ $(r).get-at 2 ] : debug <runtime-link>dynamic <runtime-link>static ;
### 
###     r = [ build-request.from-command-line bjam debug gcc/runtime-link=dynamic,static ] ;
###     assert.equal [ $(r).get-at 1 ] : ;
###     assert.equal [ $(r).get-at 2 ] : debug gcc/<runtime-link>dynamic 
###                  gcc/<runtime-link>static ;
### 
###     r = [ build-request.from-command-line bjam msvc gcc,borland/runtime-link=static ] ;
###     assert.equal [ $(r).get-at 1 ] : ;
###     assert.equal [ $(r).get-at 2 ] : msvc gcc/<runtime-link>static 
###                     borland/<runtime-link>static ;
### 
###     r = [ build-request.from-command-line bjam gcc-3.0 ] ;
###     assert.equal [ $(r).get-at 1 ] : ;
###     assert.equal [ $(r).get-at 2 ] : gcc-3.0 ;
### 
###     feature.finish-test build-request-test-temp ;
### }
### 
### 
