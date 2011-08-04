#ifndef BTREE_LOOF_NODE_HPP_
#define BTREE_LOOF_NODE_HPP_

#include "buffer_cache/types.hpp"
#include "config/args.hpp"
#include "errors.hpp"
#include "node.hpp"

// Eventually we'll rename loof to leaf, but right now I want the old code to look at.

namespace loof {

// These codes can appear as the first byte of a leaf node entry
// (values 250 or smaller are just key sizes for key/value pairs).

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

// We must maintain timestamps and deletion entries as best we can,
// with the following limitations.  The timestamps stored need not be
// more than the most `MANDATORY_TIMESTAMPS` recent timestamps.  The
// deletions stored need not be any more than what is necessary to
// fill `(block_size - offsetof(loof_t, pair_offsets)) /
// DELETION_RESERVE_FRACTION` bytes.  For example, with a 4084 block
// size, if the five most recent operations were deletions of 250-byte
// keys, we would only be required to store the 2 most recent
// deletions and the 2 most recent timestamps.

const int MANDATORY_TIMESTAMPS = 5;
const int DELETION_RESERVE_FRACTION = 10;

struct loof_t {
    // The value-type-specific magic value.
    block_magic_t magic;

    // The size of pair_offsets.
    uint16_t num_pairs;

    // The total size (in bytes) of the live entries and their 2-byte
    // pair offsets in pair_offsets.
    uint16_t live_size;

    // The frontmost offset.
    uint16_t frontmost;

    // The first offset whose entry is not accompanied by a timestamp.
    uint16_t tstamp_cutpoint;

    // The pair offsets.
    uint16_t pair_offsets[];
};

struct entry_t;
struct value_t;

inline
bool entry_is_deletion(const entry_t *p) {
    uint8_t x = *reinterpret_cast<const uint8_t *>(p);
    rassert(x != 251);
    return x == DELETE_ENTRY_CODE;
}

inline
bool entry_is_live(const entry_t *p) {
    uint8_t x = *reinterpret_cast<const uint8_t *>(p);
    rassert(x != 251);
    rassert(MAX_KEY_SIZE == 250);
    return x <= MAX_KEY_SIZE;
}

inline
bool entry_is_skip(const entry_t *p) {
    return !entry_is_deletion(p) && !entry_is_live(p);
}

inline
const btree_key_t *entry_key(const entry_t *p) {
    if (entry_is_deletion(p)) {
        return reinterpret_cast<const btree_key_t *>(1 + reinterpret_cast<const char *>(p));
    } else {
        return reinterpret_cast<const btree_key_t *>(p);
    }
}

template <class V>
const V *entry_value(const entry_t *p) {
    if (entry_is_deletion(p)) {
        return NULL;
    } else {
        return reinterpret_cast<const V *>(reinterpret_cast<const char *>(p) + entry_key(p)->full_size());
    }
}

template <class V>
bool entry_fits(value_sizer_t<V> *sizer, const entry_t *p, int size) {
    if (size < 1) {
        return false;
    }

    uint8_t code = *reinterpret_cast<const uint8_t *>(p);
    switch (code) {
    case DELETE_ENTRY_CODE:
        return entry_key(p)->fits(size - 1);
    case SKIP_ENTRY_CODE_ONE:
        return true;
    case SKIP_ENTRY_CODE_TWO:
        return size >= 2;
    case SKIP_ENTRY_CODE_MANY:
        return size >= 3 && size >= 3 + *reinterpret_cast<const uint16_t *>(1 + reinterpret_cast<const char *>(p));
    default:
        rassert(code <= MAX_KEY_SIZE);
        return entry_key(p)->fits(size) && sizer->fits(entry_value<V>(p), size - entry_key(p)->full_size());
    }
}

template <class V>
int entry_size(value_sizer_t<V> *sizer, const entry_t *p) {
    uint8_t code = *reinterpret_cast<const uint8_t *>(p);
    switch (code) {
    case DELETE_ENTRY_CODE:
        return 1 + entry_key(p)->full_size();
    case SKIP_ENTRY_CODE_ONE:
        return 1;
    case SKIP_ENTRY_CODE_TWO:
        return 2;
    case SKIP_ENTRY_CODE_MANY:
        return 3 + *reinterpret_cast<const uint16_t *>(1 + reinterpret_cast<const char *>(p));
    default:
        rassert(code <= MAX_KEY_SIZE);
        return entry_key(p)->full_size() + sizer->size(entry_value<V>(p));
    }
}

inline
const entry_t *get_entry(const loof_t *node, int offset) {
    return reinterpret_cast<const entry_t *>(reinterpret_cast<const char *>(node) + offset + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0));
}

inline
entry_t *get_entry(loof_t *node, int offset) {
    return reinterpret_cast<entry_t *>(reinterpret_cast<char *>(node) + offset + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0));
}

inline
char *get_at_offset(loof_t *node, int offset) {
    return reinterpret_cast<char *>(node) + offset;
}

inline
repli_timestamp_t get_timestamp(const loof_t *node, int offset) {
    return *reinterpret_cast<const repli_timestamp_t *>(reinterpret_cast<const char *>(node) + offset);
}

struct entry_iter_t {
    int offset;

    template <class V>
    void step(value_sizer_t<V> *sizer, const loof_t *node) {
	rassert(!done(sizer));

	offset += entry_size(sizer, get_entry(node, offset)) + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0);
    }

    template <class V>
    bool done(value_sizer_t<V> *sizer) const {
        int bs = sizer->block_size().value();
	rassert(offset <= bs, "offset=%d, bs=%d", offset, bs);
	return offset == bs;
    }

    static entry_iter_t make(const loof_t *node) {
	entry_iter_t ret;
	ret.offset = node->frontmost;
	return ret;
    }
};

template <class V>
void print_entry(FILE *fp, value_sizer_t<V> *sizer, const entry_t *entry) {
    if (entry_is_live(entry)) {
        const btree_key_t *key = entry_key(entry);
        fprintf(fp, "%.*s:", int(key->size), key->contents);
        fprintf(fp, "[entry size=%d]", entry_size(sizer, entry));
        fprintf(fp, "[value size=%d]", sizer->size(entry_value<V>(entry)));
    } else if (entry_is_deletion(entry)) {
        const btree_key_t *key = entry_key(entry);
        fprintf(fp, "%.*s:[deletion]", int(key->size), key->contents);
    } else if (entry_is_skip(entry)) {
        fprintf(fp, "[skip %d]", entry_size(sizer, entry));
    } else {
        fprintf(fp, "[code %d]", *reinterpret_cast<const uint8_t *>(entry));
    }
    fflush(fp);
}

template <class V>
void print(FILE *fp, value_sizer_t<V> *sizer, const loof_t *node) {
    fprintf(fp, "Loof(magic='%4.4s', num_pairs=%u, live_size=%u, frontmost=%u, tstamp_cutpoint=%u)\n",
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
            fprintf(fp, "[t=%u]", tstamp.time);
            fflush(fp);
        }
        print_entry(fp, sizer, get_entry(node, iter.offset));
        iter.step(sizer, node);
    }
    fprintf(fp, "\n");
    fflush(fp);
}

template <class V>
void validate(value_sizer_t<V> *sizer, const loof_t *node) {
#ifndef NDEBUG
    print(stdout, sizer, node);

    // Check that all offsets are contiguous (with interspersed skip
    // entries), that they start with frontmost, that live_size is
    // correct, that we have correct magic, that the keys are in
    // order, that there are no deletion entries after
    // tstamp_cutpoint, that tstamp_cutpoint lies on an entry
    // boundary.

    // Basic sanity checks on fields' values.
    rassert(node->magic == sizer->btree_leaf_magic());
    rassert(node->frontmost >= offsetof(loof_t, pair_offsets) + node->num_pairs * sizeof(uint16_t));
    rassert(node->live_size <= (sizer->block_size().value() - node->frontmost) + sizeof(uint16_t) * node->num_pairs);
    rassert(node->frontmost <= node->tstamp_cutpoint);
    rassert(node->tstamp_cutpoint <= sizer->block_size().value());

    // sizeof(offs) is guaranteed to be less than the block_size() thanks to assertions above.
    uint16_t offs[node->num_pairs];
    memcpy(offs, node->pair_offsets, node->num_pairs * sizeof(uint16_t));

    std::sort(offs, offs + node->num_pairs);

    rassert(node->num_pairs == 0 || node->frontmost <= offs[0],
            "num_pairs=%d, frontmost=%d, offs[0]?=%d", node->num_pairs, node->frontmost, node->num_pairs == 0 ? 0 : offs[0]);
    rassert(node->num_pairs == 0 || offs[node->num_pairs - 1] < sizer->block_size().value());

    entry_iter_t iter = entry_iter_t::make(node);

    int observed_live_size = 0;

    int i = 0;
    bool seen_tstamp_cutpoint = false;
    while (!iter.done(sizer)) {
        int offset = iter.offset;

        // tstamp_cutpoint is supposed to be on some entry's offset.
        if (offset >= node->tstamp_cutpoint && !seen_tstamp_cutpoint) {
            rassert(offset == node->tstamp_cutpoint);
            seen_tstamp_cutpoint = true;
        }

        const entry_t *ent = get_entry(node, offset);
        if (entry_is_live(ent)) {
            observed_live_size += sizeof(uint16_t) + entry_size(sizer, ent);
            rassert(i < node->num_pairs);
            rassert(offset == offs[i]);
            ++i;
        } else if (entry_is_deletion(ent)) {
            rassert(!seen_tstamp_cutpoint);
            rassert(i < node->num_pairs);
            rassert(offset == offs[i]);
            ++i;
        }

        iter.step(sizer, node);
    }

    rassert(i == node->num_pairs, "i=%d, num_pairs=%d", i, node->num_pairs);

    rassert(node->live_size == observed_live_size);

    // Entries look valid, check key ordering.

    const btree_key_t *last = NULL;
    for (int k = 0; k < node->num_pairs; ++k) {
        const btree_key_t *key = entry_key(get_entry(node, node->pair_offsets[k]));
        rassert(last == NULL || sized_strcmp(last->contents, last->size, key->contents, key->size) < 0);
        last = key;
    }

    // Okay.
#endif
}

template <class V>
void init(value_sizer_t<V> *sizer, loof_t *node) {
    node->magic = sizer->btree_leaf_magic();
    node->num_pairs = 0;
    node->live_size = 0;
    node->frontmost = sizer->block_size().value();
    node->tstamp_cutpoint = node->frontmost;

    validate(sizer, node);
}

template <class V>
int free_space(value_sizer_t<V> *sizer) {
    return sizer->block_size().value() - offsetof(loof_t, pair_offsets);
}

// Returns the mandatory storage cost of the node, returning a value
// in the closed interval [0, free_space(sizer)].  Outputs the offset
// of the first entry for which storing a timestamp is not mandatory.
template <class V>
int mandatory_cost(value_sizer_t<V> *sizer, const loof_t *node, int required_timestamps, int *tstamp_back_offset_out) {
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

template <class V>
int mandatory_cost(value_sizer_t<V> *sizer, const loof_t *node, int required_timestamps) {
    int ignored;
    return mandatory_cost(sizer, node, required_timestamps, &ignored);
}

template <class V>
int loof_epsilon(value_sizer_t<V> *sizer) {
    // Returns the maximum possible entry size, i.e. the key cost plus
    // the value cost plus pair_offsets plus timestamp cost.

    int key_cost = sizeof(uint8_t) + MAX_KEY_SIZE;

    // If the value is always empty, the DELETE_ENTRY_CODE byte needs to be considered.
    int n = std::max(sizer->max_possible_size(), 1);
    int pair_offsets_cost = sizeof(uint16_t);
    int timestamp_cost = sizeof(repli_timestamp_t);

    return key_cost + n + pair_offsets_cost + timestamp_cost;
}

inline bool is_empty(const loof_t *node) {
    return node->num_pairs == 0;
}


template <class V>
bool is_full(value_sizer_t<V> *sizer, const loof_t *node, const btree_key_t *key, const V *value) {

    // Upon an insertion, we preserve `MANDATORY_TIMESTAMPS - 1`
    // timestamps and add our own (accounted for below)
    int size = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS - 1);

    // Add the space we'll need for the new key/value pair we would
    // insert.  We conservatively assume the key is not already
    // contained in the node.

    size += sizeof(uint16_t) + sizeof(repli_timestamp_t) + key->full_size() + sizer->size(value);

    // The node is full if we can't fit all that data within the free space.
    return size > free_space(sizer);
}

template <class V>
bool is_underfull(value_sizer_t<V> *sizer, const loof_t *node) {

    // An underfull node is one whose mandatory fields' cost
    // constitutes significantly less than half the free space, where
    // "significantly" is enough to prevent a split-then-merge.

    // (Note that x / y is indeed taken to mean floor(x / y) below.)

    // A split node's size S is always within loof_epsilon of
    // free_space and then is split as evenly as possible.  This means
    // the two resultant nodes' sizes are both no less than S / 2 -
    // loof_epsilon / 2, which is no less than (free_space -
    // loof_epsilon) / 2 - loof_epsilon / 2.  Which is no less than is
    // free_space / 2 - loof_epsilon.  We don't want an immediately
    // split node to be underfull, hence the threshold used below.

    return mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS) < free_space(sizer) / 2 - loof_epsilon(sizer);
}

// Compares indices by looking at values in another array.
class indirect_index_comparator_t {
public:
    indirect_index_comparator_t(const uint16_t *array) : array_(array) { }

    bool operator()(uint16_t x, uint16_t y) {
        return array_[x] < array_[y];
    }

private:
    const uint16_t *array_;
};

template <class V>
void garbage_collect(value_sizer_t<V> *sizer, loof_t *node, int num_tstamped, int *preserved_index) {
    validate(sizer, node);

    uint16_t indices[node->num_pairs];

    for (int i = 0; i < node->num_pairs; ++i) {
        indices[i] = i;
    }

    std::sort(indices, indices + node->num_pairs, indirect_index_comparator_t(node->pair_offsets));

    int mand_offset;
    UNUSED int cost = mandatory_cost(sizer, node, num_tstamped, &mand_offset);

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

        // Preserve the timestamp.
        int sz = sizeof(repli_timestamp_t) + entry_size(sizer, get_entry(node, offset));

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

template <class V>
void garbage_collect(value_sizer_t<V> *sizer, loof_t *node, int num_tstamped) {
    int ignore = 0;
    garbage_collect(sizer, node, num_tstamped, &ignore);
    rassert(ignore == 0);
}

inline void clean_entry(void *p, int sz) {
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
        *reinterpret_cast<uint16_t *>(q + 1) = sz - 3;

        // Some  memset implementations are broken for nonzero values.
        for (int i = 3; i < sz; ++i) {
            q[i] = SKIP_ENTRY_RESERVED;
        }
    }
}

// Moves entries with pair_offsets indices in the clopen range [beg,
// end) from fro to tow.
template <class V>
void move_elements(value_sizer_t<V> *sizer, loof_t *fro, int beg, int end, int wpoint, loof_t *tow, int fro_copysize, int fro_mand_offset) {
    validate(sizer, fro);
    validate(sizer, tow);

    printf("move_elements beg = %d, end = %d, wpoint = %d, fro_copysize = %d, fro_mand_offset = %d\n",
           beg, end, wpoint, fro_copysize, fro_mand_offset);

    rassert(is_underfull(sizer, tow));

#ifndef NDEBUG
    int verify_fro_mand_offset;
    int fro_cost = mandatory_cost(sizer, fro, MANDATORY_TIMESTAMPS, &verify_fro_mand_offset);
    rassert(fro_cost + mandatory_cost(sizer, tow, MANDATORY_TIMESTAMPS) <= free_space(sizer));
    rassert(verify_fro_mand_offset == fro_mand_offset);
#endif

    // Make tow have a nice big region we can copy entries to.  Also,
    // this means we have no "skip" entries in tow.
    garbage_collect(sizer, tow, MANDATORY_TIMESTAMPS);

    // Now resize and move tow's pair_offsets.
    memmove(tow->pair_offsets + wpoint + (end - beg), tow->pair_offsets + wpoint, sizeof(uint16_t) * (tow->num_pairs - wpoint));

    tow->num_pairs += end - beg;

    // Now we're going to do something crazy.  Fill the new hole in
    // the pair offsets with the numbers in [0, end - beg).
    for (int i = 0; i < end - beg; ++i) {
        tow->pair_offsets[wpoint + i] = i;
    }

    // We treat these numbers as indices into [beg, end) in fro, and
    // sort them so that we can access [beg, end) in order by
    // increasing offset.
    std::sort(tow->pair_offsets + wpoint, tow->pair_offsets + wpoint + (end - beg), indirect_index_comparator_t(fro->pair_offsets + beg));

    // The offset we read from, in tow.
    int tow_offset = tow->frontmost;

    // The offset we read from (indirectly pointing to fro's [beg,
    // end)) in tow->pair_offsets, and the offset at which we stop.
    int fro_index = wpoint;
    int fro_index_end = wpoint + (end - beg);

    // The new frontmost offset.
    const int new_frontmost = tow->frontmost - fro_copysize;

    // The offset at which we write the next entry.
    int wri_offset = new_frontmost;

    int adjustable_tow_offsets[MANDATORY_TIMESTAMPS];
    int num_adjustable_tow_offsets = 0;

    // We will gradually compute the live size.
    int livesize = tow->live_size;

    for (int i = 0; i < wpoint; ++i) {
        if (tow->pair_offsets[i] < tow->tstamp_cutpoint) {
            rassert(num_adjustable_tow_offsets < MANDATORY_TIMESTAMPS);
            adjustable_tow_offsets[num_adjustable_tow_offsets] = i;
            num_adjustable_tow_offsets ++;
        }
    }

    for (int i = wpoint + (end - beg); i < tow->num_pairs; ++i) {
        if (tow->pair_offsets[i] < tow->tstamp_cutpoint) {
            rassert(num_adjustable_tow_offsets < MANDATORY_TIMESTAMPS);
            adjustable_tow_offsets[num_adjustable_tow_offsets] = i;
            num_adjustable_tow_offsets ++;
        }
    }

    int fro_live_size_adjustment = 0;

    for (;;) {
        printf("fro_index = %d, tow_offset = %d, wri_offset = %d\n", fro_index, tow_offset, wri_offset);
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
            clean_entry(ent, entsz);
            fro_live_size_adjustment -= entsz + sizeof(uint16_t);

            // Update the pair offset in fro to be the offset in tow
            // -- we'll never use the old value again and we'll copy
            // the newer values to tow later.
            fro->pair_offsets[beg + tow->pair_offsets[fro_index]] = wri_offset;

            wri_offset += sz;
            fro_index++;

            if (entry_is_live(ent)) {
                livesize += entsz + sizeof(uint16_t);
            }
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
        printf("fro_index = %d, wri_offset = %d\n", fro_index, wri_offset);
        int fro_offset = fro->pair_offsets[beg + tow->pair_offsets[fro_index]];
        entry_t *ent = get_entry(fro, fro_offset);
        if (entry_is_live(ent)) {
            int sz = entry_size(sizer, ent);
            memmove(get_at_offset(tow, wri_offset), get_at_offset(fro, fro_offset), sz);
            clean_entry(ent, sz);
            fro_live_size_adjustment -= sz + sizeof(uint16_t);

            fro->pair_offsets[beg + tow->pair_offsets[fro_index]] = wri_offset;
            wri_offset += sz;
            livesize += sz + sizeof(uint16_t);
        }
    }

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
        }
        tow_offset += sizeof(repli_timestamp_t) + sz;
    }

    rassert(wri_offset <= tow_offset);

    // If we needed to untimestamp any tow entries, we'll need a skip
    // entry for the open space.
    if (wri_offset < tow_offset) {
        printf("wri_offset = %d, tow_offset = %d\n", wri_offset, tow_offset);
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

    printf("Validating fro\n");
    validate(sizer, fro);
    printf("Validating tow\n");
    validate(sizer, tow);
}

template <class V>
void split(value_sizer_t<V> *sizer, loof_t *node, loof_t *rnode, btree_key_t *median_out) {
    int tstamp_back_offset;
    int mandatory = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS, &tstamp_back_offset);

    rassert(mandatory >= free_space(sizer) - loof_epsilon(sizer));

    // We shall split the mandatory cost of this node as evenly as possible.

    int num_mandatories = 0;
    int i = node->num_pairs - 1;
    int prev_rcost = 0;
    int rcost = 0;
    while (i >= 0 && 2 * rcost < mandatory) {
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

            ++ num_mandatories;
        } else {
            rassert(entry_is_deletion(ent));

            if (offset < tstamp_back_offset) {
                prev_rcost = rcost;
                rcost += entry_size(sizer, ent) + sizeof(uint16_t) + sizeof(repli_timestamp_t);

                ++ num_mandatories;
            }
        }

        --i;
    }

    // Since the mandatory_cost is at least free_space - loof_epsilon there's no way i can equal num_pairs or zero.
    rassert(i < node->num_pairs);
    rassert(i > 0);

    // Now prev_rcost and rcost envelope mandatory / 2.
    rassert(prev_rcost <= mandatory / 2);
    rassert(rcost > mandatory / 2);

    int s;
    int end_rcost;
    if ((mandatory - prev_rcost) - prev_rcost < rcost - (mandatory - rcost)) {
        end_rcost = prev_rcost;
        s = i + 2;
        -- num_mandatories;
    } else {
        end_rcost = rcost;
        s = i + 1;
    }

    // If our math was right, neither node can be underfull just
    // considering the split of the mandatory costs.
    rassert(end_rcost >= free_space(sizer) / 2 - loof_epsilon(sizer));
    rassert(mandatory - end_rcost >= free_space(sizer) / 2 - loof_epsilon(sizer));

    // Now we wish to move the elements at indices [s, num_pairs) to rnode.

    init(sizer, rnode);

    int node_copysize = end_rcost - num_mandatories * sizeof(uint16_t);
    move_elements(sizer, node, s, node->num_pairs, 0, rnode, node_copysize, tstamp_back_offset);

    keycpy(median_out, entry_key(get_entry(node, node->pair_offsets[s - 1])));
}

template <class V>
void merge(value_sizer_t<V> *sizer, loof_t *left, loof_t *right, btree_key_t *key_to_remove_out) {
    rassert(left != right);
    rassert(is_underfull(sizer, left));
    rassert(is_underfull(sizer, right));

    rassert(left->num_pairs > 0);
    rassert(right->num_pairs > 0);

    int tstamp_back_offset;
    int mandatory = mandatory_cost(sizer, left, MANDATORY_TIMESTAMPS, &tstamp_back_offset);
    move_elements(sizer, left, 0, left->num_pairs, 0, right, mandatory - left->num_pairs * sizeof(uint16_t), tstamp_back_offset);

    rassert(right->num_pairs > 0);
    keycpy(key_to_remove_out, entry_key(get_entry(right, right->pair_offsets[0])));
}

// We move keys out of sibling and into node.
template <class V>
bool level(value_sizer_t<V> *sizer, loof_t *node, loof_t *sibling, btree_key_t *key_to_replace_out, btree_key_t *replacement_key_out) {
    rassert(node != sibling);

    // If sibling were underfull, we'd just merge the nodes.
    rassert(is_underfull(sizer, node));
    rassert(!is_underfull(sizer, sibling));

    rassert(node->num_pairs > 0);
    rassert(sibling->num_pairs > 0);

    // First figure out the inclusive range [beg, end] of elements we want to move from sibling.
    int beg, end, *w, wstep;

    int node_weight = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS);
    int tstamp_back_offset;
    int sibling_weight = mandatory_cost(sizer, sibling, MANDATORY_TIMESTAMPS, &tstamp_back_offset);

    rassert(node_weight < sibling_weight);

    btree_key_t *nodecmp_left_key = entry_key(get_entry(node, node->pair_offsets[0]));
    btree_key_t *nodecmp_right_key = entry_key(get_entry(sibling, sibling->pair_offsets[0]));
    int nodecmp_result = sized_strcmp(nodecmp_left_key->contents, nodecmp_left_key->size,
                                      nodecmp_right_key->contents, nodecmp_right_key->size);
    rassert(nodecmp_result != 0);
    if (nodecmp_result < 0) {
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

    rassert(end - beg != sibling->num_pairs - 1);

    int prev_weight_movement = 0;
    int weight_movement = 0;
    int num_mandatories = 0;
    int prev_diff;
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

            ++ num_mandatories;
        } else {
            rassert(entry_is_deletion(ent));

            if (offset < tstamp_back_offset) {
                int sz = entry_size(sizer, ent) + sizeof(uint16_t) + sizeof(repli_timestamp_t);
                prev_diff = sibling_weight - node_weight;
                prev_weight_movement = weight_movement;
                weight_movement += sz;
                node_weight += sz;
                sibling_weight -= sz;

                ++ num_mandatories;
            }
        }

        if (end - beg == sibling->num_pairs - 1 || node_weight >= sibling_weight) {
            break;
        }

        *w += wstep;
    }

    rassert(end - beg < sibling->num_pairs - 1);

    if (prev_diff <= sibling_weight - node_weight) {
        *w -= wstep;
        -- num_mandatories;
        weight_movement = prev_weight_movement;
    }

    if (end < beg) {
        // Alas, there is no actual leveling to do.
        rassert(end + 1 == beg);
        return false;
    }

    int sib_copysize = weight_movement - num_mandatories * sizeof(uint16_t);
    move_elements(sizer, sibling, beg, end + 1, nodecmp_result < 0 ? node->num_pairs : 0, node, sib_copysize, tstamp_back_offset);

    // key_to_replace_out is set to a key that is <= the key to
    // replace, but > any lesser key, and replacement_key_out is the
    // actual replacement key.
    if (nodecmp_result < 0) {
        keycpy(key_to_replace_out, entry_key(get_entry(node, node->pair_offsets[0])));
        keycpy(replacement_key_out, entry_key(get_entry(node, node->pair_offsets[node->num_pairs - 1])));
    } else {
        keycpy(key_to_replace_out, entry_key(get_entry(sibling, sibling->pair_offsets[0])));
        keycpy(replacement_key_out, entry_key(get_entry(sibling, sibling->pair_offsets[sibling->num_pairs - 1])));
    }

    return true;
}

template <class V>
bool is_mergable(value_sizer_t<V> *sizer, const loof_t *node, const loof_t *sibling) {
    return is_underfull(sizer, node) && is_underfull(sizer, sibling);
}

// Sets *index_out to the index for the live entry or deletion entry
// for the key, or to the index the key would have if it were
// inserted.  Returns true if the key at said index is actually equal.
inline
bool find_key(const loof_t *node, const btree_key_t *key, int *index_out) {
    int beg = 0;
    int end = node->num_pairs;

    // beg == 0 or key > *(beg - 1).
    // end == num_pairs or key < *end.

    while (beg < end) {
        // when (end - beg) > 0, (end - beg) / 2 is always less than (end - beg).  So beg <= test_point < end.
        int test_point = beg + (end - beg) / 2;

        const btree_key_t *ek = entry_key(get_entry(node, node->pair_offsets[test_point]));

        int res = sized_strcmp(key->contents, key->size, ek->contents, ek->size);

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

template <class V>
bool lookup(value_sizer_t<V> *sizer, const loof_t *node, const btree_key_t *key, V *value_out) {
    validate(sizer, node);

    int index;
    if (find_key(node, key, &index)) {
        entry_t *ent = get_entry(node, node->pair_offsets[index]);
        if (entry_is_live(ent)) {
            const V *val = entry_value<V>(ent);
            memcpy(value_out, val, sizer->size(val));
            return true;
        }
    }

    return false;
}

// Inserts a key/value pair into the node.  Hopefully you've already
// cleaned up the old value, if there is one.
template <class V>
void insert(value_sizer_t<V> *sizer, loof_t *node, const btree_key_t *key, const V *value, repli_timestamp_t tstamp) {
    validate(sizer, node);

    rassert(!is_full(sizer, node, key, value));

    if (offsetof(loof_t, pair_offsets) + sizeof(uint16_t) * (node->num_pairs + 1) + sizeof(repli_timestamp_t) + key->full_size() + sizer->size(value) > node->frontmost) {
        garbage_collect(sizer, node, MANDATORY_TIMESTAMPS - 1);
    }

    int index;
    bool found = find_key(node, key, &index);

    int live_size_adjustment = 0;
    int num_pairs_adjustment = 0;

    if (found) {
        int offset = node->pair_offsets[index];
        entry_t *ent = get_entry(node, offset);

        int sz = entry_size(sizer, ent);

        if (entry_is_live(ent)) {
            live_size_adjustment -= sizeof(uint16_t) + sz;
        }

        clean_entry(ent, sz);
    } else {
        memmove(node->pair_offsets + index + 1, node->pair_offsets + index, sizeof(uint16_t) * (node->num_pairs - index));
        num_pairs_adjustment = 1;
    }

    int sz = sizer->size(value);

    int w = node->frontmost;
    w -= sz;
    memcpy(get_at_offset(node, w), value, sz);

    w -= key->full_size();
    memcpy(get_at_offset(node, w), key, key->full_size());

    live_size_adjustment += sizeof(uint16_t) + (node->frontmost - w);

    w -= sizeof(repli_timestamp_t);
    *reinterpret_cast<repli_timestamp_t *>(get_at_offset(node, w)) = tstamp;

    node->num_pairs += num_pairs_adjustment;
    node->pair_offsets[index] = w;
    node->live_size += live_size_adjustment;
    node->frontmost = w;

    validate(sizer, node);
}

// This asserts that the key is in the node.
template <class V>
void remove(value_sizer_t<V> *sizer, loof_t *node, const btree_key_t *key, repli_timestamp_t tstamp) {
    validate(sizer, node);

    int index;
    bool found = find_key(node, key, &index);

    rassert(found);
    if (found) {
        int offset = node->pair_offsets[index];
        entry_t *ent = get_entry(node, offset);

        rassert(entry_is_live(ent));
        if (entry_is_live(ent)) {

            int sz = entry_size(sizer, ent);
            node->live_size -= sizeof(uint16_t) + sz;

            clean_entry(ent, sz);

            if (offsetof(loof_t, pair_offsets) + sizeof(uint16_t) * node->num_pairs + sizeof(repli_timestamp_t) + 1 + key->full_size() > node->frontmost) {
                memmove(node->pair_offsets + index, node->pair_offsets + index + 1, (node->num_pairs - (index + 1)) * sizeof(uint16_t));
                node->num_pairs -= 1;

                garbage_collect(sizer, node, MANDATORY_TIMESTAMPS - 1, &index);

                memmove(node->pair_offsets + index + 1, node->pair_offsets + index, (node->num_pairs - index) * sizeof(uint16_t));

                node->num_pairs += 1;
            }

            int w = node->frontmost;
            w -= key->full_size();
            memcpy(get_at_offset(node, w), key, key->full_size());
            w -= 1;
            *reinterpret_cast<uint8_t *>(get_at_offset(node, w)) = DELETE_ENTRY_CODE;
            w -= sizeof(repli_timestamp_t);
            *reinterpret_cast<repli_timestamp_t *>(get_at_offset(node, w)) = tstamp;

            node->pair_offsets[index] = w;
            node->frontmost = w;
        }
    }

    validate(sizer, node);
}

template <class V>
class entry_reception_callback_t {
public:
    // Says that the timestamp was too early, and we can't send accurate deletion history.
    virtual void lost_deletions() = 0;

    // Sends a deletion in the deletion history.
    virtual void deletion(const btree_key_t *k, repli_timestamp_t tstamp) = 0;

    // Sends a key/value pair in the leaf.
    virtual void key_value(const btree_key_t *k, const V *value, repli_timestamp_t tstamp) = 0;

protected:
    virtual ~entry_reception_callback_t() { }
};

template <class V>
void dump_entries_since_time(value_sizer_t<V> *sizer, const loof_t *node, repli_timestamp_t minimum_tstamp, entry_reception_callback_t<V> *cb) {
    validate(sizer, node);

    int stop_offset = 0;

    {
        repli_timestamp_t earliest = repli_timestamp_t::invalid;

        entry_iter_t iter = entry_iter_t::make(node);
        while (!iter.done(sizer) && iter.offset < node->tstamp_cutpoint) {
            repli_timestamp_t new_earliest = get_timestamp(node, iter.offset);
            rassert(! (earliest < new_earliest));
            earliest = new_earliest;
            iter.step(sizer, node);

            if (earliest < minimum_tstamp) {
                stop_offset = iter.offset;
                break;
            }
        }
    }

    bool include_deletions = true;
    if (stop_offset == 0) {
        stop_offset = sizer->block_size().value();
        cb->lost_deletions();
        include_deletions = false;
    }

    {
        entry_iter_t iter = entry_iter_t::make(node);
        while (iter.offset < stop_offset) {
            repli_timestamp_t tstamp = get_timestamp(node, iter.offset);
            const entry_t *ent = get_entry(node, iter.offset);

            if (entry_is_live(ent)) {
                cb->key_value(entry_key(ent), entry_value<V>(ent), tstamp);
            } else if (entry_is_deletion(ent) && include_deletions) {
                cb->deletion(entry_key(ent), tstamp);
            }
        }
    }
}


}  // namespace loof


#endif  // BTREE_LOOF_NODE_HPP_
