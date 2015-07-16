# RethinkDB Java driver hacking guide

This readme is for anyone who wants to dive in and hack on the Java
driver. If you want to learn to use the Java driver, checkout out the
documentation at [java-api-docs][]

[java-api-docs]: rethinkdb.com/api/java

## Basics

The Java driver is built using gradle. To build the driver from
source, you need gradle, Java, and python installed:

```bash
$ gradle build
```

The build process runs a python script (`metajava.py`) that
automatically generates the required Java classes for reql terms. The
process looks like this:

```
metajava.py creates:

ql2.proto -> proto_basic.json ---+
        |                        |
        |                        v
        +--> term_info.json -> templates/ -> ast/gen/{Term}.java
                                       |
                                       +---> proto/{ProtocolType}.java
                                       +---> response/Response.java
                                       +--->
```
