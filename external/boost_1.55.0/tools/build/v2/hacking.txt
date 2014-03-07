Copyright 2003, 2006 Vladimir Prus
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)


             ----------------------------------
             Boost.Build contributor guidelines
             ----------------------------------

Boost.Build is an open-source project. This means that we welcome and appreciate
all contributions --- be it ideas, bug reports, or patches. This document
contains guidelines which helps to assure that development goes on smoothly, and
changes are made quickly.

The guidelines are not mandatory, and you can decide for yourself which one to
follow. But note, that 10 mins that you spare writing a comment, for example,
might lead to significally longer delay for everyone.

Before contributing, make sure you are subscribed to our mailing list

   boost-build@lists.boost.org

Additional resources include

   - The issue tracker
     http://trac.lvk.cs.msu.su/boost.build/

   - mailing list:
     boost-build@lists.boost.org
     http://lists.boost.org/boost-build/


BUGS and PATCHES

Both bugs and patches can be send to our mailing list.

When reporting a bug, please try to provide the following information.

   - What you did. A minimal reproducible testcase is very much appreciated.
     Shell script with some annotations is much better than verbose description
     of the problem. A regression test is the best (see test/test_system.html).
   - What you got.
   - What you expected.
   - What version of Boost.Build and Boost.Jam did you use. If possible,
     please try to test with the CVS HEAD state.

When submitting a patch, please:

  - make a single patch for a single logical change
  - follow the policies and coding conventions below,
  - send patches in unified diff format,
    (using either "cvs diff -u" or "diff -u")
  - provide a log message together with the patch
  - put the patch and the log message as attachment to your email.

The purpose of log message serves to communicate what was changed, and *why*.
Without a good log message, you might spend a lot of time later, wondering where
a strange piece of code came from and why it was necessary.

The good log message mentions each changed file and each rule/method, saying
what happend to it, and why. Consider, the following log message

    Better direct request handling.

     * new/build-request.jam
       (directly-requested-properties-adjuster): Redo.

     * new/targets.jam
       (main-target.generate-really): Adjust properties here.

     * new/virtual-target.jam
       (register-actual-name): New rule.
       (virtual-target.actualize-no-scanner): Call the above, to detected bugs,
       where two virtual target correspond to one Jam target name.

The log messages for the last two files are good. They tell what was changed.
The change to the first file is clearly undercommented.

It's OK to use terse log messages for uninteresting changes, like ones induced
by interface changes elsewhere.


POLICIES.

1. Testing.

All serious changes must be tested. New rules must be tested by the module where
they are declared. Test system (test/test_system.html) should be used to verify
user-observable behaviour.

2. Documentation.

It turns out that it's hard to have too much comments, but it's easy to have too
little. Please prepend each rule with a comment saying what the rule does and
what arguments mean. Stop for a minute and consider if the comment makes sense
for anybody else, and completely describes what the rules does. Generic phrases
like "adjusts properties" are really not enough.

When applicable, make changes to the user documentation as well.


CODING CONVENTIONS.

    1. All names of rules and variables are lowercase with "-" to separate
    words.

        rule call-me-ishmael ( ) ...

    2. Names with dots in them are "intended globals". Ordinary globals use a
    dot prefix:

        .foobar
        $(.foobar)

    3. Pseudofunctions or associations are <parameter>.<property>:

        $(argument).name = hello ;
        $($(argument).name)

    4. Class attribute names are prefixed with "self.":

        self.x
        $(self.x)

    5. Builtin rules are called via their ALL_UPPERCASE_NAMES:

        DEPENDS $(target) : $(sources) ;

    6. Opening and closing braces go on separate lines:

        if $(a)
        {
            #
        }
        else
        {
            #
        }
