# C++ Coding Style

The general rule is:

 - Write code in a manner similar to that around it.

This document describes guidelines that people new to the codebase
tend not to immediately pick up on.  These aren't universal rules
(besides the first rule) and situations might require your better
judgement.

1. Use `if (...) {`, `while (...) {`, and `switch (...) {`, not `if(...){`,
`while(...){`, and `switch(...){`.  Use braces.

2.  Header files should be included in the following order:
  1. Your parent .hpp file (if you are a .cc file),
  2. C system headers (`<math.h>`, `<unistd.h>`, etc),
  3. Standard C++ headers (`<vector>`, `<algorithm>`, etc),
  4. Boost headers, with `"errors.hpp"` included before them, if there are any.
  5. Local headers with full paths (`"rdb_protocol/protocol.hpp"`, etc).

  > Reason: It's always good to have one .cc file include each .hpp file first,
    so that you know it includes all of its dependencies.  The Google Style
    Guide specifies the C/C++/Local order and we blindly assume there's a
    reason.  errors.hpp defines `BOOST_ENABLE_ASSERT_HANDLER` to make Boost
    assertion failures get forwarded to the RethinkDB assertion mechanism, so
    it needs to go before boost headers.

3. Don't use non-const lvalue references, except, perhaps, in return values.

  > Reason: The entire codebase does things this way.  Right now,
    somebody seeing `foo(bar)` can currently expect `bar` not to be
    modified.

  > Exception: `std::swap` and STL-like `swap` methods, perhaps, and
    other peculiar situations.

4. Don't make fields whose type is a reference type (const or non-const), and
don't turn references into pointers in ways that might surprise the caller.

  > Reason: This makes object lifetime dependencies easier to see (somebody
    calling `foo(bar)` can assume there's no new restrictions on the lifetime
    of `bar` -- the code must become `foo(&bar)` which shows that something
    spooky is happening).

  > Null pointer dereferences are also easier to track down than use-after-free
    bugs.  (Also, they generally don't happen: a function taking a pointer
    argument does not imply that it accepts a null pointer value, and this
    hasn't been a problem.)

5. If your type should not be copied, use the `DISABLE_COPYING` macro to make
it non-copyable.

  > Or declare its copy constructor and assignment operator with `= delete;`
    yourself.

6. Run (from the src directory) `../scripts/check_style.sh` to see if your
code has introduced any of a certain class of style errors.

  > The include-what-you-use errors exist to make you suffer -- but keeping
    awareness of this makes efforts to untangle header file dependences a lot
    less labor-intensive.

  > Bogus errors can be suppressed with `// NOLINT(specific/category)`.

  > This script has caught several bugs (mostly race conditions caused by not
    using reentrant "_r" versions of libc functions).
