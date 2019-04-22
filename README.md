<img style="width:100%;" src="/github-banner.png">

[RethinkDB](https://www.rethinkdb.com)
======================================


What is RethinkDB?
------------------

* **Open-source** database for building realtime web applications
* **NoSQL** database that stores schemaless JSON documents
* **Distributed** database that is easy to scale
* **High availability** database with automatic failover and robust fault tolerance

RethinkDB is the first open-source scalable database built for realtime applications. It exposes a new database access model -- instead of polling for changes, the developer can tell the database to continuously push updated query results to applications in realtime. RethinkDB allows developers to build scalable realtime apps in a fraction of the time with less effort.

To learn more, check out [rethinkdb.com](https://rethinkdb.com).

Not sure what types of projects RethinkDB can help you build? Here are a few examples:

* Build a [realtime liveblog](https://rethinkdb.com/blog/rethinkdb-pubnub/) with RethinkDB and PubNub
* Create a [collaborative photo sharing whiteboard](https://www.youtube.com/watch?v=pdPRp3UxL_s)
* Build an [IRC bot in Go](https://rethinkdb.com/blog/go-irc-bot/) with RethinkDB changefeeds
* Look at [cats on Instagram in realtime](https://rethinkdb.com/blog/cats-of-instagram/)
* Watch [how Platzi uses RethinkDB](https://www.youtube.com/watch?v=Nb_UzRYDB40) to educate


Quickstart
----------

For a thirty-second RethinkDB quickstart, check out  [rethinkdb.com/docs/quickstart](https://www.rethinkdb.com/docs/quickstart).

Or, get started right away with our ten-minute guide in these languages:

* [**JavaScript**](https://rethinkdb.com/docs/guide/javascript/)
* [**Python**](https://rethinkdb.com/docs/guide/python/)
* [**Ruby**](https://rethinkdb.com/docs/guide/ruby/)
* [**Java**](https://rethinkdb.com/docs/guide/java/) 

Besides our four official drivers, we also have many [third-party drivers](https://rethinkdb.com/docs/install-drivers/) supported by the RethinkDB community. Here's a few:

* **C#/.NET:** [RethinkDb.Driver](https://github.com/bchavez/RethinkDb.Driver), [rethinkdb-net](https://github.com/mfenniak/rethinkdb-net)
* **C++:** [librethinkdbxx](https://github.com/AtnNn/librethinkdbxx)
* **Clojure:** [clj-rethinkdb](https://github.com/apa512/clj-rethinkdb)
* **Elixir:** [rethinkdb-elixir](https://github.com/hamiltop/rethinkdb-elixir)
* **Go:** [GoRethink](https://github.com/dancannon/gorethink)
* **Haskell:** [haskell-rethinkdb](https://github.com/atnnn/haskell-rethinkdb)
* **PHP:** [php-rql](https://github.com/danielmewes/php-rql)
* **Rust:** [reql](https://github.com/rust-rethinkdb/reql)
* **Scala:** [rethink-scala](https://github.com/kclay/rethink-scala)

Looking to explore what else RethinkDB offers or the specifics of ReQL? Check out [our RethinkDB docs](https://rethinkdb.com/docs/) and [ReQL API](https://rethinkdb.com/api/).

Building
--------

First install some dependencies.  For example, on Ubuntu or Debian:

    sudo apt-get install build-essential protobuf-compiler python \
        libprotobuf-dev libcurl4-openssl-dev libboost-all-dev \
        libncurses5-dev libjemalloc-dev wget m4 g++ libssl-dev

Generally, you will need

* GCC or Clang
* Protocol Buffers
* jemalloc
* Ncurses
* Boost
* Python 2
* libcurl
* libcrypto (OpenSSL)
* libssl-dev

Then, to build:

    ./configure --allow-fetch
    # or run ./configure --allow-fetch CXX=clang++

    make -j4
    # or run make -j4 DEBUG=1

    sudo make install
    # or run ./build/debug_clang/rethinkdb


Need help?
----------

A great place to start is [rethinkdb.com/community](https://rethinkdb.com/community). Here you can find out how to ask us questions, reach out to us, or [report an issue](https://github.com/rethinkdb/rethinkdb/issues). You'll be able to find all the places we frequent online and at which conference or meetups you might be able to meet us next.

If you need help right now, you can also find us [on Slack](http://slack.rethinkdb.com/), [Twitter](https://twitter.com/rethinkdb), or IRC at [#rethinkdb](irc://chat.freenode.net/#rethinkdb) on Freenode.

**Join us now:** <a href="http://slack.rethinkdb.com/"><img valign="middle"  src="http://slack.rethinkdb.com/badge.svg"></a>

Contributing
------------

RethinkDB was built by a dedicated team, but it wouldn't have been possible without the support and contributions of hundreds of people from all over the world. We could use your help too! Check out our [contributing guidelines](CONTRIBUTING.md) to get started.

Donors
------

* [DNSimple](https://dnsimple.com) provides DNS services for the RethinkDB project.
* [ZeroTier](https://www.zerotier.com) sponsored the development of per-table configurable write aggregation including the ability to set write delay to infinite to create a memory-only table ([PR #6392](https://github.com/rethinkdb/rethinkdb/pull/6392))

Licensing
---------

RethinkDB is licensed by the Linux Foundation under the open-source Apache 2.0 license. Portions of the software are licensed by Google and others and used with permission or subject to their respective license agreements.

Where's the changelog?
----------------------
We keep [a list of changes and feature explanations here](NOTES.md).
