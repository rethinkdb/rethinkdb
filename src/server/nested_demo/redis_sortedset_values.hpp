#ifndef __SERVER_NESTED_DEMO_REDIS_SORTEDSET_VALUES_HPP__
#define	__SERVER_NESTED_DEMO_REDIS_SORTEDSET_VALUES_HPP__

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include "server/nested_demo/redis_utils.hpp"
#include "btree/iteration.hpp"
#include "config/args.hpp"


struct redis_nested_float_value_t {
    float value;
} __attribute__((__packed__));
template <>
class value_sizer_t<redis_nested_float_value_t> {
public:
    value_sizer_t<redis_nested_float_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(UNUSED const redis_nested_float_value_t *value) const {
        return sizeof(float);
    }

    bool fits(UNUSED const redis_nested_float_value_t *value, UNUSED int length_available) const {
        return length_available >= static_cast<int>(sizeof(float));
    }

    int max_possible_size() const {
        return sizeof(float);
    }

    static block_magic_t btree_leaf_magic() {
        block_magic_t magic = { { 'l', 'r', 'n', 'f' } }; // leaf redis nested float
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

struct redis_sortedset_value_t {
    /*
    The member_score btree maps from the member to a redis_nested_float_value_t.
    The score_member btree maps from a concatenation of score (in lexicographical representation) and
    member to an empty value.
     */
    block_id_t member_score_nested_root; // nested tree from member to score
    block_id_t score_member_nested_root; // nested tree from score to member
    // TODO! We need the third subtree size nested tree!
    uint32_t size;

public:
    int inline_size(UNUSED block_size_t bs) const {
        return sizeof(member_score_nested_root) + sizeof(score_member_nested_root) + sizeof(size);
    }

    int64_t value_size() const {
        return 0;
    }

    const char *value_ref() const { return NULL; }
    char *value_ref() { return NULL; }


    /* Some operations that you can do on a hash (resembling redis commands)... */

    // For checking if a member is too long
    bool does_member_fit(const std::string &member) const {
        // We construct concatenations of a lex float and the actual member, so that has to fit
        return member.length() + redis_utils::LEX_FLOAT_SIZE <= MAX_KEY_SIZE;
    }

    int zcard() const;

    // Returns the score if member exists
    boost::optional<float> zscore(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member) const;

    // TODO! This has O(n) runtime right now!
    boost::optional<int> zrank(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread, const std::string &member) const;

    // returns true if a member has been deleted (i.e. existed in the set)
    bool zrem(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member);

    // Returns true iff the member was new
    bool zadd(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, const std::string &member, float score);

    // Deletes all elements from the subtree. Must be called before deleting the set value in the super tree!
    void clear(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread);

    // Provides an iterator over all member-score pairs starting at the start-th member.
    // Results are sorted properly (primarily by score, secondarily by the member in lexicographical order)
    // From the redis documentation:
    /*
     Both start and stop are zero-based indexes, where 0 is the first element, 1
     is the next element and so on. They can also be negative numbers indicating
     offsets from the end of the sorted set, with -1 being the last element of
     the sorted set, -2 the penultimate element and so on.
    */
    // TODO! This has O(n) runtime right now!
    boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > zrange(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread, int start) const;

    /* (can also trivially be used to implement zcount) */
    boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > zrangebyscore(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread, rget_bound_mode_t min_mode, float min, rget_bound_mode_t max_mode, float max) const;

    /* This is equivalent to zrange with start == 0. It might be faster though, and is meant for internal operations. */
    boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > get_full_iter(value_sizer_t<redis_sortedset_value_t> *super_sizer, boost::shared_ptr<transaction_t> transaction, int slice_home_thread) const;

    /*
     Set operations can be implemented on top of get_full_iter, and this is how:
     TODO!
     (we need special iterators for this, because merging two members with different scores adds
     complexity over the implementation for plain sets. Specifically, two members with different scores
     have to be treated as equal, but their scores have to be merged in certain ways (summing them up
     or taking the maximum or mininum).
     */


private:
    // Transforms from key_value_pair_t<redis_nested_empty_value_t> to pairs of
    // member and score, where the input keys should be generated using make_score_member_key
    std::pair<float, std::string> transform_value(const key_value_pair_t<redis_nested_empty_value_t> &pair) const {
        return decode_score_member_key(pair.key);
    }

    // Build a concatenation of score and member, such that we have the following ordering property
    // under lexicographical ordering:
    // - primaryily sorted by score in ascending order
    // - secondarily sorted by member in ascending order
    std::string make_score_member_key(const float score, const std::string &member) const {
        std::string key;
        key.reserve(redis_utils::LEX_FLOAT_SIZE + member.length());
        char lex_score[redis_utils::LEX_FLOAT_SIZE];
        redis_utils::to_lex_float(score, lex_score);
        key.assign(lex_score, redis_utils::LEX_FLOAT_SIZE);
        key.append(member);
        return key;
    }

    // Decodes a concatenated score-member key
    std::pair<float, std::string> decode_score_member_key(const std::string &key) const {
        rassert(key.length() >= redis_utils::LEX_FLOAT_SIZE);
        const float score = redis_utils::from_lex_float(key.data());
        const std::string member = key.substr(redis_utils::LEX_FLOAT_SIZE);
        return std::pair<float, std::string>(score, member);
    }
} __attribute__((__packed__));

template <>
class value_sizer_t<redis_sortedset_value_t> {
public:
    value_sizer_t<redis_sortedset_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_sortedset_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(const redis_sortedset_value_t *value, int length_available) const {
        return size(value) <= length_available;
    }

    int max_possible_size() const {
        // It's of constant size...
        return sizeof(redis_sortedset_value_t);
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'r', 'e', 'z' } }; // Leaf REdis sorted set
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

#endif	/* __SERVER_NESTED_DEMO_REDIS_SORTEDSET_VALUES_HPP__ */
