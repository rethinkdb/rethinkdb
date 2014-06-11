Protocol Buffers test
---------------------

With version 1.13 RethinkDB moves from using Protocol Buffers in the drivers to speaking JSON with the server.
However, the server continues to speak Protocol Buffers to allow third-party drivers to continue to work.
This test uses a third-party C++ driver to make a few requests to the server over Protocol Buffers to ensure 
the server side is not broken.

Run the test by invoking the `protobuf.test` file in this directory.

Acknowledgement to rethink-db-cpp-driver project
------------------------------------------------

https://github.com/jurajmasar/rethink-db-cpp-driver

This test uses the rethink-db-cpp-driver project as a simple driver to connect to the server with Protocol Buffers.
A few minor changes have been made to the project to allow it to compile cleanly on unix systems.
The RethinkDB team thanks GitHub user @jurajmasar for this project.
