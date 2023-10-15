// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/leaf_node.hpp"

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

#include <algorithm>
#include <set>

#include "btree/node.hpp"
#include "containers/unaligned.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

// We comment out this warning, and static_assert that pair_offsets is
// at an aligned offset.
#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 901)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif

static_assert(offsetof(leaf_node_t, pair_offsets) % 2 == 0,
              "pair_offsets must be at uint16_t alignment");

// An insane sanity check, or, documentation of exactly what we expect from the type.
static_assert(alignof(unaligned<uint16_t>) == 1, "expecting unaligned struct to have 1 byte alignment");
static_assert(offsetof(unaligned<uint16_t>, value) == 0, "expecting unaligned<>::value to have 0 offset");

namespace leaf {


// A leaf node is a set of key/value, key/value/modification_time, and
// key/deletion_time tuples with distinct keys, where modification
// time or deletion time is omitted for sufficiently old entries.  A
// key/deletion_time entry says that the key was recently deleted.
//
// In particular, all the modification times and deletions after a
// certain time are recorded, and those before a certain time are
// omitted.  You'll note that we don't have deletion entries that omit
// a timestamp.

// Leaf nodes support efficient key lookup and efficient key-order
// iteration, and efficient iteration in order of descending
// modification time.  The details of implementation are described
// later.


// These codes can appear as the first byte of a leaf node entry (see
// below) (values 250 or smaller are just key sizes for key/value
// pairs).

// Means we have a deletion entry.
const int DELETE_ENTRY_CODE = 255;

// Means we have a skipped entry exactly one byte long.
const int SKIP_ENTRY_CODE_ONE = 254;

// Means we have a skipped entry exactly two bytes long.
const int SKIP_ENTRY_CODE_TWO = 253;

// Means we have a skipped entry exactly N bytes long, of form { uint8_t 252; uint16_t N; char garbage[]; }
const int SKIP_ENTRY_CODE_MANY = 252;

// A reserved meaningless value.
const int SKIP_ENTRY_RESERVED = 251;





// Entries are contiguously connected to the end of a btree
// block. Here's what a full leaf node looks like.
//
// [magic][num_pairs][live_size][frontmost][tstamp_cutpoint][off0][off1][off2]...[offN-1]........[tstamp][entry][tstamp][entry][tstamp][entry][tstamp][entry][entry][entry][entry][entry][entry][entry]
//                                                          \___________________________/        ^                                                           ^                                         ^
//                                                                  N = num_pairs            frontmost                                                tstamp_cutpoint                            (block size)
//
// [tstamp] in [tstamp][entry] pairs are non-increasing (when you look at them
// from frontmost to tstamp_cutpoint). This is true even of skip entries.
//
// Here's what an "[entry]" may look like:
//
//   [btree key][btree value]                       -- a live entry
//   [255][btree key]                               -- a deletion entry
//   [254]                                          -- a skip entry of size one
//   [253][byte]                                    -- a skip entry of size two
//   [252][uint16_t sz][byte][byte]...[byte]      -- a skip entry of size "sz + 3"
//                     \___________________/
//                           sz bytes
//
// The extra byte(s) in a skip entry are filled with `SKIP_ENTRY_RESERVED`.
//
// The reason why we special-case skip entries that are only one or two bytes
// long is that we the header we use for a skip entry of size three or more is
// itself three bytes, so it can't fit in a slot of size one or two. We don't
// expect to actually see many entries of size one or two, but it pays to be
// thorough.


struct entry_t;
struct value_t;

bool entry_is_deletion(const entry_t *p) {
    uint8_t x = *reinterpret_cast<const uint8_t *>(p);
    rassert(x != SKIP_ENTRY_RESERVED);
    return x == DELETE_ENTRY_CODE;
}

bool entry_is_live(const entry_t *p) {
    uint8_t x = *reinterpret_cast<const uint8_t *>(p);
    rassert(x != SKIP_ENTRY_RESERVED);
    rassert(MAX_KEY_SIZE == 250);
    return x <= MAX_KEY_SIZE;
}

bool entry_is_skip(const entry_t *p) {
    return !entry_is_deletion(p) && !entry_is_live(p);
}

const btree_key_t *entry_key(const entry_t *p) {
    if (entry_is_deletion(p)) {
        return reinterpret_cast<const btree_key_t *>(1 + reinterpret_cast<const char *>(p));
    } else {
        return reinterpret_cast<const btree_key_t *>(p);
    }
}

const void *entry_value(const entry_t *p) {
    if (entry_is_deletion(p)) {
        return nullptr;
    } else {
        return reinterpret_cast<const char *>(p) + entry_key(p)->full_size();
    }
}

int entry_size(value_sizer_t *sizer, const entry_t *p) {
    uint8_t code = *reinterpret_cast<const uint8_t *>(p);
    switch (code) {
    case DELETE_ENTRY_CODE:
        return 1 + entry_key(p)->full_size();
    case SKIP_ENTRY_CODE_ONE:
        return 1;
    case SKIP_ENTRY_CODE_TWO:
        return 2;
    case SKIP_ENTRY_CODE_MANY:
        return 3 + reinterpret_cast<const unaligned<uint16_t> *>(1 + reinterpret_cast<const char *>(p))->value;
    default:
        rassert(code <= MAX_KEY_SIZE);
        return entry_key(p)->full_size() + sizer->size(entry_value(p));
    }
}

const entry_t *get_entry(const leaf_node_t *node, int offset) {
    return reinterpret_cast<const entry_t *>(reinterpret_cast<const char *>(node) + offset + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0));
}

entry_t *get_entry(leaf_node_t *node, int offset) {
    return reinterpret_cast<entry_t *>(reinterpret_cast<char *>(node) + offset + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0));
}

char *get_at_offset(leaf_node_t *node, int offset) {
    return reinterpret_cast<char *>(node) + offset;
}

repli_timestamp_t get_timestamp(const leaf_node_t *node, int offset) {
    return reinterpret_cast<const unaligned<repli_timestamp_t> *>(reinterpret_cast<const char *>(node) + offset)->value;
}

struct entry_iter_t {
    int offset;

    void step(value_sizer_t *sizer, const leaf_node_t *node) {
        rassert(!done(sizer));

        offset += entry_size(sizer, get_entry(node, offset)) + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0);
    }

    bool done(value_sizer_t *sizer) const {
        int bs = sizer->block_size().value();
        guarantee(offset <= bs, "offset=%d, bs=%d", offset, bs);
        return offset == bs;
    }

    static entry_iter_t make(const leaf_node_t *node) {
        entry_iter_t ret;
        ret.offset = node->frontmost;
        return ret;
    }
};

void strprint_entry(std::string *out, value_sizer_t *sizer, const entry_t *entry) {
    if (entry_is_live(entry)) {
        const btree_key_t *key = entry_key(entry);
        *out += strprintf("%.*s:", static_cast<int>(key->size), key->contents);
        *out += strprintf("[entry size=%d]", entry_size(sizer, entry));
        *out += strprintf("[value size=%d]", sizer->size(entry_value(entry)));
    } else if (entry_is_deletion(entry)) {
        const btree_key_t *key = entry_key(entry);
        *out += strprintf("%.*s:[deletion]", static_cast<int>(key->size), key->contents);
    } else if (entry_is_skip(entry)) {
        *out += strprintf("[skip %d]", entry_size(sizer, entry));
    } else {
        *out += strprintf("[code %d]", *reinterpret_cast<const uint8_t *>(entry));
    }
}


std::string strprint_leaf(value_sizer_t *sizer, const leaf_node_t *node) {
    std::string out;
    out += strprintf("Leaf(magic='%4.4s', num_pairs=%u, live_size=%u, frontmost=%u, tstamp_cutpoint=%u)\n",
            node->magic.bytes, node->num_pairs, node->live_size, node->frontmost, node->tstamp_cutpoint);

    out += strprintf("  Offsets:");
    for (int i = 0; i < node->num_pairs; ++i) {
        out += strprintf(" %d", node->pair_offsets[i]);
    }
    out += strprintf("\n");

    out += strprintf("  By Key:");
    for (int i = 0; i < node->num_pairs; ++i) {
        out += strprintf(" %d:", node->pair_offsets[i]);
        strprint_entry(&out, sizer, get_entry(node, node->pair_offsets[i]));
    }
    out += strprintf("\n");

    out += strprintf("  By Offset:");

    entry_iter_t iter = entry_iter_t::make(node);
    while (out += strprintf(" %d", iter.offset), !iter.done(sizer)) {
        out += strprintf(":");
        if (iter.offset < node->tstamp_cutpoint) {
            repli_timestamp_t tstamp = get_timestamp(node, iter.offset);
            out += strprintf("[t=%" PRIu64 "]", tstamp.longtime);
        }
        strprint_entry(&out, sizer, get_entry(node, iter.offset));
        iter.step(sizer, node);
    }
    out += strprintf("\n");

    return out;
}


void print_entry(FILE *fp, value_sizer_t *sizer, const entry_t *entry) {
    if (entry_is_live(entry)) {
        const btree_key_t *key = entry_key(entry);
        fprintf(fp, "%.*s:", static_cast<int>(key->size), key->contents);
        fprintf(fp, "[entry size=%d]", entry_size(sizer, entry));
        fprintf(fp, "[value size=%d]", sizer->size(entry_value(entry)));
    } else if (entry_is_deletion(entry)) {
        const btree_key_t *key = entry_key(entry);
        fprintf(fp, "%.*s:[deletion]", static_cast<int>(key->size), key->contents);
    } else if (entry_is_skip(entry)) {
        fprintf(fp, "[skip %d]", entry_size(sizer, entry));
    } else {
        fprintf(fp, "[code %d]", *reinterpret_cast<const uint8_t *>(entry));
    }
    fflush(fp);
}


void print(FILE *fp, value_sizer_t *sizer, const leaf_node_t *node) {
    fprintf(fp, "Leaf(magic='%4.4s', num_pairs=%u, live_size=%u, frontmost=%u, tstamp_cutpoint=%u)\n",
            node->magic.bytes, node->num_pairs, node->live_size, node->frontmost, node->tstamp_cutpoint);

    fprintf(fp, "  Offsets:");
    for (int i = 0; i < node->num_pairs; ++i) {
        fprintf(fp, " %d", node->pair_offsets[i]);
    }
    fprintf(fp, "\n");
    fflush(fp);

    fprintf(fp, "  By Key:");
    for (int i = 0; i < node->num_pairs; ++i) {
        fprintf(fp, " %d:", node->pair_offsets[i]);
        print_entry(fp, sizer, get_entry(node, node->pair_offsets[i]));
    }
    fprintf(fp, "\n");

    fprintf(fp, "  By Offset:");
    fflush(fp);

    entry_iter_t iter = entry_iter_t::make(node);
    while (fprintf(fp, " %d", iter.offset), fflush(fp), !iter.done(sizer)) {
        fprintf(fp, ":");
        fflush(fp);
        if (iter.offset < node->tstamp_cutpoint) {
            repli_timestamp_t tstamp = get_timestamp(node, iter.offset);
            fprintf(fp, "[t=%" PRIu64 "]", tstamp.longtime);
            fflush(fp);
        }
        print_entry(fp, sizer, get_entry(node, iter.offset));
        iter.step(sizer, node);
    }
    fprintf(fp, "\n");
    fflush(fp);
}


class do_nothing_fscker_t : public key_value_fscker_t {
    bool fsck(UNUSED value_sizer_t *sizer, UNUSED const btree_key_t *key,
              UNUSED const void *value, UNUSED std::string *msg_out) {
        return true;
    }
};


bool fsck(value_sizer_t *sizer, const btree_key_t *left_exclusive_or_null, const btree_key_t *right_inclusive_or_null, const leaf_node_t *node, key_value_fscker_t *fscker, std::string *msg_out) {

    struct {
        std::string *msg_out;
        bool operator()(bool test, const char *msg) {
            if (!test) *msg_out = msg;
            return !test;
        }
    } failed;
    failed.msg_out = msg_out;

    // We check that all offsets are contiguous (with interspersed
    // skip entries) between frontmost and block_size, that frontmost
    // is the smallest offset, that live_size is correct, that we have
    // correct magic, that the keys are in order, that there are no
    // deletion entries after tstamp_cutpoint, and that
    // tstamp_cutpoint lies on an entry boundary, and that frontmost
    // is not before the end of pair_offsets

    // Basic sanity checks on fields' values.
    if (failed(node->magic == sizer->btree_leaf_magic(),
               "bad leaf magic")
        || failed(node->frontmost >= offsetof(leaf_node_t, pair_offsets) + node->num_pairs * sizeof(uint16_t),
                  "frontmost offset is before the end of pair_offsets")
        || failed(node->live_size <= (sizer->block_size().value() - node->frontmost) + sizeof(uint16_t) * node->num_pairs,
                  "live_size is impossibly large")
        || failed(node->tstamp_cutpoint >= node->frontmost,
                  "timestamp cut offset below frontmost offset")
        || failed(node->tstamp_cutpoint <= sizer->block_size().value(),
                  "timestamp cut offset larger than block size")
        ) {
        return false;
    }

    // sizeof(offs) is guaranteed to be less than the block_size() thanks to assertions above.
    scoped_array_t<uint16_t> offs(node->num_pairs);
    memcpy(offs.data(), node->pair_offsets, node->num_pairs * sizeof(uint16_t));

    std::sort(offs.data(), offs.data() + node->num_pairs);

    if (failed(node->num_pairs == 0 || node->frontmost <= offs[0],
               "smallest pair offset is before frontmost offset")
        || failed(node->num_pairs == 0 || offs[node->num_pairs - 1] < sizer->block_size().value(),
                  "largest pair offset is larger than block size")
        ) {
        return false;
    }

    entry_iter_t iter = entry_iter_t::make(node);

    int observed_live_size = 0;

    int i = 0;
    bool seen_tstamp_cutpoint = false;
    static_assert(std::is_same<uint64_t, decltype(repli_timestamp_t::longtime)>::value,
                  "This code assumes repli_timestamp_t is a uint64_t.");
    uint64_t earliest_so_far = UINT64_MAX;
    while (!iter.done(sizer)) {
        int offset = iter.offset;

        // tstamp_cutpoint is supposed to be on some entry's offset.
        if (offset >= node->tstamp_cutpoint && !seen_tstamp_cutpoint) {
            if (failed(offset == node->tstamp_cutpoint, "misaligned tstamp_cutpoint")) {
                return false;
            }
            seen_tstamp_cutpoint = true;
        }

        if (failed(offset + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0) < sizer->block_size().value(),
                   "offset would be past block size after accounting for the timestamp")) {
            return false;
        }

        if (offset < node->tstamp_cutpoint) {
            repli_timestamp_t tstamp = get_timestamp(node, offset);
            if (tstamp.longtime > earliest_so_far) {
                *msg_out = strprintf("timestamps out of order (%" PRIu64 " after %" PRIu64 ")\n", tstamp.longtime, earliest_so_far);
                return false;
            }
            earliest_so_far = tstamp.longtime;
        }

        const entry_t *ent = get_entry(node, offset);
        if (entry_is_live(ent)) {
            const void *value = entry_value(ent);
            int space = sizer->block_size().value() - (reinterpret_cast<const char *>(value) - reinterpret_cast<const char *>(node));
            if (!sizer->fits(value, space)) {
                *msg_out = strprintf("problem with key %.*s: value does not fit\n", entry_key(ent)->size, entry_key(ent)->contents);
                return false;
            }

            std::string fscker_msg;
            if (!fscker->fsck(sizer, entry_key(ent), value, &fscker_msg)) {
                *msg_out = strprintf("Problem with key %.*s: %s\n", entry_key(ent)->size, entry_key(ent)->contents, fscker_msg.c_str());
                return false;
            }

            observed_live_size += sizeof(uint16_t) + entry_size(sizer, ent);
            if (failed(i < node->num_pairs, "missing entry offsets")) {
                return false;
            }
            if (offset != offs[i]) {
                *msg_out = strprintf("missing live entries or entry offsets (next "
                    "offset in offsets table is %d, but in node is %d)",
                    offs[i], offset);
                return false;
            }

            ++i;
        } else if (entry_is_deletion(ent)) {
            if (failed(!seen_tstamp_cutpoint, "deletion entry after tstamp_cutpoint")
                || failed(i < node->num_pairs, "missing entry offsets")
                || failed(offset == offs[i], "missing deletion entries or entry offsets")) {
                return false;
            }
            ++i;
        }

        // It's safe to step (because we just checked that the entry
        // is valid).
        iter.step(sizer, node);
    }

    if (failed(i == node->num_pairs, "missing entries")
        || failed(node->live_size == observed_live_size, "incorrect live_size recorded")) {
        return false;
    }

    // Entries look valid, check key ordering.

    const btree_key_t *last = left_exclusive_or_null;
    for (int k = 0; k < node->num_pairs; ++k) {
        const btree_key_t *key = entry_key(get_entry(node, node->pair_offsets[k]));
        if (failed(last == nullptr || btree_key_cmp(last, key) < 0,
                   "keys out of order")) {
            return false;
        }
        last = key;
    }

    if (failed(last == nullptr || right_inclusive_or_null == nullptr
               || btree_key_cmp(last, right_inclusive_or_null) <= 0,
               "keys out of order (with right_inclusive key)")) {
        return false;
    }

    return true;
}


void validate(DEBUG_VAR value_sizer_t *sizer, DEBUG_VAR const leaf_node_t *node) {
#ifndef NDEBUG
    do_nothing_fscker_t fits;
    std::string msg;
    bool fscked_successfully = fsck(sizer, nullptr, nullptr, node, &fits, &msg);
    rassert(fscked_successfully, "%s", msg.c_str());
#endif
}

void init(value_sizer_t *sizer, leaf_node_t *node) {
    node->magic = sizer->btree_leaf_magic();
    node->num_pairs = 0;
    node->live_size = 0;
    node->frontmost = sizer->block_size().value();
    node->tstamp_cutpoint = node->frontmost;
}

int free_space(value_sizer_t *sizer) {
    return sizer->block_size().value() - offsetof(leaf_node_t, pair_offsets);
}

// Returns the mandatory storage cost of the node, returning a value
// in the closed interval [0, free_space(sizer)].  Outputs the offset
// of the first entry for which storing a timestamp is not mandatory.
int mandatory_cost(value_sizer_t *sizer, const leaf_node_t *node, int required_timestamps, int *tstamp_back_offset_out) {
    int size = node->live_size;

    // node->live_size does not include deletion entries, deletion
    // entries' timestamps, and live entries' timestamps.  We add that
    // to size.

    entry_iter_t iter = entry_iter_t::make(node);
    int count = 0;
    int deletions_cost = 0;
    int max_deletions_cost = free_space(sizer) / DELETION_RESERVE_FRACTION;
    while (!(count == required_timestamps || iter.done(sizer) || iter.offset >= node->tstamp_cutpoint)) {
        const entry_t *ent = get_entry(node, iter.offset);
        if (entry_is_deletion(ent)) {
            if (deletions_cost >= max_deletions_cost) {
                break;
            }

            int this_entry_cost = sizeof(uint16_t) + sizeof(repli_timestamp_t) + entry_size(sizer, ent);
            deletions_cost += this_entry_cost;
            size += this_entry_cost;
            ++count;
        } else if (entry_is_live(ent)) {
            ++count;
            size += sizeof(repli_timestamp_t);
        }

        iter.step(sizer, node);
    }

    *tstamp_back_offset_out = iter.offset;

    return size;
}

int mandatory_cost(value_sizer_t *sizer, const leaf_node_t *node, int required_timestamps) {
    int ignored;
    return mandatory_cost(sizer, node, required_timestamps, &ignored);
}

int leaf_epsilon(value_sizer_t *sizer) {
    // Returns the maximum possible entry size, i.e. the key cost plus
    // the value cost plus pair_offsets plus timestamp cost.

    int key_cost = sizeof(uint8_t) + MAX_KEY_SIZE;

    // If the value is always empty, the DELETE_ENTRY_CODE byte needs to be considered.
    int n = std::max(sizer->max_possible_size(), 1);
    int pair_offsets_cost = sizeof(uint16_t);
    int timestamp_cost = sizeof(repli_timestamp_t);

    return key_cost + n + pair_offsets_cost + timestamp_cost;
}

bool is_empty(const leaf_node_t *node) {
    return node->num_pairs == 0;
}

bool is_full(value_sizer_t *sizer, const leaf_node_t *node, const btree_key_t *key, const void *value) {

    // Upon an insertion, we preserve `MANDATORY_TIMESTAMPS - 1`
    // timestamps and add our own (accounted for below)
    //
    // Notice this is the site of some bullshit XXX, the line below used to
    // consider that we would need to preserve MANDATORY_TIMESTAMPS - 1
    // timestamps. However this is incorrect because the key that we're
    // inserting may overwrite one of the timestamped keys. This means that a
    // key which mandatory cost had assumed was safe to garbage collect won't
    // be which allows us to get into a situation where is_full returns false
    // but when we call prepare_space_for_new_entry we fail with an insertion
    // because it doesn't actually fit.
    int size = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS);

    // Add the space we'll need for the new key/value pair we would
    // insert.  We conservatively assume the key is not already
    // contained in the node.

    size += sizeof(uint16_t) + sizeof(repli_timestamp_t) + key->full_size() + sizer->size(value);

    // The node is full if we can't fit all that data within the free space.
    return size > free_space(sizer);
}

bool is_underfull(value_sizer_t *sizer, const leaf_node_t *node) {

    // An underfull node is one whose mandatory fields' cost
    // constitutes significantly less than half the free space, where
    // "significantly" is enough to prevent a split-then-merge.

    // (Note that x / y is indeed taken to mean floor(x / y) below.)

    // A split node's size S is always within leaf_epsilon of
    // free_space and then is split as evenly as possible.  This means
    // the two resultant nodes' sizes are both no less than S / 2 -
    // leaf_epsilon / 2, which is no less than (free_space -
    // leaf_epsilon) / 2 - leaf_epsilon / 2.  Which is no less than is
    // free_space / 2 - leaf_epsilon.  We don't want an immediately
    // split node to be underfull, hence the threshold used below.

    return mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS) < free_space(sizer) / 2 - leaf_epsilon(sizer);
}


// Compares indices by looking at values in another array.
class indirect_index_comparator_t {
public:
    explicit indirect_index_comparator_t(const uint16_t *array) : array_(array) { }

    bool operator()(uint16_t x, uint16_t y) {
        return array_[x] < array_[y];
    }

private:
    const uint16_t *array_;
};


void garbage_collect(
        value_sizer_t *sizer,
        leaf_node_t *node,
        int num_tstamped,
        int *preserved_index,
        optional<int> tstamp_cutoff_upper_bound = optional<int>()) {
    scoped_array_t<uint16_t> indices(node->num_pairs);

    for (int i = 0; i < node->num_pairs; ++i) {
        indices[i] = i;
    }

    std::sort(indices.data(), indices.data() + node->num_pairs, indirect_index_comparator_t(node->pair_offsets));

    int mand_offset;
    UNUSED int cost = mandatory_cost(sizer, node, num_tstamped, &mand_offset);
    if (tstamp_cutoff_upper_bound) {
        /* If we have an upper bound for the timestamp cutoff point, make sure
        that we cut off at least that many timestamps. */
        mand_offset = std::min(*tstamp_cutoff_upper_bound, mand_offset);
    }

    int w = sizer->block_size().value();
    int i = node->num_pairs - 1;
    for (; i >= 0; --i) {
        int offset = node->pair_offsets[indices[i]];

        if (offset < mand_offset) {
            break;
        }

        entry_t *ent = get_entry(node, offset);
        if (entry_is_live(ent)) {
            int sz = entry_size(sizer, ent);
            w -= sz;
            memmove(get_at_offset(node, w), ent, sz);
            node->pair_offsets[indices[i]] = w;
        } else {
            node->pair_offsets[indices[i]] = 0;
        }
    }

    // Either i < 0 or node->pair_offsets[indices[i]] < mand_offset.

    node->tstamp_cutpoint = w;

    for (; i >= 0; --i) {
        int offset = node->pair_offsets[indices[i]];
        entry_t *ent = get_entry(node, offset);
        rassert(!entry_is_skip(ent));

        // Preserve the timestamp.
        int sz = sizeof(repli_timestamp_t) + entry_size(sizer, ent);

        w -= sz;

        memmove(get_at_offset(node, w), get_at_offset(node, offset), sz);
        node->pair_offsets[indices[i]] = w;
    }

    node->frontmost = w;

    // Now squash dead indices.
    int j = 0, k = 0;
    for (; k < node->num_pairs; ++k) {
        if (*preserved_index == k) {
            *preserved_index = j;
        }

        if (node->pair_offsets[k] != 0) {
            node->pair_offsets[j] = node->pair_offsets[k];

            j += 1;
        }
    }
    if (*preserved_index == node->num_pairs) {
        *preserved_index = j;
    }

    node->num_pairs = j;

    validate(sizer, node);
}


void garbage_collect(value_sizer_t *sizer, leaf_node_t *node, int num_tstamped) {
    int ignore = 0;
    garbage_collect(sizer, node, num_tstamped, &ignore);
    rassert(ignore == 0);
}

void clean_entry(void *p, int sz) {
    rassert(sz > 0);

    uint8_t *q = reinterpret_cast<uint8_t *>(p);
    if (sz == 1) {
        q[0] = SKIP_ENTRY_CODE_ONE;
    } else if (sz == 2) {
        q[0] = SKIP_ENTRY_CODE_TWO;
        q[1] = SKIP_ENTRY_RESERVED;
    } else if (sz > 2) {
        q[0] = SKIP_ENTRY_CODE_MANY;
        rassert(sz < 65536);
        reinterpret_cast<unaligned<uint16_t> *>(q + 1)->value = sz - 3;

        // Some  memset implementations are broken for nonzero values.
        for (int i = 3; i < sz; ++i) {
            q[i] = SKIP_ENTRY_RESERVED;
        }
    }
}

// Moves entries with pair_offsets indices in the clopen range [beg,
// end) from fro to tow.
void move_elements(value_sizer_t *sizer, leaf_node_t *fro, int beg, int end,
                   int wpoint, leaf_node_t *tow, int fro_copysize,
                   int fro_mand_offset,
                   std::vector<const void *> *moved_values_out) {
    rassert(is_underfull(sizer, tow));
    rassert(end >= beg);

    // This assertion is a bit loose.
    rassert(fro_copysize + mandatory_cost(sizer, tow, MANDATORY_TIMESTAMPS) <= free_space(sizer));

    // Make tow have a nice big region we can copy entries to.  Also,
    // this means we have no "skip" entries in tow.
    garbage_collect(sizer, tow, MANDATORY_TIMESTAMPS, &wpoint);

    // Now resize and move tow's pair_offsets.
    memmove(tow->pair_offsets + wpoint + (end - beg), tow->pair_offsets + wpoint, sizeof(uint16_t) * (tow->num_pairs - wpoint));

    tow->num_pairs += end - beg;

    // pos a

    // Now we're going to do something crazy.  Fill the new hole in
    // the pair offsets with the numbers in [0, end - beg).
    for (int i = 0; i < end - beg; ++i) {
        tow->pair_offsets[wpoint + i] = i;
    }

    // We treat these numbers as indices into [beg, end) in fro, and
    // sort them so that we can access [beg, end) in order by
    // increasing offset.
    std::sort(tow->pair_offsets + wpoint, tow->pair_offsets + wpoint + (end - beg), indirect_index_comparator_t(fro->pair_offsets + beg));

    int tow_offset = tow->frontmost;

    // The offset we read from (indirectly pointing to fro's [beg,
    // end)) in tow->pair_offsets, and the offset at which we stop.
    int fro_index = wpoint;
    int fro_index_end = wpoint + (end - beg);

    const int new_frontmost = tow->frontmost - fro_copysize;

    int wri_offset = new_frontmost;

    int adjustable_tow_offsets[MANDATORY_TIMESTAMPS];
    int num_adjustable_tow_offsets = 0;

    // To detect bugs in the code
    int actually_copied = 0;

    // We will gradually compute the live size.
    int livesize = tow->live_size;

    for (int i = 0; i < wpoint; ++i) {
        if (tow->pair_offsets[i] < tow->tstamp_cutpoint) {
            rassert(num_adjustable_tow_offsets < MANDATORY_TIMESTAMPS);
            adjustable_tow_offsets[num_adjustable_tow_offsets] = i;
            ++num_adjustable_tow_offsets;
        }
    }

    for (int i = wpoint + (end - beg); i < tow->num_pairs; ++i) {
        if (tow->pair_offsets[i] < tow->tstamp_cutpoint) {
            rassert(num_adjustable_tow_offsets < MANDATORY_TIMESTAMPS);
            adjustable_tow_offsets[num_adjustable_tow_offsets] = i;
            ++num_adjustable_tow_offsets;
        }
    }

    int fro_live_size_adjustment = 0;

    for (;;) {
        rassert(tow_offset <= tow->tstamp_cutpoint);
        if (tow_offset == tow->tstamp_cutpoint || fro_index == fro_index_end) {
            // We have no more timestamped information to push.
            break;
        }

        int fro_offset = fro->pair_offsets[beg + tow->pair_offsets[fro_index]];

        if (fro_offset >= fro_mand_offset) {
            // We have no more timestamped information to push.
            break;
        }

        repli_timestamp_t fro_tstamp = get_timestamp(fro, fro_offset);
        repli_timestamp_t tow_tstamp = get_timestamp(tow, tow_offset);

        // Greater timestamps go first.
        if (tow_tstamp < fro_tstamp) {
            entry_t *ent = get_entry(fro, fro_offset);
            int entsz = entry_size(sizer, ent);
            int sz = sizeof(repli_timestamp_t) + entsz;
            memmove(get_at_offset(tow, wri_offset), get_at_offset(fro, fro_offset), sz);

            if (entry_is_live(ent)) {
                livesize += entsz + sizeof(uint16_t);
                fro_live_size_adjustment -= entsz + sizeof(uint16_t);
            }

            clean_entry(ent, entsz);

            // Update the pair offset in fro to be the offset in tow
            // -- we'll never use the old value again and we'll copy
            // the newer values to tow later.
            fro->pair_offsets[beg + tow->pair_offsets[fro_index]] = wri_offset;

            wri_offset += sz;
            actually_copied += sz;
            fro_index++;

        } else {
            int sz = sizeof(repli_timestamp_t) + entry_size(sizer, get_entry(tow, tow_offset));
            memmove(get_at_offset(tow, wri_offset), get_at_offset(tow, tow_offset), sz);

            // Update the pair offset of the entry we've moved.
            int i;
            for (i = 0; i < num_adjustable_tow_offsets; ++i) {
                int j = adjustable_tow_offsets[i];
                if (tow->pair_offsets[j] == tow_offset) {
                    tow->pair_offsets[j] = wri_offset;
                    break;
                }
            }

            // Make sure we updated something.
            rassert(i != num_adjustable_tow_offsets);

            wri_offset += sz;
            tow_offset += sz;
        }
    }

    uint16_t new_tstamp_cutpoint = wri_offset;

    // Now we have some untimestamped entries to write.
    for (; fro_index < fro_index_end; ++fro_index) {
        int fro_offset = fro->pair_offsets[beg + tow->pair_offsets[fro_index]];
        entry_t *ent = get_entry(fro, fro_offset);
        if (entry_is_live(ent)) {
            int sz = entry_size(sizer, ent);
            memmove(get_at_offset(tow, wri_offset), ent, sz);
            clean_entry(ent, sz);
            fro_live_size_adjustment -= sz + sizeof(uint16_t);

            fro->pair_offsets[beg + tow->pair_offsets[fro_index]] = wri_offset;

            wri_offset += sz;
            livesize += sz + sizeof(uint16_t);
            actually_copied += sz;
            rassert(wri_offset <= tow_offset);
        } else {
            rassert(entry_is_deletion(ent));

            // This is a dead entry.  We'll need to squash this dead entry later.
            fro->pair_offsets[beg + tow->pair_offsets[fro_index]] = 0;

            int sz = entry_size(sizer, ent);
            clean_entry(ent, sz);
        }
    }

    guarantee(actually_copied <= fro_copysize);
    rassert(wri_offset <= tow_offset);

    // tow may have some timestamped entries that need to become
    // un-timestamped.
    while (tow_offset < tow->tstamp_cutpoint) {
        rassert(wri_offset <= tow_offset);

        entry_t *ent = get_entry(tow, tow_offset);
        int sz = entry_size(sizer, ent);
        if (entry_is_live(ent)) {
            memmove(get_at_offset(tow, wri_offset), ent, sz);

            // Update the pair offset of the entry we've moved.
            int i;
            for (i = 0; i < num_adjustable_tow_offsets; ++i) {
                int j = adjustable_tow_offsets[i];
                if (tow->pair_offsets[j] == tow_offset) {
                    tow->pair_offsets[j] = wri_offset;
                    break;
                }
            }

            // Make sure we updated something.
            rassert(i != num_adjustable_tow_offsets);

            wri_offset += sz;
        } else {

            // Update the pair offset of the entry we've deleted, so
            // that it gets squashed later.
            int i;
            for (i = 0; i < num_adjustable_tow_offsets; ++i) {
                int j = adjustable_tow_offsets[i];
                if (tow->pair_offsets[j] == tow_offset) {
                    tow->pair_offsets[j] = 0;
                }
            }
        }
        tow_offset += sizeof(repli_timestamp_t) + sz;
    }

    guarantee(wri_offset <= tow_offset);

    // If we needed to untimestamp any tow entries, we'll need a skip
    // entry for the open space.
    if (wri_offset < tow_offset) {
        // printf("wri_offset = %d, tow_offset = %d\n", wri_offset, tow_offset);
        clean_entry(get_at_offset(tow, wri_offset), tow_offset - wri_offset);
    }

    // We don't need to do anything else for tow entries because we
    // did a garbage collection in tow so they are contiguous through
    // the end of the buffer (and the rest don't have timestamps).

    // Copy the valid tow offsets from [beg, end) to the wpoint point
    // in tow, and move fro entries.
    memcpy(tow->pair_offsets + wpoint, fro->pair_offsets + beg,
           sizeof(uint16_t) * (end - beg));
    memmove(fro->pair_offsets + beg, fro->pair_offsets + end, sizeof(uint16_t) * (fro->num_pairs - end));
    fro->num_pairs -= end - beg;

    tow->frontmost = new_frontmost;

    tow->live_size = livesize;

    tow->tstamp_cutpoint = new_tstamp_cutpoint;

    fro->live_size += fro_live_size_adjustment;

    if (moved_values_out != nullptr) {
        // Collect value pointers of the moved values
        moved_values_out->clear();
        moved_values_out->reserve(end - beg);
        for (int pair_idx = wpoint; pair_idx < wpoint + (end - beg); ++pair_idx) {
            const int offset = tow->pair_offsets[pair_idx];
            // Skip dead entries
            if (offset != 0) {
                const entry_t *entry = get_entry(tow, offset);
                // Skip deletions
                if (entry_is_live(entry)) {
                    moved_values_out->push_back(entry_value(entry));
                }
            }
        }
    }

    {
        // Squash dead entries that we copied pair_offsets from fro,
        // for, and that we removed from tow, as well.
        int j, k;
        for (j = 0, k = 0; k < tow->num_pairs; ++k) {
            if (tow->pair_offsets[k] != 0) {
                tow->pair_offsets[j] = tow->pair_offsets[k];

                j += 1;
            }
        }
        tow->num_pairs = j;
    }

    validate(sizer, fro);
    validate(sizer, tow);
}

void split(value_sizer_t *sizer, leaf_node_t *node, leaf_node_t *rnode, btree_key_t *median_out) {
    int tstamp_back_offset;
    int mandatory = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS, &tstamp_back_offset);

    guarantee(mandatory >= free_space(sizer) - leaf_epsilon(sizer));

    // We shall split the mandatory cost of this node as evenly as possible.

    int num_mandatories = 0;
    int i = node->num_pairs - 1;
    int prev_rcost = 0;
    int rcost = 0;
    while (i >= 0 && rcost < mandatory / 2) {
        int offset = node->pair_offsets[i];
        entry_t *ent = get_entry(node, offset);

        // We only take mandatory entries' costs into consideration,
        // which guarantees correct behavior (in that neither node nor
        // can become underfull after a split).  If we didn't do this,
        // it would be possible to bias one node with a bunch of
        // deletions that makes its mandatory_cost artificially small.

        if (entry_is_live(ent)) {
            prev_rcost = rcost;
            rcost += entry_size(sizer, ent) + sizeof(uint16_t) + (offset < tstamp_back_offset ? sizeof(repli_timestamp_t) : 0);

            ++num_mandatories;
        } else {
            rassert(entry_is_deletion(ent));

            if (offset < tstamp_back_offset) {
                prev_rcost = rcost;
                rcost += entry_size(sizer, ent) + sizeof(uint16_t) + sizeof(repli_timestamp_t);

                ++num_mandatories;
            }
        }

        --i;
    }

    // Since the mandatory_cost is at least free_space - leaf_epsilon there's no way i can equal num_pairs or zero.
    guarantee(i < node->num_pairs);
    guarantee(i > 0);

    // Now prev_rcost and rcost envelope mandatory / 2.
    guarantee(prev_rcost < mandatory / 2);
    guarantee(rcost >= mandatory / 2, "rcost = %d, mandatory / 2 = %d, i = %d", rcost, mandatory / 2, i);

    int s;
    int end_rcost;
    if ((mandatory - prev_rcost) - prev_rcost < rcost - (mandatory - rcost)) {
        end_rcost = prev_rcost;
        s = i + 2;
        --num_mandatories;
    } else {
        end_rcost = rcost;
        s = i + 1;
    }

    // If our math was right, neither node can be underfull just
    // considering the split of the mandatory costs.
    guarantee(end_rcost >= free_space(sizer) / 2 - leaf_epsilon(sizer));
    guarantee(mandatory - end_rcost >= free_space(sizer) / 2 - leaf_epsilon(sizer));

    // Now we wish to move the elements at indices [s, num_pairs) to rnode.

    init(sizer, rnode);

    int node_copysize = end_rcost - num_mandatories * sizeof(uint16_t);
    move_elements(sizer, node, s, node->num_pairs, 0, rnode, node_copysize,
                  tstamp_back_offset, nullptr);

    keycpy(median_out, entry_key(get_entry(node, node->pair_offsets[s - 1])));
}

void merge(value_sizer_t *sizer, leaf_node_t *left, leaf_node_t *right) {
    rassert(left != right);

    rassert(is_underfull(sizer, left));
    rassert(is_underfull(sizer, right));

    int tstamp_back_offset;
    int mandatory = mandatory_cost(sizer, left, MANDATORY_TIMESTAMPS, &tstamp_back_offset);

    int left_copysize = mandatory;
    // Uncount the uint16_t cost of mandatory entries.  Sigh.
    // This includes deletion entries *before* the `tstamp_back_offset`, as well
    // as all non-deletion entries.
    for (int i = 0; i < left->num_pairs; ++i) {
        if (left->pair_offsets[i] < tstamp_back_offset
            || !entry_is_deletion(get_entry(left, left->pair_offsets[i]))) {
            left_copysize -= sizeof(uint16_t);
        }
    }

    move_elements(sizer, left, 0, left->num_pairs, 0, right, left_copysize,
                  tstamp_back_offset, nullptr);
}

// We move keys out of sibling and into node.
bool level(value_sizer_t *sizer, int nodecmp_node_with_sib,
           leaf_node_t *node, leaf_node_t *sibling,
           btree_key_t *replacement_key_out,
           std::vector<const void *> *moved_values_out) {
    rassert(node != sibling);

    // If sibling were underfull, we'd just merge the nodes.
    rassert(is_underfull(sizer, node));
    rassert(!is_underfull(sizer, sibling));

    // First figure out the inclusive range [beg, end] of elements we want to move
    // from sibling.
    int beg, end, *w, wstep;

    int node_weight = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS);
    int tstamp_back_offset;
    int sibling_weight = mandatory_cost(sizer, sibling, MANDATORY_TIMESTAMPS,
                                        &tstamp_back_offset);

    guarantee(node_weight < sibling_weight);

    if (nodecmp_node_with_sib < 0) {
        // node is to the left of sibling, so we want to move elements
        // [0, k) from sibling.

        beg = 0;
        end = 0;
        w = &end;
        wstep = 1;
    } else {
        // node is to the right of sibling, so we want to move
        // elements [num_pairs - k, num_pairs) from sibling.

        beg = sibling->num_pairs - 1;
        end = sibling->num_pairs - 1;
        w = &beg;
        wstep = -1;
    }

    guarantee(end - beg != sibling->num_pairs - 1);

    int prev_weight_movement = 0;
    int weight_movement = 0;
    int num_mandatories = 0;
    int prev_diff = sizer->block_size().value();  // some impossibly large value
    for (;;) {
        int offset = sibling->pair_offsets[*w];
        entry_t *ent = get_entry(sibling, offset);

        // We only take mandatory entries' costs into consideration.
        if (entry_is_live(ent)) {
            int sz = entry_size(sizer, ent) + sizeof(uint16_t) + (offset < tstamp_back_offset ? sizeof(repli_timestamp_t) : 0);
            prev_diff = sibling_weight - node_weight;
            prev_weight_movement = weight_movement;
            weight_movement += sz;
            node_weight += sz;
            sibling_weight -= sz;

            ++num_mandatories;
        } else {
            rassert(entry_is_deletion(ent));

            if (offset < tstamp_back_offset) {
                int sz = entry_size(sizer, ent) + sizeof(uint16_t) + sizeof(repli_timestamp_t);
                prev_diff = sibling_weight - node_weight;
                prev_weight_movement = weight_movement;
                weight_movement += sz;
                node_weight += sz;
                sibling_weight -= sz;

                ++num_mandatories;
            }
        }

        if (end - beg == sibling->num_pairs - 1 || node_weight >= sibling_weight) {
            break;
        }

        *w += wstep;
    }

    guarantee(end - beg < sibling->num_pairs - 1);

    if (prev_diff <= sibling_weight - node_weight) {
        *w -= wstep;
        --num_mandatories;
        weight_movement = prev_weight_movement;
    }

    if (end < beg) {
        // Alas, there is no actual leveling to do.
        guarantee(end + 1 == beg);
        return false;
    }

    int sib_copysize = weight_movement - num_mandatories * sizeof(uint16_t);
    move_elements(sizer, sibling, beg, end + 1,
                  nodecmp_node_with_sib < 0 ? node->num_pairs : 0, node,
                  sib_copysize, tstamp_back_offset, moved_values_out);

    guarantee(node->num_pairs > 0);
    guarantee(sibling->num_pairs > 0);

    if (nodecmp_node_with_sib < 0) {
        keycpy(replacement_key_out, entry_key(get_entry(node, node->pair_offsets[node->num_pairs - 1])));
    } else {
        keycpy(replacement_key_out, entry_key(get_entry(sibling, sibling->pair_offsets[sibling->num_pairs - 1])));
    }

    return true;
}

bool is_mergable(value_sizer_t *sizer, const leaf_node_t *node, const leaf_node_t *sibling) {
    return is_underfull(sizer, node) && is_underfull(sizer, sibling);
}

// Sets *index_out to the index for the live entry or deletion entry
// for the key, or to the index the key would have if it were
// inserted.  Returns true if the key at said index is actually equal.
bool find_key(const leaf_node_t *node, const btree_key_t *key, int *index_out) {
    int beg = 0;
    int end = node->num_pairs;

    // beg == 0 or key > *(beg - 1).
    // end == num_pairs or key < *end.

    while (beg < end) {
        // when (end - beg) > 0, (end - beg) / 2 is always less than (end - beg).  So beg <= test_point < end.
        int test_point = beg + (end - beg) / 2;

        const btree_key_t *ek = entry_key(get_entry(node, node->pair_offsets[test_point]));

        int res = btree_key_cmp(key, ek);

        if (res < 0) {
            // key < *test_point.
            end = test_point;
        } else if (res > 0) {
            // key > *test_point.  Since test_point < end, we have test_point + 1 <= end.
            beg = test_point + 1;
        } else {
            // We found the key!
            *index_out = test_point;
            return true;
        }
    }

    // (Since beg == end, then *(beg - 1) < key < *beg (with appropriate
    // provisions for beg == 0 or beg == num_pairs) and index_out
    // should be set to beg, and false should be returned.
    *index_out = beg;
    return false;
}

bool lookup(value_sizer_t *sizer, const leaf_node_t *node, const btree_key_t *key, void *value_out) {
    int index;
    if (find_key(node, key, &index)) {
        const entry_t *ent = get_entry(node, node->pair_offsets[index]);
        if (entry_is_live(ent)) {
            const void *val = entry_value(ent);
            memcpy(value_out, val, sizer->size(val));
            return true;
        }
    }

    return false;
}

/* `insert()` and `remove()` call this to insert a new entry into the leaf node.

First it removes any existing entry for `key`; then it makes room in the leaf
node for a new entry and fills `*space_out` with a pointer to the beginning of
where the new entry should go. `prepare_space_for_new_entry()` takes care of
updating `pair_offsets` and `num_pairs`, moving around other entries as needed
to maintain timestamp ordering, putting a timestamp on the new entry if
appropriate, and maintaining `frontmost` and `tstamp_cutpoint`. The caller is
responsible for writing the actual entry itself (including the key) and for
updating `live_size` if the newly created entry is live.

`new_entry_size` is the total size of the new entry, including key, value,
and/or code byte, but not including repli timestamp.

It is an error to put a deletion entry after `tstamp_cutpoint`. If the caller
intends to insert a deletion entry, it should pass `false` for
`allow_after_tstamp_cutpoint`. If the entry would go after `tstamp_cutpoint`
and `allow_after_tstamp_cutpoint` is false, then instead of making room for the
entry, `prepare_space_for_new_entry()` will return false. It will still remove
any preexisting entry that was in the leaf node. If the entry would go before
`tstamp_cutpoint` or `allow_after_tstamp_cutpoint` is true, then the return
value will be true. */
MUST_USE bool prepare_space_for_new_entry(
        value_sizer_t *sizer,
        leaf_node_t *node,
        const btree_key_t *key,
        int new_entry_size,
        repli_timestamp_t tstamp,
        /* Used to derive the highest possible timestamp that non-timestamped
        entries might have. Usually the recency of the buf_t that node is in. */
        repli_timestamp_t maximum_existing_tstamp,
        bool allow_after_tstamp_cutpoint,
        char **space_out) {

    /* Get an upper bound for the timestamp cutoff point in case we later call
    `garbage_collect()`. We want to make sure that garbage collection cleans up
    *at least* as much space as it would clean up now. This is crucial, because
    our caller (in particular `insert()`) might have used `is_full()` in order to
    check that the leaf node has enough space for a given entry.
    If the key already exists, we are going to call `clear_entry()` on it below,
    before we call `garbage_collect()`. However this can have the unwanted side
    effect that `mandatory_cost` in `garbage_collect()` no longer exceeds the
    `max_deletions_cost`, or exceeds it only at a later offset
    (this is true in particular if the existing entry is a deletion entry).
    As a consequence of that, `garbage_collect()` might clean up slightly less space
    than was originally assumed.
    We avoid this problem by getting the cutoff point now, and having `garbage_collect()`
    pick the lower of the two. */
    int gc_tstamp_cutoff_upper_bound;
    mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS - 1, &gc_tstamp_cutoff_upper_bound);

    /* Figure out where in `pair_offsets` to put the offset of the new entry,
    and simultaneously check for an existing entry for this key. If the entry
    already exists, clean it. */

    int index;
    bool found = find_key(node, key, &index);

    if (found) {
        int offset = node->pair_offsets[index];
        entry_t *ent = get_entry(node, offset);

        int sz = entry_size(sizer, ent);

        if (entry_is_live(ent)) {
            node->live_size -= sizeof(uint16_t) + sz;
        }

        clean_entry(ent, sz);

        /* We'll re-use the now open slot in `pair_offsets`. If it turns out
        that we aren't actually creating a new entry, like if we're deleting a
        key which has no timestamp, then we'll close up this space in
        `pair_offsets` later. */

    } else {
        /* Later, once we've confirmed that we actually want to create an entry
        in the leaf node, we'll open up some space in `pair_offsets`. */
    }

    /* Garbage collect if appropriate.
    We do it after cleaning up any existing entry so that adding a deletion entry
    *almost* always works no matter how full the node is.
    If the existing entry is smaller than the new deletion entry that we're trying
    to add, we might not have enough space for it even after garbage collection.
    This can happen if the previous entry didn't have a timestamp and had a very
    small value size (in practice this should be really rare).
    We check for this condition further down, and recover from it by dropping
    all existing timestamps and discarding the delete entry by returning `false`. */

    if (offsetof(leaf_node_t, pair_offsets) +
            sizeof(uint16_t) * (node->num_pairs + (found ? 0 : 1)) +
            sizeof(repli_timestamp_t) +
            new_entry_size >
            node->frontmost) {

        if (found) {
            /* We can't re-use an existing index if we're garbage collecting. */
            found = false;
            memmove(
                node->pair_offsets + index,
                node->pair_offsets + index + 1,
                sizeof(uint16_t) * (node->num_pairs - index - 1));
            --node->num_pairs;
        }

        /* Passing `&index` as a parameter to `garbage_collect()`
        guarantees that it will remain valid even as `pair_offsets` entries are
        moved around. */
        garbage_collect(sizer, node, MANDATORY_TIMESTAMPS - 1, &index,
                        make_optional(gc_tstamp_cutoff_upper_bound));

        /* Make sure that `index` still refers to where the new key should be
        inserted. */
        DEBUG_VAR int index2;
        rassert(!find_key(node, key, &index2));
        rassert(index == index2, "garbage_collect() failed to preserve index");
    }

    /* Compute where in the node to put the new entry */

    uint16_t end_of_where_new_entry_should_go;
    bool new_entry_should_have_timestamp;

    if (node->frontmost == sizer->block_size().value() ||
            (node->frontmost < node->tstamp_cutpoint && get_timestamp(node, node->frontmost) <= tstamp)) {
        /* In the most common case, the new value will go right at
        `node->frontmost` and will get a timestamp. For performance reasons, we
        check for this case specially and short-circuit. If a cosmic ray were to
        strike the computer and make us take the second branch of this `if` when
        we should have taken the first branch, the result should be the same. */
        end_of_where_new_entry_should_go = node->frontmost;
        new_entry_should_have_timestamp = true;

    } else {
        entry_iter_t iter = entry_iter_t::make(node);
        while (!iter.done(sizer) && iter.offset < node->tstamp_cutpoint && get_timestamp(node, iter.offset) > tstamp) {
            iter.step(sizer, node);
        }
        end_of_where_new_entry_should_go = iter.offset;

        if (end_of_where_new_entry_should_go == node->tstamp_cutpoint &&
                node->tstamp_cutpoint != sizer->block_size().value()) {
            /* We are after all of the timestamped entries, but before at least
            one non-timestamped entry. We know that the non-timestamped entries
            have a timestamp of at most maximum_existing_tstamp. If our own timestamp
            is higher than that, we can put a timestamp on ourself. Otherwise, we
            don't know what the timestamp would have been on the non-timestamped entry,
            so we mustn't put a timestamp on ourself. */
            new_entry_should_have_timestamp = tstamp > maximum_existing_tstamp;
        } else {
            new_entry_should_have_timestamp = true;
        }
    }

    /* If a deletion would go after `tstamp_cutpoint`, instead we want to just
    discard it entirely (deletions have `allow_after_tstamp_cutpoint == false`). */
    bool actually_create_entry = new_entry_should_have_timestamp || allow_after_tstamp_cutpoint;

    /* As mentioned above, it can theoretically happen that we're trying to replace
    an entry by a deletion entry and we don't have enough space to write the deletion
    entry.
    We check for this here, and shift the timestamp cutoff point in order to
    maintain correctness.
    This will be very rare in practice, since values are usually larger than
    deletion entries. */
    bool drop_timestamps = false;
    if (actually_create_entry
        && !allow_after_tstamp_cutpoint
        && offsetof(leaf_node_t, pair_offsets)
           + sizeof(uint16_t) * (node->num_pairs + (found ? 0 : 1))
           + new_entry_size
           + sizeof(repli_timestamp_t)
           > node->frontmost) {
        actually_create_entry = false;
        drop_timestamps = true;
    }

    if (!actually_create_entry) {
        if (found) {
            /* We're deleting the previous entry for this key, but not inserting
            a new one; close the gap in `pair_offsets`. `index` is the location
            of the open slot. */
            memmove(
                node->pair_offsets + index,
                node->pair_offsets + index + 1,
                sizeof(uint16_t) * (node->num_pairs - index - 1));
            --node->num_pairs;
        }

        if (drop_timestamps) {
            erase_deletions(sizer, node, optional<repli_timestamp_t>());
        }

        return false;
    }

    /* There was no previous entry for this key, but we're creating a new entry;
    make some room in `pair_offsets`. `index` is where the open slot should go.
    We didn't do this before because we weren't sure if we were actually gonna
    create a new entry or not. */

    if (!found) {
        memmove(
            node->pair_offsets + index + 1,
            node->pair_offsets + index,
            sizeof(uint16_t) * (node->num_pairs - index));
        ++node->num_pairs;
    }

    /* Now that we know where in the leaf node to write our entry, make space if
    necessary. */

    int total_space_for_new_entry = new_entry_size + (new_entry_should_have_timestamp ? sizeof(repli_timestamp_t) : 0);

    if (end_of_where_new_entry_should_go == node->frontmost) {
        /* This is the common case. Just like before, we check for this case
        specially and short-circuit, even though the algorithm in the `else`
        branch is a no-op if `end_of_where_new_entry_should_go` is
        `node->frontmost`. */
    } else {
        /* Move the entries themselves */
        memmove(
            get_at_offset(node, node->frontmost - total_space_for_new_entry),
            get_at_offset(node, node->frontmost),
            end_of_where_new_entry_should_go - node->frontmost);
        /* Update the `pair_offsets` table so it points to the new locations of
        the entries */
        for (int i = 0; i < node->num_pairs; ++i) {
            if (i == index) continue;
            if (node->pair_offsets[i] < end_of_where_new_entry_should_go) {
                node->pair_offsets[i] -= total_space_for_new_entry;
            }
        }
    }

    node->frontmost -= total_space_for_new_entry;
    guarantee(offsetof(leaf_node_t, pair_offsets)
              + sizeof(uint16_t) * node->num_pairs <= node->frontmost);

    /* Write the timestamp if we need one, and update `node->tstamp_cutpoint` if
    we don't. */

    uint16_t start_of_where_new_entry_should_go = end_of_where_new_entry_should_go - total_space_for_new_entry;
    if (new_entry_should_have_timestamp) {
        reinterpret_cast<unaligned<repli_timestamp_t> *>(get_at_offset(node, start_of_where_new_entry_should_go))->value = tstamp;
    } else {
        rassert(end_of_where_new_entry_should_go == node->tstamp_cutpoint);
        node->tstamp_cutpoint = start_of_where_new_entry_should_go;
    }

    /* Record the offset in `pair_offsets` */

    node->pair_offsets[index] = start_of_where_new_entry_should_go;

    /* Fill output variable */

    if (new_entry_should_have_timestamp) {
        *space_out = get_at_offset(node, start_of_where_new_entry_should_go + sizeof(repli_timestamp_t));
    } else {
        *space_out = get_at_offset(node, start_of_where_new_entry_should_go);
    }
    guarantee(end_of_where_new_entry_should_go <= sizer->block_size().value());

    return true;
}

// Inserts a key/value pair into the node.  Hopefully you've already
// cleaned up the old value, if there is one.
void insert(
        value_sizer_t *sizer,
        leaf_node_t *node,
        const btree_key_t *key,
        const void *value,
        repli_timestamp_t tstamp,
        repli_timestamp_t maximum_existing_tstamp) {
    rassert(!is_full(sizer, node, key, value));

    /* Make space for the entry itself */

    char *location_to_write_data;
    bool should_write = prepare_space_for_new_entry(sizer, node,
        key, key->full_size() + sizer->size(value), tstamp, maximum_existing_tstamp,
        true,
        &location_to_write_data);
    guarantee(should_write);

    /* Now copy the data into the node itself */

    memcpy(location_to_write_data, key, key->full_size());
    location_to_write_data += key->full_size();
    memcpy(location_to_write_data, value, sizer->size(value));

    node->live_size += sizeof(uint16_t) + key->full_size() + sizer->size(value);

    validate(sizer, node);
}

void remove(
        value_sizer_t *sizer,
        leaf_node_t *node,
        const btree_key_t *key,
        repli_timestamp_t tstamp,
        repli_timestamp_t maximum_existing_tstamp) {
    /* If the deletion entry would fall after `tstamp_cutpoint`, then it
    shouldn't be written at all. If that's the case, then
    `prepare_space_for_new_entry()` will return false because we pass false for
    `allow_after_tstamp_cutpoint`. */

    char *location_to_write_data;
    if (prepare_space_for_new_entry(sizer, node,
            key,
            1 + key->full_size(),   /* 1 for `DELETE_ENTRY_CODE` */
            tstamp,
            maximum_existing_tstamp,
            false,
            &location_to_write_data)) {
        *location_to_write_data = static_cast<char>(DELETE_ENTRY_CODE);
        ++location_to_write_data;
        memcpy(location_to_write_data, key, key->full_size());
    }

    validate(sizer, node);
}

// Erases the entry for the given key, leaving behind no trace.
void erase_presence(value_sizer_t *sizer, leaf_node_t *node, const btree_key_t *key) {
    int index;
    bool found = find_key(node, key, &index);
    if (found) {
        int offset = node->pair_offsets[index];
        entry_t *ent = get_entry(node, offset);

        int sz = entry_size(sizer, ent);
        if (entry_is_live(ent)) {
            node->live_size -= sizeof(uint16_t) + sz;
        }

        clean_entry(ent, sz);

        memmove(node->pair_offsets + index, node->pair_offsets + index + 1, (node->num_pairs - (index + 1)) * sizeof(uint16_t));
        node->num_pairs -= 1;
    }

    validate(sizer, node);
}

repli_timestamp_t min_deletion_timestamp(
        value_sizer_t *sizer,
        const leaf_node_t *node,
        repli_timestamp_t maximum_existing_timestamp) {
    repli_timestamp_t earliest_so_far = maximum_existing_timestamp;
    entry_iter_t iter = entry_iter_t::make(node);
    while (!iter.done(sizer) && iter.offset < node->tstamp_cutpoint) {
        repli_timestamp_t tstamp = get_timestamp(node, iter.offset);
        rassert(earliest_so_far >= tstamp,
            "asserted earliest_so_far (%" PRIu64 ") >= tstamp (%" PRIu64 ")",
            earliest_so_far.longtime, tstamp.longtime);
        earliest_so_far = tstamp;
        iter.step(sizer, node);
    }
    /* It's possible for us to forget a deletion with timestamp equal to the oldest
    timestamped entry, so the min deletion timestamp is one time unit newer than the
    oldest timestamped entry. */
    return earliest_so_far.next();
}

void erase_deletions(
        value_sizer_t *sizer, leaf_node_t *node,
        optional<repli_timestamp_t> min_del_timestamp) {
    int old_tstamp_cutpoint = node->tstamp_cutpoint;
    entry_iter_t iter = entry_iter_t::make(node);

    if (min_del_timestamp.has_value()) {
        /* Advance `iter` to the first entry with a timestamp that's lower than
        `min_del_timestamp - 1`. */
        while (true) {
            if (iter.done(sizer) || iter.offset >= old_tstamp_cutpoint) {
                return;
            }
            if (get_timestamp(node, iter.offset).next() < *min_del_timestamp) {
                break;
            }
            iter.step(sizer, node);
        }
    }

    /* We'll remove timestamps from all of the entries between `old_tstamp_cutpoint` and
    `new_tstamp_cutpoint`. But we won't update `leaf->tstamp_cutpoint` until the end of
    the function because we want `iter` to continue iterating according to the old value.
    */
    int new_tstamp_cutpoint = iter.offset;

    /* Step until we reach `old_tstamp_cutpoint`, erasing timestamps and deletions as we
    go. Make a note of each deletion's offset so we can remove them from the
    `pair_offsets` array later. */
    std::set<int> deletion_offsets;
    while (!iter.done(sizer) && iter.offset != old_tstamp_cutpoint) {
        int off = iter.offset;
        guarantee(off >= new_tstamp_cutpoint && off < old_tstamp_cutpoint);
        const entry_t *ent = get_entry(node, off);
        /* Move `iter` before we modify `ent`, to avoid confusion */
        iter.step(sizer, node);
        if (entry_is_deletion(ent)) {
            clean_entry(
                get_at_offset(node, off),
                sizeof(repli_timestamp_t) + entry_size(sizer, ent));
            deletion_offsets.insert(off);
        } else {
            /* This is the code path for both skip entries and live entries, because skip
            entries have timestamps too. */
            clean_entry(get_at_offset(node, off), sizeof(repli_timestamp_t));
        }
    }

    /* If a pointer in `pair_offsets` was pointing to a timestamp that's been erased on
    a key-value pair, we need to move it forward by `sizeof(repli_timestamp_t)`. If it
    was pointing to a deletion entry that's been erased, we need to remove that entry
    from `pair_offsets`. */
    int src = 0, dst = 0;
    int num_deleted = deletion_offsets.size();
    for (; src < node->num_pairs; ++src) {
        uint16_t off = node->pair_offsets[src];
        auto it = deletion_offsets.find(off);
        if (it == deletion_offsets.end()) {
            if (off >= new_tstamp_cutpoint && off < old_tstamp_cutpoint) {
                off += sizeof(repli_timestamp_t);
            }
            node->pair_offsets[dst++] = off;
        } else {
            guarantee(off >= new_tstamp_cutpoint && off < old_tstamp_cutpoint);
            deletion_offsets.erase(it);
        }
    }
    guarantee(deletion_offsets.empty());
    node->num_pairs -= num_deleted;

    /* Finally, update `node->tstamp_cutpoint` */
    node->tstamp_cutpoint = new_tstamp_cutpoint;
}

/* Calls `cb` on every entry in the node, whether a real entry or a deletion. The calls
will be in order from most recent to least recent. For entries with no timestamp, the
callback will get `min_deletion_timestamp() - 1`. */
continue_bool_t visit_entries(
        value_sizer_t *sizer,
        const leaf_node_t *node,
        repli_timestamp_t maximum_existing_timestamp,
        const std::function<continue_bool_t(
            const btree_key_t *key,
            repli_timestamp_t timestamp,
            const void *value   /* null for deletion */
            )> &cb) {
    repli_timestamp_t earliest_so_far = maximum_existing_timestamp;
    for (entry_iter_t iter = entry_iter_t::make(node);
            !iter.done(sizer); iter.step(sizer, node)) {
        repli_timestamp_t tstamp;
        if (iter.offset < node->tstamp_cutpoint) {
            tstamp = get_timestamp(node, iter.offset);
        } else {
            tstamp = earliest_so_far;
        }
        earliest_so_far = tstamp;

        const entry_t *ent = get_entry(node, iter.offset);

        if (entry_is_skip(ent)) {
            continue;
        }

        if (continue_bool_t::ABORT == cb(entry_key(ent), tstamp, entry_value(ent))) {
            return continue_bool_t::ABORT;
        }
    }
    return continue_bool_t::CONTINUE;
}

iterator::iterator()
    : node_(nullptr), index_(-1) { }

iterator::iterator(const leaf_node_t *node, int index)
    : node_(node), index_(index) { }

std::pair<const btree_key_t *, const void *> iterator::operator*() const {
    guarantee(index_ < static_cast<int>(node_->num_pairs));
    guarantee(index_ >= 0);
    const entry_t *entree = get_entry(node_, node_->pair_offsets[index_]);
    return std::make_pair(entry_key(entree), entry_value(entree));
}

iterator &iterator::operator++() {
    guarantee(index_ < static_cast<int>(node_->num_pairs),
              "Trying to increment past the end of an iterator.");
    do {
        ++index_;
    } while (index_ < node_->num_pairs && !entry_is_live(get_entry(node_, node_->pair_offsets[index_])));
    return *this;
}

iterator &iterator::operator--() {
    guarantee(index_ > -1, "Trying to decrement past the beginning of an iterator.");
    do {
        --index_;
    } while (index_ >= 0 && !entry_is_live(get_entry(node_, node_->pair_offsets[index_])));
    return *this;
}

bool iterator::operator==(const iterator &other) const { return cmp(other) == 0; }
bool iterator::operator!=(const iterator &other) const { return cmp(other) != 0; }
bool iterator::operator<(const iterator &other) const { return cmp(other) < 0; }
bool iterator::operator>(const iterator &other) const { return cmp(other) > 0; }
bool iterator::operator<=(const iterator &other) const { return cmp(other) <= 0; }
bool iterator::operator>=(const iterator &other) const { return cmp(other) >= 0; }

int iterator::cmp(const iterator &other) const {
    guarantee(node_ == other.node_);
    return index_ - other.index_;
}

reverse_iterator::reverse_iterator() { }

reverse_iterator::reverse_iterator(const leaf_node_t *node, int index)
    : inner_(node, index) { }

std::pair<const btree_key_t *, const void *> reverse_iterator::operator*() const {
    return *inner_;
}

reverse_iterator &reverse_iterator::operator++() {
    --inner_;
    return *this;
}

reverse_iterator &reverse_iterator::operator--() {
    ++inner_;
    return *this;
}

bool reverse_iterator::operator==(const reverse_iterator &other) const { return inner_ == other.inner_; }
bool reverse_iterator::operator!=(const reverse_iterator &other) const { return inner_ != other.inner_; }
bool reverse_iterator::operator<(const reverse_iterator &other) const { return inner_ > other.inner_; }
bool reverse_iterator::operator>(const reverse_iterator &other) const { return inner_ < other.inner_; }
bool reverse_iterator::operator<=(const reverse_iterator &other) const { return inner_ >= other.inner_; }
bool reverse_iterator::operator>=(const reverse_iterator &other) const { return inner_ <= other.inner_; }


leaf_node_t::iterator begin(const leaf_node_t &leaf_node) {
    return ++leaf_node_t::iterator(&leaf_node, -1);
}

leaf_node_t::iterator end(const leaf_node_t &leaf_node) {
    return leaf_node_t::iterator(&leaf_node, leaf_node.num_pairs);
}

leaf_node_t::reverse_iterator rbegin(const leaf_node_t &leaf_node) {
    return ++leaf_node_t::reverse_iterator(&leaf_node, leaf_node.num_pairs);
}

leaf_node_t::reverse_iterator rend(const leaf_node_t &leaf_node) {
    return leaf_node_t::reverse_iterator(&leaf_node, -1);
}

leaf::iterator inclusive_lower_bound(const btree_key_t *key, const leaf_node_t &leaf_node) {
    int index;
    leaf::find_key(&leaf_node, key, &index);
    if (index == leaf_node.num_pairs ||
        entry_is_live(leaf::get_entry(&leaf_node, leaf_node.pair_offsets[index]))) {
        return leaf_node_t::iterator(&leaf_node, index);
    } else {
        return ++leaf_node_t::iterator(&leaf_node, index);
    }
}

leaf::reverse_iterator exclusive_upper_bound(const btree_key_t *key, const leaf_node_t &leaf_node) {
    int index;
    leaf::find_key(&leaf_node, key, &index);
    if (index < leaf_node.num_pairs) {
        const leaf::entry_t *entry = leaf::get_entry(&leaf_node, leaf_node.pair_offsets[index]);
        const btree_key_t *ekey = leaf::entry_key(entry);
        if (entry_is_live(entry) &&
            btree_key_cmp(ekey, key) == 0) {
            // We have to skip this entry to make the iterator exclusive,
            // hence the ++.
            return ++leaf_node_t::reverse_iterator(&leaf_node, index);
        }
    }

    return ++leaf_node_t::reverse_iterator(&leaf_node, index);
}

}  // namespace leaf

#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 901)
#pragma GCC diagnostic pop
#endif
