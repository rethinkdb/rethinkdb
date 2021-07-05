# RethinkDB Old Admin

This repo has the "old" RethinkDB admin UI.  That's possibly the
"current" admin UI, but a new one is under development.

The "old" admin UI originally shared a repository with the main
RethinkDB repository, the one at
https://github.com/rethinkdb/rethinkdb .

This repository is formed from RethinkDB's main repo, branch v2.4.x,
by removing everything that isn't admin UI-related, while keeping the
build system.  That means the following got removed:

 - The C++ source code (except for src/rdb_protocol/ql2.proto)

 - Some library packages in mk/support/pkg that are included in the
   C++ executable (for example, v8, gtest, zlib, re2).

 - Drivers other than the JavaScript driver

 - Test code (in ./tests)

 - Other miscellanea

The web_assets.cc file still gets generated in the same place: In
src/gen/web_assets.cc.

## Usage

To generate src/gen/web_assets.cc, run the following:

    ./configure --fetch all CXX=clang++
    make -j8 generate

To generate a web_assets.cc for a specific RethinkDB version, you might want to try

    make PVERSION=<your choice> -j8 generate

Once src/gen/web_assets.cc is built, you can copy the file into your
rethinkdb repository, replacing the existing one at the same location,
src/gen/web_assets.cc.  Then, presumably, commit that version.

## Development Process

To develop the "old" admin UI, you just need an existing RethinkDB
binary.  You don't have to build it yourself.  Suppose this repository
is checked out at ~/rethinkdb-old-admin.  Then run the following:

    cd ~/rethinkdb-old-admin
    make web-assets
    make web-assets-watch

That will automatically update ~/rethinkdb-old-admin/build/web_assets
when you make changes in ~/rethinkdb-old-admin/admin.  Then, in
another terminal, run:

    rethinkdb --web-static-directory ~/rethinkdb-old-admin/build/web_assets

Now you can work on the web UI in ~/rethinkdb-old-admin/admin/ and
semi-instantly view the results in a web browser.

Then, once you're satisfied with your changes, interrupt the `make
web-assets-watch` with Ctrl+C and use `make generate` to create your
web_assets.cc, as described in the Usage section above.
