## Style Rules
* Lines are not to exceed 89 characters.  Classes and functions should
  be under 66 lines if possible.
* Use boost only in exceptional circumstances.
* Try to be explicit unless it's a total pain in the ass.

## Errors

You should throw an exception (which will be propagated back to the
user) by using either `rcheck(predicate, error_message)` or
`rfail(printf_style_args)`.  In debug mode, `rfail` will include the
assertion that failed and the line it failed on in the error message.
In the future we will have many more types of errors.

## Checkpoints

If you're doing some computation that will produce needless allocations, you should use an `env_checkpoint_t` to create a checkpoint in the environment.  You can read more about this in `env.hpp` at the bottom, and see some examples in e.g. the code for filter.  There's also an `env_gc_checkpoint_t` used for reduce and gmr.

## wire_datum_t, wire_datum_map_t, wire_func_t, etc.

A `wire_x_t` object allows an `x` to be serialized over the wire.
Generally you need to call `finalize` on it before writing it over the
wire (if such a method exists) and `compile` after reading it off the
wire but before accessing its data.  (Don't ask, it's a long story
about scopes, but it never hurts to make code more explicit.)  These
objects keep track of their state (e.g. a `wire_datum_t` can be one of
`{ INVALID, JUST_READ, COMPILED, READY_TO_WRITE }`), and will error at
runtime if you try to perform an operation that is illegal in their
current state.

## Random Notes
* `%.20g` is the accepted way of printing doubles.
