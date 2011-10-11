## rethink_redis_test.tcl

source tests/support/test.tcl
source tests/support/util.tcl
source tests/support/redis.tcl

set ::denytags {}
set ::allowtags {}
set ::curfile ""
set ::tags {}
set ::verbose 0
set ::traceleaks 0

proc tags {tags code} {
    set ::tags [concat $::tags $tags]
    uplevel 1 $code
    set ::tags [lrange $::tags 0 end-[llength $tags]]
}

# An alternate definition for the 'start_server' functions 
# supplied by the redis test suite for use with RethinkDB

proc start_server {options {code undefined} } {
    # We assume that the python RethinkDB test harness has already
    # started a server on our behalf. Therefore the only thing
    # we really have to do is run the test code.
    
    uplevel 1 $code
    return
}

proc execute_tests name {
    set path "tests/$name.tcl"
    source $path
}

set ::client [redis 127.0.0.1 6380]
proc r {args} {
    $::client {*}$args
}

# This script executes the test file specified
execute_tests [lindex $argv 0]
