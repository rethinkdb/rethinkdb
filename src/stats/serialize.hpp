#ifndef _STATS_SERIALIZE_HPP_
#define _STATS_SERIALIZE_HPP_

#include <stdint.h>
#include <istream>
#include <ostream>

// These functions assume stream errors are impossible, and use rassert() to check.
void serialize_u64(std::ostream &s, uint64_t val);
uint64_t unserialize_u64(std::istream &s);
void serialize_i64(std::ostream &s, int64_t val);
int64_t unserialize_i64(std::istream &s);
void serialize_float(std::ostream &s, float val);
float unserialize_float(std::istream &s);

#endif
