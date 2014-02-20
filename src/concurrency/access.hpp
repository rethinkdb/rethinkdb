#ifndef CONCURRENCY_ACCESS_HPP_
#define CONCURRENCY_ACCESS_HPP_

// For specifying read or write access.  The only valid values of this type are, and
// always will be, `read` and `write`.
enum class access_t { read, write };

// For specifying read access.
enum class read_access_t { read };

// For specifying write access.
enum class write_access_t { write };

#endif  // CONCURRENCY_ACCESS_HPP_
