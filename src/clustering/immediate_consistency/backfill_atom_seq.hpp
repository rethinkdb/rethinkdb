#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_HPP_

/* A `backfill_atom_seq_t` contains all of the `backfill_{pre_}atom_t`s in some range of
the key-space. */

template<class atom_t>
class backfill_atom_seq_t {
public:
    /* Initializes an undefined seq that cannot be used for anything. */
    backfill_atom_seq_t() { }

    /* Initializes an empty seq with a zero-width region at the given location */
    backfill_atom_seq_t(
            uint64_t _beg_hash, uint64_t _end_hash, key_range_t::right_bound_t key) :
        beg_hash(_beg_hash), end_hash(_end_hash), left_key(key), right_key(key),
        mem_size(0) { }

    key_range_t::right_bound_t get_left_key() const { return left_key; }
    key_range_t::right_bound_t get_right_key() const { return right_key; }
    size_t get_mem_size() const { return mem_size; }

    region_t get_region() const {
        if (left_key == right_key) {
            return region_t::empty();
        } else {
            key_range_t kr;
            kr.left = left_key.key;
            kr.right = right_key;
            return region_t(beg_hash, end_hash, kr);
        }
    }

    /* Appends an atom to the end of the seq. Atoms must be appended in lexicographical
    order. */
    void push_back(atom_t &&atom) {
        region_t atom_region = atom.get_region();
        guarantee(atom_region.beg == beg_hash && atom_region.end == end_hash);
        guarantee(key_range_t::right_bound_t(atom_region.inner.left) >= right_key);
        right_key = atom_region.inner.right;
        mem_size += atom.get_mem_size();
        atoms.push_back(std::move(atom));
    }

    /* Indicates that there are no more atoms until the given key. */
    void push_back_nothing(const key_range_t::right_bound_t &bound) {
        guarantee(bound >= right_key);
        right_key = bound;
    }

    /* Concatenates two `backfill_atom_seq_t`s. They must be adjacent. */
    void concat(backfill_atom_seq_t &&other) {
        guarantee(beg_hash == other.beg_hash && end_hash == other.end_hash);
        guarantee(right_key == other.left_key);
        right_key = other.right_key;
        mem_size += other.mem_size;
        atoms.splice(atoms.end(), std::move(other.atoms));
    }

private:
    /* A `backfill_atom_seq_t` has a `region_t`, with one oddity: even when the region
    is empty, the `backfill_atom_seq_t` still has a meaningful left and right bound. This
    is why this is stored as four separate variables instead of a `region_t`. We use two
    `key_range_t::right_bound_t`s instead of a `key_range_t` so that we can represent
    a zero-width region after the last key. */
    uint64_t beg_hash, end_hash;
    key_range_t::right_bound_t left_key, right_key;

    /* The cumulative byte size of the atoms (i.e. the sum of `a.get_mem_size()` over all
    the atoms) */
    size_t mem_size;

    std::list<atom_t> atoms;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_HPP_ */

