// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef FSCK_FSCK_HPP_
#define FSCK_FSCK_HPP_

int run_fsck(int argc, char **argv);

namespace fsck {
    void usage(const char *name);
}

#endif  // FSCK_FSCK_HPP_
