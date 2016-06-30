# RethinkDB Java driver hacking guide

This readme is for anyone who wants to dive in and hack on the Java
driver. If you want to learn to use the Java driver, checkout out the
documentation at [java-api-docs][]

[java-api-docs]: http://rethinkdb.com/api/java/

## Basics

The Java driver is built using gradle (2.7+). To build the driver from
source, you need gradle, Java, and python3.4 installed:

```bash
$ make
```

The build process runs a python script (`metajava.py`) that
automatically generates the required Java classes for reql terms. The
process looks like this:

```
metajava.py creates:

ql2.proto -> proto_basic.json ---+
        |                        |
        |                        v
        +--> term_info.json -> $BUILD_DIR/java_term_info.json
                                 |
                                 v
global_info.json ------------> templates/ -> $PACKAGE_DIR/gen/ast/{Term}.java
                                       |
                                       +---> $PACKAGE_DIR/gen/proto/{ProtocolType}.java
                                       +---> $PACKAGE_DIR/gen/model/TopLevel.java
                                       +---> $PACKAGE_DIR/gen/exc/Reql{Exception}Error.java
```

Generally, you won't need to use metajava.py if you only want to build
the current driver, since all generated files are checked into
git. Only if you want to modify the templates in `templates/` will you
need python3 and mako installed.

If you're building the java driver without changing the template
files, you can simply do:

```bash
$ gradle assemble
# or if you want to run tests as well
$ gradle build
```

This will create the .jar files in
`$BUILD_DIR/drivers/java/gradle/libs` where `$BUILD_DIR` is usually
`../../build` from the directory where this README is located.

## Testing

Tests are created from the polyglot yaml tests located in
`../../test/rql_test/src/`. The script `convert_tests.py` is used to
run it, which requires python3 to be installed, but otherwise has no
external dependencies.

`convert_tests.py` will output junit test files into
`src/test/java/gen`. These are also checked into git, so you don't
need to run the conversion script if you just want to verify that the
existing tests pass.

`convert_tests.py` makes use of
`process_polyglot.py`. `process_polyglot.py` is intended to be
independent of the java driver, and only handles reading in the
polyglot tests and normalizing them into a format that's easier to use
to generate new tests from. Short version: `process_polyglot.py`
doesn't have the word "java" anywhere in it, while `convert_tests.py`
has all of the java specific behavior in it and builds on top of the
stream of test definitions that `process_polyglot.py` emits.

## Deploying a release or snapshot

To deploy you'll need to create a file called `gradle.properties` with
the following format:

```
signing.keyId=<KEY_ID>
signing.password=
signing.secretKeyRingFile=<KEYRING_LOCATION>

ossrhUsername=<SONATYPE_USERNAME>
ossrhPassword=<SONATYPE_PASSWORD>
```

It goes without saying that this file should not be checked back into
git, it's in the `.gitignore` file to prevent accidents. You'll need
to add your gpg signing key id and keyring file. Usually the keyring
file is located at `~/.gnupg/secring.gpg` but it won't expand
home-dirs in the config file so you have to put the absolute path
in. If you don't have a password on your private key for package
signing, you must leave the `signing.password=` line in the properties
file, just empty.

If you have a gpg version >= 2.1 there is no `secring.gpg` file. If
that's the case, either use a gpg 2.0 or below, or maybe newer
versions of the `signing` plugin for gradle have been released and you
can fix this for gpg 2.1. (PRs welcome)

For the sonatype username and password, you'll have to sign up on the
sonatype jira website
(https://issues.sonatype.org/secure/Signup!default.jspa), and you'll
need to be added to the `com.rethinkdb` group (someone internal will
have to do that for you).

Next, you'll need to run

```bash
$ gradle uploadArchives
```

This should sign and upload the package to the release
repository. This is for official releases/betas etc. If you just want
to upload a snapshot, add the suffix `-SNAPSHOT` to the `version`
value in `build.gradle`. The gradle maven plugin decides which repo to
upload to depending on whether the version looks like `2.2` or
`2.2-SNAPSHOT`, so this is important to get right or it won't go to
the right place.

If you just want to do a snapshot: if `gradle uploadArchives`
succeeds, you're done. The snapshot will be located at
https://oss.sonatype.org/content/repositories/snapshots/com/rethinkdb/rethinkdb-driver/
with the version you gave it.

If you are doing a full release, you need to go to
https://oss.sonatype.org/#stagingRepositories and search for
"rethinkdb" in the search box, find the release that is in status
`open`. Select it and then click the `Close` button. This will check
it and make it ready for release. If that stage passes you can click
the `Release` button.

For full instructions see:
http://central.sonatype.org/pages/releasing-the-deployment.html
