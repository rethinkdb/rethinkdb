
#ifndef __ACCESS_HPP__
#define __ACCESS_HPP__

enum access_t {
    // Intent to read
    rwi_read,
    
    // Intent to write
    rwi_write,
    
    // Intent to read, with an additional hint that an upgrade to a
    // write mode might follow
    rwi_intent,

    // Upgrade from rwi_intent to rwi_write
    rwi_upgrade
};

#endif // __ACCESS_HPP__

