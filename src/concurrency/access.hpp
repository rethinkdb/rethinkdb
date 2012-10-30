// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_ACCESS_HPP_
#define CONCURRENCY_ACCESS_HPP_

/* TODO: This enum is pretty terrible. It's used for several barely-related
things:

* The buffer cache uses it to identify transaction modes. For this, `rwi_intent`
    and `rwi_upgrade` are illegal.
* The buffer cache uses it to identify block acquisition modes. For this,
    `rwi_intent` and `rwi_upgrade` are illegal, and all the `rwi_read_*` modes
    are equivalent (I think)
* `rwi_lock_t` uses it to identify how the lock is being locked or unlocked. For
    this, `rwi_read_outdated_ok` and `rwi_read_sync` are illegal.

Probably it should be split into several enums. */

enum access_t {
    // Intent to read
    rwi_read,

    // Intent to read, but with relaxed semantics that the object is
    // permitted to become outdated while the read is
    // happening. Essentially, this allows a copy-on-write
    // implementation where an object accessed via
    // rwi_read_outdated_ok can be copied to a different location and
    // written to by another task.
    rwi_read_outdated_ok,

    // Intent to read, but guarantees that starting transactions
    // does keep in order with write transactions. In other words,
    // this means that the read transaction can see the effects
    // of any write transactions started before it.
    // Specifically, those read transactions get throttled just like
    // writes.
    rwi_read_sync,

    // Intent to write
    rwi_write,

    // Intent to read, with an additional hint that an upgrade to a
    // write mode might follow
    rwi_intent,

    // Upgrade from rwi_intent to rwi_write
    rwi_upgrade
};

inline bool is_read_mode(access_t mode) {
    return mode == rwi_read_sync || mode == rwi_read || mode == rwi_read_outdated_ok;
}
inline bool is_write_mode(access_t mode) {
    return !is_read_mode(mode);
}

#endif // CONCURRENCY_ACCESS_HPP_

