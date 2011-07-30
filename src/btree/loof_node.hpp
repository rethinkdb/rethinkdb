#ifndef BTREE_LOOF_NODE_HPP_
#define BTREE_LOOF_NODE_HPP_

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
    return x == DELETE_PAIR_CODE;
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

const entry_t *get_entry(const loof_t *node, int offset) {
    return reinterpret_cast<const entry_t *>(reinterpret_cast<const char *>(node) + offset + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0));
}

entry_t *get_entry(loof_t *node, int offset) {
    return reinterpret_cast<entry_t *>(reinterpret_cast<char *>(node) + offset + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0));
}

struct entry_iter_t {
    int offset;

    template <class V>
    void step(value_sizer_t<V> *sizer, const loof_t *node) {
	rassert(!done(sizer));

	offset += entry_size(get_entry(node, offset)) + (offset < node->tstamp_cutpoint ? sizeof(repli_timestamp_t) : 0);
    }

    bool done(value_sizer_t<V> *sizer) const {
	rassert(offset <= sizer->block_size().value());
	return offset >= sizer->block_size().value();
    }

    static entry_iter_t make(const loof_t *node) {
	entry_iter_t ret;
	ret.offset = node->frontmost;
	return ret;
    }
};

template <class V>
void init(value_sizer_t<V> *sizer, loof_t *node) {
    node->magic = sizer->btree_leaf_magic();
    node->num_pairs = 0;
    node->live_size = 0;
    node->frontmost = sizer->block_size().value();
    node->tstamp_cutpoint = node->frontmost;
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

	entry_t *ent = get_entry(node, iter.offset);
	if (entry_is_deletion(ent)) {
            if (deletions_cost >= max_deletions_cost) {
                break;
            }

            int this_entry_cost = sizeof(uint16_t) + sizeof(repli_timestamp_t) + entry_size(ent);
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

template <class V>
void split(value_sizer_t<V> *sizer, loof_t *node, loof_t *rnode, btree_key_t *median_out) {
    int tstamp_back_offset;
    int mandatory = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS, &tstamp_back_offset);

    rassert(mandatory >= free_space(sizer) - loof_epsilon(sizer));

    // We shall split the mandatory cost of this node as evenly as possible.

    int i = 0;
    int prev_lcost = 0;
    int lcost = 0;
    while (i < node->num_pairs && 2 * lcost < mandatory) {
        int offset = node->pair_offsets[i];
        entry_t *ent = get_entry(node, offset);

        // We only take mandatory entries' costs into consideration,
        // which guarantees correct behavior (in that neither node nor
        // can become underfull after a split).  If we didn't do this,
        // it would be possible to bias one node with a bunch of
        // deletions that makes its mandatory_cost artificially small.

        if (entry_is_live(ent)) {
            prev_lcost = lcost;
            lcost += entry_size(sizer, ent) + sizeof(uint16_t) + (offset < tstamp_back_offset ? sizeof(repli_timestamp_t) : 0);
        } else {
            rassert(entry_is_deletion(ent));

            if (offset < tstamp_back_offset) {
                prev_lcost = lcost;
                lcost += entry_size(sizer, ent) + sizeof(uint16_t) + sizeof(repli_timestamp_t);
            }
        }

        ++i;
    }

    // Since the mandatory_cost is at least free_space - loof_epsilon there's no way i can equal num_pairs or zero.
    rassert(i < node->num_pairs);
    rassert(i > 0);

    // Now prev_lcost and lcost envelope mandatory / 2.
    rassert(prev_lcost <= mandatory / 2);
    rassert(lcost > mandatory / 2);

    int s;
#ifndef NDEBUG
    int end_lcost;
#endif
    if ((mandatory - prev_lcost) - prev_lcost < lcost - (mandatory - lcost)) {
#ifndef NDEBUG
        end_lcost = prev_lcost;
#endif
        s = i - 1;
    } else {
#ifndef NDEBUG
        end_lcost = lcost;
#endif
        s = i;
    }

    // If our math was right, neither node can be underfull just
    // considering the split of the mandatory costs.
    rassert(end_lcost >= free_space(sizer) / 2 - loof_epsilon(sizer));
    rassert(mandatory - end_lcost >= free_space(sizer) / 2 - loof_epsilon(sizer));

    // Now we wish to move the elements at indices [s, num_pairs) to rnode.

    init(sizer, rnode);

    move_elements(sizer, node, s, node->num_pairs, rnode);

    node->num_pairs = s;

    keycpy(median_out, entry_key(get_entry(node, node->pair_offsets[s - 1])));
}

template <class V>
void merge(value_sizer_t<V> *sizer, loof_t *left, loof_t *right, btree_key_t *key_to_remove_out) {
    rassert(left != right);
    rassert(is_underfull(sizer, left));
    rassert(is_underfull(sizer, right));

    rassert(left->num_pairs > 0);
    rassert(right->num_pairs > 0);

    move_elements(sizer, left, 0, left->num_pairs, right);

    rassert(right->num_pairs > 0);
    keycpy(key_to_remove_out, entry_key(get_entry(right, right->pair_offsets[0])));
}

// We move keys out of sibling and into node.
template <class V>
bool level(value_sizer_t<V> *sizer, loof_t *node, loof_t *sibling, btree_key_t *key_to_replace_out, btree_key_t *replacement_key_out) {
    rassert(node != sibling);

    // If sibling were underfull, we'd just merge the nodes.
    rassert(is_underfull(node));
    rassert(!is_underfull(sibling));

    rassert(node->num_pairs > 0);
    rassert(sibling->num_pairs > 0);

    // First figure out the inclusive range [beg, end] of elements we want to move from sibling.
    int beg, end, *w, wstep;

    int node_weight = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS);
    int tstamp_back_offset;
    int sibling_weight = mandatory_cost(sizer, node, MANDATORY_TIMESTAMPS, &tstamp_back_offset);

    rassert(node_weight < sibling_weight);

    int nodecmp_result = nodecmp(node, sibling);
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

    int prev_diff;
    for (;;) {
        int offset = sibling->pair_offsets[*w];
        entry_t *ent = get_entry(sibling, offset);

        // We only take mandatory entries' costs into consideration.
        if (entry_is_live(ent)) {
            int sz = entry_size(sizer, ent) + sizeof(uint16_t) + (offset < tstamp_back_offset ? sizeof(repli_timestamp_t) : 0);
            prev_diff = sibling_weight - node_weight;
            node_weight += sz;
            sibling_weight -= sz;
        } else {
            rassert(entry_is_deletion(ent));

            if (offset < tstamp_back_offset) {
                int sz = entry_size(sizer, ent) + sizeof(uint16_t) + sizeof(repli_timestamp_t);
                prev_diff = sibling_weight - node_weight;
                node_weight += sz;
                sibling_weight -= sz;
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
    }

    if (end < beg) {
        // Alas, there is no actual leveling to do.
        rassert(end + 1 == beg);
        return false;
    }

    move_elements(sizer, sibling, beg, end + 1, node);

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













}  // namespace loof


#endif  BTREE_LOOF_NODE_HPP_
