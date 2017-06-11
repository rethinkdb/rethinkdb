#!/usr/bin/env bash
#
# Runs clang-tidy on the c++ sources
# Tested with LLVM 3.9.1
#
# How to use:
#
#  $ ./configure CXX=`pwd`/scripts/clang-tidy.sh
#  $ make
#
# make must be called from the top level of the repo
#
# clang-tidy flags are prefixed with --tidy. For example, to
# automatically fix some detected issues, pass `CXXFLAGS=--tidy-fix'
#
# List all possible checks:
#
#  clang-tidy --checks='*' --list-checks

set -e

checks=
check () { checks="$checks,$1"; }

check '*'
check -clang-analyzer-alpha.'*'
check -cppcoreguidelines-pro-type-reinterpret-cast
check -cppcoreguidelines-pro-type-vararg
check -modernize-use-bool-literals
check -readability-else-after-return
check -cppcoreguidelines-pro-bounds-'*'
check -google-readability-todo

clang_tidy_flags=
cxx_flags=
source=

absdir () (
    cd "$(dirname $1)"
    pwd
)

src_dir=$(absdir $(dirname "$0"))/src

for arg in "$@"; do
    case "$arg" in
	src/*.cc)
	    case $(absdir $arg) in
		 $src_dir/*) source="$arg"
	    esac ;;
	--tidy-*)
	    clang_tidy_flags="$clang_tidy_flags $(printf "%q" "${arg#--tidy}")"
	    continue ;;
    esac
    cxx_flags="$cxx_flags $(printf "%q" "$arg")"
done

extra_includes () {
    set -- `clang++ '-###' -x c++ -c <(true) 2>&1 | tail -n 1`
    while [ $# != 0 ]; do
	case "$1" in
	    *-isystem*) echo -n " -isystem" "$2"; shift ;;
	    *-idirafter*) echo -n " -idirafter" "$2"; shift ;;
	esac
	shift
    done
}

if [ "$source" != "" ]; then
    eval clang-tidy -header-filter=$src_dir --checks="$checks" "$clang_tidy_flags" "$source" -- "$cxx_flags" "$(extra_includes)"
else
    eval "clang++ $cxx_flags"
fi
