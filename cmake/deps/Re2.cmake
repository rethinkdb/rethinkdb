include(FetchContent)

set(_rethinkdb_re2_version "2015-11-01")
rethinkdb_fetch_source(re2
    URL "https://github.com/google/re2/archive/${_rethinkdb_re2_version}.tar.gz"
    URL_HASH "SHA256=4aeca04541b63af1e38682b20440fcde2207a4a8a04466359dd113c8a7d4657e"
)

# The upstream Makefile's libre2 objects. util/pcre.cc, random.cc, and
# thread.cc belong to the RE2 tests.
set(RETHINKDB_RE2_SOURCES
    util/hash.cc
    util/logging.cc
    util/rune.cc
    util/stringprintf.cc
    util/strutil.cc
    util/valgrind.cc
    re2/bitstate.cc
    re2/compile.cc
    re2/dfa.cc
    re2/filtered_re2.cc
    re2/mimics_pcre.cc
    re2/nfa.cc
    re2/onepass.cc
    re2/parse.cc
    re2/perl_groups.cc
    re2/prefilter.cc
    re2/prefilter_tree.cc
    re2/prog.cc
    re2/re2.cc
    re2/regexp.cc
    re2/set.cc
    re2/simplify.cc
    re2/stringpiece.cc
    re2/tostring.cc
    re2/unicode_casefold.cc
    re2/unicode_groups.cc
)
list(TRANSFORM RETHINKDB_RE2_SOURCES PREPEND "${re2_SOURCE_DIR}/")

add_library(RethinkDB_re2 STATIC ${RETHINKDB_RE2_SOURCES})
add_library(RethinkDB::re2 ALIAS RethinkDB_re2)
target_include_directories(RethinkDB_re2 SYSTEM PUBLIC "${re2_SOURCE_DIR}")
set_target_properties(RethinkDB_re2 PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_compile_options(RethinkDB_re2 PRIVATE -w)

if(NOT WIN32)
    target_link_libraries(RethinkDB_re2 PUBLIC Threads::Threads)
endif()

if(MSVC)
    target_compile_definitions(RethinkDB_re2 PRIVATE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
    )
    target_compile_options(RethinkDB_re2 PRIVATE /wd4005 /wd4018 /wd4244 /wd4267)
endif()

rethinkdb_record_dist_source(re2 "${re2_SOURCE_DIR}")
message(STATUS "Using RE2 ${_rethinkdb_re2_version} from ${re2_SOURCE_DIR}")
