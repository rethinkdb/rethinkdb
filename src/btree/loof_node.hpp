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

// The amount of extra timestamp information we _must_ carry if there
// are sufficiently many keys in the node.
const int MANDATORY_EXTRAS = 4;

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


struct loof_t {
    // The value-type-specific magic value.
    block_magic_t magic;

    // The size of pair_offsets.
    uint16_t npairs;

    // The number of 
    int16_t nextra;
    repli_timestamp_t base_tstamp;
    uint16_t frontmost;
    uint16_t pair_offsets[];

    uint16_t *extra_tstamps() const {
        return pair_offsets + npairs;
    }
};

const entry_t *get_entry(const loof_t *node, int offset) {
    return reinterpret_cast<const entry_t *>(reinterpret_cast<const char *>(node) + offset);
}

entry_t *get_entry(loof_t *node, int offset) {
    return reinterpret_cast<entry_t *>(reinterpret_cast<char *>(node) + offset);
}

inline
void init(block_size_t block_size, loof_t *node) {
    node->magic = leaf_node_t::expected_magic;
    node->npairs = 0;
    node->nextra = -1;
    node->frontmost = block_size.value();
    node->base_tstamp = repli_timestamp_t::invalid;
}

inline
int mandatory_left_end_size(const loof_t *node) {
    return offsetof(loof_t, pair_offsets) + (node->npairs + MANDATORY_EXTRAS) * sizeof(uint16_t);
}

template <class V>
int live_size(value_sizer_t<V> *sizer, const loof_t *node) {
    int ret = 0;
    for (int i = 0; i < node->npairs; ++i) {
        ret += entry_size(sizer, get_entry(node, node->pair_offsets[i]));
    }
    return ret;
}

template <class V>
int mandatory_allocated_size(value_sizer_t<V> *sizer, const loof_t *node) {
    // We will need to gc.  So we compute the size needed for surviving allocated objects.
    // (Maybe we should just do the gc right here.)

    int livesize = live_size(sizer, node);

    // Count mandatory deletions.
    int mandatory_deletions_size = 0;
    int offset = node->frontmost;
    int count = 0;
    while (offset < sizer->block_size().value() && count < 1 + std::min<int>(node->nextra, MANDATORY_EXTRAS)) {
        const entry_t *entry = get_entry(node, offset);
        int es = entry_size(sizer, entry);
        if (entry_is_deletion(entry)) {
            mandatory_deletions_size += es;
        }

        offset += es;
        ++count;
    }

    return livesize + mandatory_deletions_size;
}

template <class V>
bool is_full(value_sizer_t<V> *sizer, const loof_t *node, const btree_key_t *key, const V *value) {
    // Can the key/value pair fit?  We can be conservative and assume
    // it does not already exist within the node.

    // The size for the entry, including its pair_offset.
    int kvp_size = key->full_size() + sizer->size(value) + sizeof(uint16_t);

    // The size of the left edge of the node, before inserting the entry.
    int left_end_size = mandatory_left_end_size(node);

    if (left_end_size + kvp_size <= node->frontmost) {
        // We won't need to gc before inserting the pair.
        return true;
    }

    return mandatory_allocated_size(sizer, node) + left_end_size + kvp_size > sizer->block_size().value();
}

template <class V>
bool is_underfull(value_sizer_t<V> *sizer, const loof_t *node) {
    return mandatory_left_end_size(node) + mandatory_allocated_size(sizer, node) <= sizer->block_size().value() / 4;
}

class key_filter_t {
public:
    virtual bool test(entry_t *ent) = 0;
protected:
    virtual ~key_filter_t() { }
};

class key_at_least_t : public key_filter_t {
public:
    key_at_least_t(btree_key_t *floor) : buffer_(floor) { }

    bool test(entry_t *ent) {
        if (entry_is_deletion(ent) || entry_is_live(ent)) {
            btree_key_t *k = entry_key(ent);
            return 0 <= sized_strcmp(buffer_.key()->contents, buffer_.key()->size, k->contents, k->size);
        }

        return false;
    }

private:
    btree_key_buffer_t buffer_;
};

class allow_everything_t : public key_filter_t {
public:
    bool test(entry_t *ent) {
        return entry_is_deletion(ent) || entry_is_live(ent);
    }
};

class and_no_deletes_t : public key_filter_t {
public:
    and_no_deletes_t(key_filter_t *inner) : inner_(inner) { }

    bool test(entry_t *ent) {
        return entry_is_live(ent) && inner_->test(ent);
    }

private:
    key_filter_t *inner_;
};

template <class V>
void press_offset(value_sizer_t<V> *sizer, loof_t *node, key_filter_t *filter, int *offset, int *tstamp_index) {
    int bs = sizer->block_size().value();

    for (;;) {
        rassert(*offset <= bs);
        if (*offset >= bs) {
            break;
        }

        entry_t *entry = get_entry(node, *offset);
        if (filter->test(entry)) {
            break;
        }

        *offset += entry_size(sizer, entry);
        if (*tstamp_index < node->nextra) {
            *tstamp_index += 1;
        }
    }
}

inline
repli_timestamp_t get_timestamp(loof_t *node, int tstamp_index, repli_timestamp_t original_base_tstamp) {
    rassert(tstamp_index >= -1);
    rassert(tstamp_index < node->nextra);
    if (tstamp_index == -1) {
        return original_base_tstamp;
    } else {
        repli_timestamp_t ret;
        ret.time = original_base_tstamp.time - (node->pair_offsets[node->npairs + tstamp_index]);
        return ret;
    }
}

inline
repli_timestamp_t get_timestamp(loof_t *node, int tstamp_index) {
    return get_timestamp(node, tstamp_index, node->base_tstamp);
}

inline
void write_skip_entry(loof_t *node, int offset, int size) {
    rassert(size > 0);
    rassert(size < 65536);
    uint8_t *p = reinterpret_cast<uint8_t *>(get_entry(node, offset));
    if (size == 1) {
        p[0] = SKIP_ENTRY_CODE_ONE;
    } else if (size == 2) {
        p[0] = SKIP_ENTRY_CODE_TWO;
        p[1] = SKIP_ENTRY_RESERVED;
    } else {
        p[0] = SKIP_ENTRY_CODE_MANY;
        *reinterpret_cast<uint16_t *>(p + 1) = size;
        for (int i = 3; i < size; ++i) {
            p[i] = SKIP_ENTRY_RESERVED;
        }
    }
}

inline
void move_entries_move_helper(loof_t *writee, int write_offset, loof_t *readee, int read_offset, int sz) {
    memmove(get_entry(writee, write_offset), get_entry(readee, read_offset), sz);

    if (writee != readee) {
        write_skip_entry(readee, read_offset, sz);
    }
}

// Garbage collects so that nextra becomes <= extra, but the pair
// offsets are no longer sorted by key.
template <class V>
void unsorted_garbage_collect(value_sizer_t<V> *sizer, loof_t *node, int extra) {
    int bs = sizer->block_size().value();
    int offset = node->frontmost;
    int tstamp_index = -1;
    allow_everything_t everything;

    int write_start = offset;
    int write_point = offset;
    int write_tstamp_index = -1;

    uint16_t *pair_offsets_overwriter = node->pair_offsets;
    repli_timestamp_t original_base_tstamp = node->base_tstamp;

    // At first we'd like to save deletions.
    for (;;) {
        press_offset(sizer, node, &everything, &offset, &tstamp_index);
        if (!(offset < bs && tstamp_index < node->nextra && tstamp_index < extra)) {
            break;
        }

        entry_t *entry = get_entry(node, offset);
        int sz = entry_size(sizer, entry);

        memmove(get_entry(node, write_point), entry, sz);

        set_timestamp(get_timestamp(node, tstamp_index, original_base_tstamp), write_tstamp_index);

        *pair_offsets_overwriter = write_point;
        ++ pair_offsets_overwriter;

        write_point += sz;
        offset += sz;
        tstamp_index += 1;
        write_tstamp_index += 1;
    }

    // Now we'd like to drop deletions.
    and_no_deletes_t every_live(&everything);

    for (;;) {
        press_offset(sizer, node, &every_live, &offset, &tstamp_index);

        if (offset >= bs) {
            break;
        }

        entry_t *entry = get_entry(node, offset);
        int sz = entry_size(sizer, entry);

        memmove(get_entry(node, write_point), entry, sz);

        *pair_offsets_overwriter = write_point;
        ++ pair_offsets_overwriter;

        write_point += sz;
        offset += sz;
    }

    rassert(pair_offsets_overwriter - node->pair_offsets == node->npairs);

    int shift = bs - write_point;
    rassert(shift >= 0);
    if (shift > 0) {
        memmove(get_entry(node, write_start + shift), get_entry(node, write_start), write_point - write_start);

        for (int i = 0; i < node->npairs; ++i) {
            node->pair_offsets[i] += shift;
        }

        // The pair_offsets are still not sorted (by key).
    }
}

class pair_offset_comp_t {
public:
    pair_offset_comp_t(loof_t *node) : node_(node) { }

    bool operator()(uint16_t x, uint16_t y) {
        btree_key_t *xkey = entry_key(get_entry(node_, x));
        btree_key_t *ykey = entry_key(get_entry(node_, y));
        return sized_strcmp(xkey->contents, xkey->size, ykey->contents, ykey->size) < 0;
    }

private:
    loof_t *node_;
};

inline
void sort_pair_offsets(loof_t *node) {
    std::sort(node->pair_offsets, node->pair_offsets + node->npairs, pair_offset_comp_t(node));
}



// Merges the deletion and live entries of `node` into `other` whose
// keys satisfy the given filter, replacing those entries with skip
// entries in the process.  Does not touch pair_offsets; the caller is
// responsible for cleaning up pair_offsets afterwards (and generally
// this is easy to do).  Retains sufficiently much recent information.
template <class V>
void move_entries_into_other_node(value_sizer_t<V> *sizer, loof_t *node, loof_t *other, key_filter_t *filter) {
    rassert(is_underfull(sizer, other));

    int bs = sizer->block_size().value();

    int nsave = 0;
    repli_timestamp_t tstamps[1 + MANDATORY_EXTRAS];
    int num_tstamps_both_nodes;

    allow_everything_t other_filter;

    int total_alloc_size = 0;

    {
        int i = 0;
        int node_offset = node->frontmost, other_offset = other->frontmost;
        int node_tstamp_index = -1, other_tstamp_index = -1;

        press_offset(sizer, node, filter, &node_offset, &node_tstamp_index);
        press_offset(sizer, other, &other_filter, &other_offset, &other_tstamp_index);

        bool last = false;
        while (!last && node_tstamp_index < node->nextra && other_tstamp_index < other->nextra && i != 1 + MANDATORY_EXTRAS) {
            repli_timestamp_t node_tstamp = get_timestamp(node, node_tstamp_index);
            repli_timestamp_t other_tstamp = get_timestamp(other, other_tstamp_index);

            key_filter_t *chosen_filter;
            repli_timestamp_t chosen_tstamp;
            int *chosen_offset, *chosen_tstamp_index;
            loof_t *chosen_node;

            if (node_tstamp > other_tstamp) {
                chosen_filter = filter;
                chosen_tstamp = node_tstamp;
                chosen_offset = &node_offset;
                chosen_tstamp_index = &node_tstamp_index;
                chosen_node = node;
            } else {
                chosen_filter = &other_filter;
                chosen_tstamp = other_tstamp;
                chosen_offset = &other_offset;
                chosen_tstamp_index = &other_tstamp_index;
                chosen_node = other;
                nsave += 1;
            }

            if (i > 0 && tstamps[0].time - chosen_tstamp.time >= 65535) {
                last = true;
                chosen_tstamp.time = tstamps[0].time - 65535;
            }

            tstamps[i] = chosen_tstamp.time;
            ++i;
            *chosen_tstamp_index += 1;
            int sz = entry_size(sizer, get_entry(chosen_node, *chosen_offset));
            *chosen_offset += sz;
            total_alloc_size += sz;
            press_offset(sizer, chosen_node, chosen_filter, chosen_offset, chosen_tstamp_index);
        }

        // Compute the rest of total_alloc_size.
        and_no_deletes_t filter_with_no_deletes(filter);
        while (node_offset < bs) {
            int sz = entry_size(sizer, get_entry(node, node_offset));
            node_offset += sz;
            total_alloc_size += sz;
            press_offset(sizer, node, &filter_with_no_deletes, &node_offset, &node_tstamp_index);
        }

        and_no_deletes_t other_filter_with_no_deletes(&other_filter);
        while (other_offset < bs) {
            int sz = entry_size(sizer, get_entry(other, other_offset));
            other_offset += sz;
            total_alloc_size += sz;
            press_offset(sizer, other, &other_filter_with_no_deletes, &other_offset, &other_tstamp_index);
        }
        num_tstamps_both_nodes = i;
    }
    // num_tstamps_both_nodes is the number of tstamps from both nodes, nsave is the number
    // of tstamps that remain from other.

    unsorted_garbage_collect(sizer, other, nsave - 1);

    // Now that other is garbage collected, we know everything will
    // fit, and a merge algorithm will work fine.

    int final_frontmost = bs - total_alloc_size;
    int write_offset = final_frontmost;

    {
        int i = 0;
        int node_offset = node->frontmost, other_offset = other->frontmost;
        int node_tstamp_index = -1, other_tstamp_index = -1;
        uint16_t *pair_offsets_writer = other->pair_offsets;

        press_offset(sizer, node, filter, &node_offset, &node_tstamp_index);
        press_offset(sizer, other, &other_filter, &other_offset, &other_tstamp_index);

        bool last = false;

        repli_timestamp_t first_tstamp;

        while (!last && node_tstamp_index < node->nextra && other_tstamp_index < other->nextra && i != 1 + MANDATORY_EXTRAS) {
            repli_timestamp_t node_tstamp = get_timestamp(node, node_tstamp_index);
            repli_timestamp_t other_tstamp = get_timestamp(other, other_tstamp_index);

            key_filter_t *chosen_filter;
            repli_timestamp_t chosen_tstamp;
            int *chosen_offset, *chosen_tstamp_index;
            loof_t *chosen_node;

            if (node_tstamp > other_tstamp) {
                chosen_filter = filter;
                chosen_tstamp = node_tstamp;
                chosen_offset = &node_offset;
                chosen_tstamp_index = &node_tstamp_index;
                chosen_node = node;
            } else {
                chosen_filter = &other_filter;
                chosen_tstamp = other_tstamp;
                chosen_offset = &other_offset;
                chosen_tstamp_index = &other_tstamp_index;
                chosen_node = other;
            }

            if (i == 0) {
                first_tstamp = chosen_tstamp.time;
            } else if (first_tstamp.time - chosen_tstamp.time >= 65535) {
                last = true;
                chosen_tstamp.time = tstamps[0].time - 65535;
            }

            ++i;
            *chosen_tstamp_index += 1;
            entry_t *entry = get_entry(chosen_node, *chosen_offset);
            int sz = entry_size(sizer, entry);

            move_entries_move_helper(other, write_offset, chosen_node, *chosen_offset, sz);

            if (entry_is_live(entry)) {
                *pair_offsets_writer = write_offset;
                ++pair_offsets_writer;
            }

            *chosen_offset += sz;
            write_offset += sz;

            press_offset(sizer, chosen_node, chosen_filter, chosen_offset, chosen_tstamp_index);
        }

        // Copy the rest of the elements.
        and_no_deletes_t filter_with_no_deletes(filter);
        while (node_offset < bs) {
            int sz = entry_size(sizer, get_entry(node, node_offset));
            move_entries_move_helper(other, write_offset, node, node_offset, sz);

            *pair_offsets_writer = write_offset;
            ++pair_offsets_writer;

            node_offset += sz;
            write_offset += sz;

            press_offset(sizer, node, &filter_with_no_deletes, &node_offset, &node_tstamp_index);
        }

        and_no_deletes_t other_filter_with_no_deletes(&other_filter);
        while (other_offset < bs) {
            int sz = entry_size(sizer, get_entry(other, other_offset));
            move_entries_move_helper(other, write_offset, other, other_offset, sz);

            *pair_offsets_writer = write_offset;
            ++pair_offsets_writer;

            other_offset += sz;
            write_offset += sz;

            press_offset(sizer, other, &other_filter_with_no_deletes, &other_offset, &other_tstamp_index);
        }
    }

    other->frontmost = final_frontmost;
    other->npairs = pair_offsets_writer - other->pair_offsets;

    if (num_tstamps_both_nodes > 0) {
        other->base_tstamp = tstamps[0];
        other->nextra = num_tstamps_both_nodes - 1;

        uint16_t *half_tstamp_writer = reinterpret_cast<uint16_t *>(pair_offsets_writer);

        for (int i = 1; i < num_tstamps_both_nodes; ++i) {
            *half_tstamp_writer = tstamps[0].time - tstamps[i].time;
            ++half_tstamp_writer;
        }
    }

    sort_pair_offsets(other);
}

// We only split when block_size - node->frontmost is greater than the
// max key-value-pair size.  So there's always at least one element to
// put in each side-node.
//
// median_out points to a buffer big enough to hold any key.  It gets
// set to the largest key remaining in node.
//
// rnode is uninitialized.
template <class V>
void split(value_sizer_t<V> *sizer, loof_t *node, loof_t *rnode, btree_key_t *median_out) {
    rassert(node != rnode);

    int livesize = live_size(sizer, node);

    int x = 0;
    int y = 0;
    int i = 0;
    while (y < livesize / 2 && i < node->npairs) {
        x = y;
        y += entry_size(sizer, get_entry(node, node->pair_offsets[i]));
        ++i;
    }

    // Now we could split after the first i entries (with size y) or
    // after the first i-1 entries (with size x).

    rassert(livesize / 2 > x);
    rassert(y >= livesize / 2);
    rassert(x != 0);
    rassert(y != 0);

    int s = livesize / 2 - x < y - livesize / 2 ? i - 1 : i;

    rassert(s > 0);
    rassert(s < node->npairs);

    init(sizer.block_size(), rnode);

    key_at_least_t filter(entry_key(get_entry(node, node->pair_offsets[s])));
    move_entries_into_other_node(sizer, node, rnode, &filter);
    int orig_npairs = node->npairs;
    node->npairs = s;
    memmove(node->pair_offsets + node->npairs, node->pair_offsets + orig_npairs, sizeof(uint16_t) * node->nextra);

    // Maybe we should just preemptively garbage collect node right now.

    keycpy(median_out, entry_key(get_entry(node, node->pair_offsets[s - 1])));
}

// Merges the contents of left into right.
template <class V>
void merge(value_sizer_t<V> *sizer, loof_t *left, loof_t *right, btree_key_t *key_to_remove_out) {
    allow_everything_t filter;

    move_entries_into_other_node(sizer, left, right, filter);

    keycpy(key_to_remove_out, entry_key(get_entry(right, right->pair_offsets[0])));
}











}  // namespace loof


#endif  BTREE_LOOF_NODE_HPP_
