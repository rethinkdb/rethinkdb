
#ifndef __LOG_HPP__
#define __LOG_HPP__

/**
 * This is the log-structured serializer, the holiest of holies of
 * RethinkDB. Please treat it with courtesy, professionalism, and
 * respect that it deserves.
 */

/**
 * Functionality breakdown:
 * - Extents, extent threading, extent metadata
 * - Lookup: (block_id_t, slice_id_t) -> off64_t
 * - Durability gurantees
 * - Metadata about lookup root (last extent)
 * - Garbage collection
 */

/**
 * Best done in two layers:
 *
 * - LogFS that provides an illusion of a log structured system
 *   - Extents, extent threading, extent metadata
 *   - Metadata about lookup root (last extent)
 *   - Garbage collection
 *   - Atomic durability gurantees
 *
 * - Serializer on top of LogFS
 *   - Lookup: (block_id_t, slice_id_t) -> off64_t
 */

#endif // __LOG_HPP__

