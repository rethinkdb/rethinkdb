# C++ Coding Style

The general rule is:

 - Write code in a manner similar to that around it.

This document describes guidelines that people new to the codebase tend not to immediately pick up on.  Obviously, these aren't universal rules (besides rule 1) and situations might require your better judgement.

1. Use `if (...) {`, `while (...) {`, and `switch (...) {`, not `if(...){`, `while(...){`, and `switch(...){`.

  > Reason:  The rest of the code is that way.
  
1. You can word-wrap your function parameters, put them all on separate lines at the same indentation level, but don't put them "almost" all on separate lines at the same indentation.

  > E.g. don't do this, especially:
  >
  >     frim->triskulate_frobbles(frobble_trisgrup,
  >                               &frobbles,
  >                               env,
  >                               interruptor, 0);  // <- eek!
  >
  > This is also undesirable:
  >
  >     frim->triskulate_frobbles(frobble_trisgrup,  // <- ook!
  >             &frobbles,
  >             env,
  >             interruptor,
  >             0);
  >
  > Reason: People are more likely to overlook the unusually placed parameter.

1. Use braces, don't put control statements like `return;` or `break;` in the same line as other things.

  > Reason: Quick scan-ability of control flow is useful for checking correctness w.r.t. certain classes of bugs.

  > Exception: Sometimes it's better to put control statements on the same line.

1. Lines in new code should be under 90 characters long.

  > Reason: @mlucy's editor windows are that big.

  > Exception: If it's really annoying to do so, or if you incidentally make a minor edit to code that happens to already break that rule.

1.  Header files should be included in the following order:
  1. Your parent .hpp file (if you are a .cc file),
  2. C system headers (`<math.h>`, `<unistd.h>`, etc),
  3. Standard C++ headers (`<vector>`, `<algorithm>`, etc),
  4. Boost headers, with `"errors.hpp"` included before them, if there are any.
  5. Local headers with full paths (`"rdb_protocol/protocol.hpp"`, etc).
  It's nice if they're sorted alphabetically but not important.

  > Reason:  It's always good to have one .cc file include each .hpp file first, so that you know it includes all of its dependencies.  The Google Style Guide specifies the C/C++/Local order and we blindly assume there's a reason.  errors.hpp defines `BOOST_ENABLE_ASSERT_HANDLER` to make Boost assertion failures get forwarded to the RethinkDB assertion mechanism, so it needs to go before boost headers.

1. Don't use non-const lvalue references, except, perhaps, in return values.

  > Reason: Somebody calling `foo(bar)` can currently expect `bar` not to be modified.  Pass a pointer to bar, if you want it to be mutable.

  > Exception: `std::swap` and STL-like `swap` methods, perhaps, and other peculiar situations.

1. Don't make fields whose type is a reference type (const or non-const), and don't turn references into pointers in ways that might surprise the caller.

  > Reason: This makes object lifetime dependencies easier to see (somebody calling `foo(bar)` can assume there's no new restrictions on the lifetime of `bar` -- the code must become `foo(&bar)` which shows that something spooky is happening).

  > Null pointer dereferences are also easier to track down than use-after-free bugs.  (Also, they generally don't happen: a function taking a pointer argument does not imply that it accepts a null pointer value, and this hasn't been a problem.)

1. Don't casually use bad Boost libraries.

  > Reason: Some boost libraries greatly increase build times.

  > Don't use boost::lexical_cast -- its exact behavior is imprecisely defined.  Use `strtoi64_strict` and `strprintf` (from utils.hpp) and the like, and handle the errors.

  > Don't use boost libraries with C++11 alternatives (boost::bind, boost::function, shared_ptr, ptr_container, and the like) except as necessary to work with old code that does (of course).

  > Right now there aren't great alternatives to `boost::variant`, and a bunch of code uses it.

1. If your type should not be copied, use the `DISABLE_COPYING` macro to make it non-copyable.

  > Or declare its copy constructor and assignment operator with `= delete;` yourself.

  > Reason: If it would be buggy to copy the objects (often segfaulty), and it's easy to write code that accidentally does that.
  
1. Explicitly compare pointers to `NULL` (actually, `nullptr`), explicitly compare integers to 0, use `.has()` on `counted_t` and `scoped_ptr_t`, don't just decay them to boolean.

  > Reason: Saying "Use your best judgement" when it comes to readibility apparently doesn't work very well -- seeing that this is a pointer or integer helps readibility.
  
  > Exception:  Writing `if (pointer_type ptr = ...)` is fine.
  
  > Exception:  It's a question of fact as to whether a decay-to-bool conversion hurts readability to other people on the team.  If a reviewer thinks it does, then it does.  However, if they don't... maybe it doesn't.
  
1. Use `static_cast`, not C++ functional-style casts or C casts.  Don't use `reinterpret_cast` where a `static_cast` would do -- use `static_cast` to cast to/from `void *`.

  > Reason: `static_cast` is ugly and unmissable.  You can grep for it.  C-style casts can discard const qualifiers.  C++-style functional casts are usually somewhat bad -- you see them for signed/unsigned conversions and the like -- and making such things look scary is a good thing.
  
  > Reason: `reinterpret_cast` can have different behavior than `static_cast` (when multiple inheritance is involved), and it deserves a higher level of scrutiny.  It's for when you're actually accessing raw data via two different types.  Don't diminish its signal by using it for mere void-pointer conversions to/from the same non-void type.

1. Avoid excessive dependency entanglements.  Prefer not to declare publicly used classes inside other classes.

  > Reason: Declaring classes inside other classes means the class cannot be forward declared, which makes refactoring header file dependencies much more work.
  
  > You could declare the classes inside a namespace instead.

1. Run (from the src directory) `../scripts/check_style.sh` to see if your code has introduced any of a certain class of style errors.

  > The include-what-you-use errors exist to make you suffer -- but keeping awareness of this makes efforts to untangle header file dependences a lot less labor-intensive.

  > Bogus errors can be suppressed with `// NOLINT(specific/category)`.

  > This script has caught several bugs (mostly race conditions caused by not using reentrant "_r" versions of libc functions).

1. Write comments that make it look like you made an effort to be professional and communicative.

  > Commenting what every field is/does in a type is often a good idea.

1. Write commit messages that are not unintelligible muttering.

  > High standards here would discourage frequent committing.

1. Use the `auto` keyword to increase readability, not to decrease typing.

  > No pun intended.
