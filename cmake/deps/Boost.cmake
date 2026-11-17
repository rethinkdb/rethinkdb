# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build Boost.

include(FetchContent)

# 1.60 does not compile as C++17 (std::unary_function, std::auto_ptr, and
# Boost.MPL integral_wrapper). 1.84 is the oldest pin checked against the
# headers this tree includes that does.
set(_rethinkdb_boost_version "1.84.0")
string(REPLACE "." "_" _rethinkdb_boost_underscore "${_rethinkdb_boost_version}")

rethinkdb_fetch_source(boost
    URL "https://archives.boost.io/release/${_rethinkdb_boost_version}/source/boost_${_rethinkdb_boost_underscore}.tar.bz2"
    URL_HASH "SHA256=cc4b893acf645c9d4b698e9a0f08ca8846aa5d6c68275c14c3e7949c24109454"
)

add_library(RethinkDB_boost INTERFACE)
add_library(RethinkDB::boost ALIAS RethinkDB_boost)
target_include_directories(RethinkDB_boost SYSTEM INTERFACE "${boost_SOURCE_DIR}")
rethinkdb_record_dist_source(boost "${boost_SOURCE_DIR}")

message(STATUS "Using Boost ${_rethinkdb_boost_version} headers from ${boost_SOURCE_DIR}")
