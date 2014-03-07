// Test program to demonstrate that Linux does not support AT_SYMLINK_NOFOLLOW

//  Copyright Duncan Exon Smith 2012

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

// Test this by running:
//
// rm -rf data && mkdir data && g++ -otest-fchmodat fchmodat_AT_SYMLINK_NOFOLLOW_6659.cpp && (cd data && ../test-fchmodat)
//
// If no assertions go off, then it looks like fchmodat is supported,
// but AT_SYMLINK_NOFOLLOW is not supported.

#include <fstream>
#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>

#ifdef NDEBUG
# error This program depends on assert() so makes no sense if NDEBUG is defined
#endif

int main(int argc, char *argv[])
{
    { std::ofstream file("out"); file << "contents"; }

    assert(!::symlink("out", "sym"));

    assert(!::fchmodat(AT_FDCWD, "out", S_IRUSR | S_IWUSR | S_IXUSR, 0));
    assert(!::fchmodat(AT_FDCWD, "sym", S_IRUSR | S_IWUSR | S_IXUSR, 0));

    assert(::fchmodat(AT_FDCWD, "sym", S_IRUSR | S_IWUSR | S_IXUSR, AT_SYMLINK_NOFOLLOW) == -1);
    assert(errno == ENOTSUP);

    return 0;
}
