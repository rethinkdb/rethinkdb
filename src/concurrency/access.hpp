#ifndef __ACCESS_HPP__
#define __ACCESS_HPP__

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

    // Intent to write
    rwi_write,

    // Intent to read, with an additional hint that an upgrade to a
    // write mode might follow
    rwi_intent,

    // Upgrade from rwi_intent to rwi_write
    rwi_upgrade
};

#endif // __ACCESS_HPP__

