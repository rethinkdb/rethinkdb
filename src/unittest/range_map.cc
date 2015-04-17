// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include <deque>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/range_map.hpp"
#include "utils.hpp"

namespace unittest {

class test_range_map_t {
public:
    test_range_map_t() { }
    test_range_map_t(int i) : inner(i), ref_offset(i) {
        validate();
    }
    test_range_map_t(int l, int r, char v) :
        inner(l, r, char(v)), ref_offset(l), ref(r - l, v)
    {
        validate();
    }
    int left_edge() const {
        int inner_l = inner.left_edge();
        EXPECT_EQ(ref_offset, inner_l);
        return inner_l;
    }
    int right_edge() const {
        int inner_r = inner.right_edge();
        EXPECT_EQ(ref_offset + ref.size(), inner_r);
        return inner_r;
    }
    bool empty_domain() const {
        bool inner_empty = inner.empty_domain();
        EXPECT_EQ(ref.empty(), inner_empty);
        return inner_empty;
    }
    char lookup(int before_point) const {
        char inner_res = inner.lookup(before_point);
        char ref_res = ref.at(before_point - ref_offset);
        EXPECT_EQ(ref_res, inner_res);
        return inner_res;
    }
    template<class callable_t>
    void visit(int l, int r, const callable_t &cb) const {
        int cursor = l;
        boost::optional<char> prev;
        inner.visit(l, r, [&](int l2, int r2, char v) {
            EXPECT_EQ(cursor, l2);
            cursor = r2;
            if (static_cast<bool>(prev)) {
                EXPECT_NE(*prev, v);
            }
            prev = boost::make_optional(v);
            for (int i = l2; i < r2; ++i) {
                EXPECT_EQ(ref.at(i - ref_offset), v);
            }
            cb(l2, r2, v);
        });
        EXPECT_EQ(r, cursor);
    }
    template<class callable_t>
    test_range_map_t map(int l, int r, const callable_t &cb) const {
        test_range_map_t res;
        res.inner = inner.map(l, r, cb);
        res.ref_offset = l;
        for (int i = l; i < r; ++i) {
            res.ref.push_back(cb(ref.at(i - ref_offset)));
        }
        res.validate();
        return res;
    }
    template<class callable_t>
    test_range_map_t map_multi(int l, int r, const callable_t &cb) const {
        test_range_map_t res;
        res.inner = inner.map_multi(l, r, cb);
        res.ref_offset = l;
        for (int i = l; i < r;) {
            int l2 = i;
            while (i < r && ref.at(i - ref_offset) == ref.at(l2 - ref_offset)) ++i;
            int r2 = i;
            range_map_t<int, char> chunk = cb(l2, r2, ref.at(l2 - ref_offset));
            for (int j = l2; j < r2; ++j) {
                res.ref.push_back(chunk.lookup(j));
            }
        }
        res.validate();
        return res;
    }
    test_range_map_t mask(int l, int r) const {
        test_range_map_t res;
        res.inner = inner.mask(l, r);
        res.ref_offset = l;
        res.ref = std::deque<char>(
            ref.begin() + (l - ref_offset),
            ref.begin() + (r - ref_offset));
        res.validate();
        return res;
    }
    void extend_right(test_range_map_t &&other) {
        inner.extend_right(std::move(other.inner));
        ref.insert(ref.end(), other.ref.begin(), other.ref.end());
        validate();
    }
    void extend_right(int l, int r, char v) {
        inner.extend_right(l, r, char(v));
        while (l++ < r) {
            ref.push_back(v);
        }
        validate();
    }
    void extend_left(test_range_map_t &&other) {
        inner.extend_left(std::move(other.inner));
        ref_offset -= other.ref.size();
        ref.insert(ref.begin(), other.ref.begin(), other.ref.end());
        validate();
    }
    void extend_left(int l, int r, char v) {
        inner.extend_left(l, r, char(v));
        while (r-- > l) {
            ref.push_front(v);
        }
        ref_offset = l;
        validate();
    }
    void update(test_range_map_t &&other) {
        inner.update(std::move(other.inner));
        for (int i = other.ref_offset;
                i < other.ref_offset + static_cast<int>(other.ref.size());
                ++i) {
            ref.at(i - ref_offset) = other.ref.at(i - other.ref_offset);
        }
        validate();
    }
    void update(int l, int r, char v) {
        inner.update(l, r, char(v));
        for (int i = l; i < r; ++i) {
            ref.at(i - ref_offset) = v;
        }
        validate();
    }
    template<class callable_t>
    void visit_mutable(int l, int r, const callable_t &cb) {
        int cursor = l;
        boost::optional<char> prev;
        inner.visit_mutable(l, r, [&](int l2, int r2, char *v) {
            EXPECT_EQ(cursor, l2) << "visit_mutable() ranges should be adjacent";
            cursor = r2;
            if (static_cast<bool>(prev)) {
                EXPECT_NE(*prev, *v) << "adjacent ranges should be coalesced";
            }
            prev = boost::make_optional(*v);
            for (int i = l2; i < r2; ++i) {
                EXPECT_EQ(ref.at(i - ref_offset), *v);
            }
            cb(l2, r2, v);
            for (int i = l2; i < r2; ++i) {
                ref.at(i - ref_offset) = *v;
            }
        });
        EXPECT_EQ(r, cursor);
        validate();
    }
    void validate() const {
        bool ok = inner.left_edge() == ref_offset;
        ok = ok && (inner.right_edge() == ref_offset + static_cast<int>(ref.size()));
        inner.visit(inner.left_edge(), inner.right_edge(), [&](int l, int r, char v) {
            for (int i = l; i < r; ++i) {
                ok = ok && (ref.at(i - ref_offset) == v);
            }
        });
        if (!ok) {
            std::string inner_str;
            inner.visit(inner.left_edge(), inner.right_edge(),
                [&](int l, int r, char v) {
                    inner_str += std::string(r - l, v);
                });
            ADD_FAILURE() << "inner/ref mismatch:\n"
                << "inner: " << inner.left_edge()
                    << " '" << inner_str << "' " << inner.right_edge() << "\n"
                << "  ref: " << ref_offset
                    << " '" << as_str() << "' " << ref_offset + ref.size();
        }
    }
    std::string as_str() const {
        return std::string(ref.begin(), ref.end());
    }
    range_map_t<int, char> inner;
    int ref_offset;
    std::deque<char> ref;
};

/* Use a small set of possible chars so that lots of coalescing will happen. */
char rand_char() {
    return 'a' + randint(4);
}

test_range_map_t rand_map(int l, int r) {
    test_range_map_t test(l);
    while (l < r) {
        test.extend_right(l, l + 1, rand_char());
        ++l;
    }
    return test;
}

TEST(RangeMap, RangeMap) {
    test_range_map_t test = rand_map(50, 100);
    for (int repeat = 0; repeat < 1000; ++repeat) {
        int opcode = randint(13);
        int l = test.left_edge() + randint(test.right_edge() - test.left_edge() + 1);
        int r = l + randint(test.right_edge() - l + 1);
        char c = rand_char();
        if (opcode == 0) {
            if (l != test.right_edge()) {
                test.lookup(l);
            }
        } else if (opcode == 1) {
            test.visit(l, r, [&](int, int, char) { });
        } else if (opcode == 2) {
            test = test.map(l, r, [&](char v) -> char { return v + 1; });
        } else if (opcode == 3) {
            test = test.map_multi(l, r, [&](int l2, int r2, char v) {
                range_map_t<int, char> chunk(l2, r2, char(v));
                chunk.update(l2, l2 + 1, v + 1);
                return chunk;
            });
        } else if (opcode == 4) {
            test = test.mask(l, r);
        } else if (opcode == 5) {
            test.extend_right(
                rand_map(test.right_edge(), test.right_edge() + randint(5)));
        } else if (opcode == 6) {
            int len = randint(5);
            test.extend_right(test.right_edge(), test.right_edge() + len, c);
        } else if (opcode == 7) {
            test.extend_left(
                rand_map(test.left_edge() - randint(5), test.left_edge()));
        } else if (opcode == 8) {
            int len = randint(5);
            test.extend_left(test.left_edge() - len, test.left_edge(), c);
        } else if (opcode == 9) {
            test.update(rand_map(l, r));
        } else if (opcode == 10) {
            test.update(l, r, c);
        } else if (opcode == 11) {
            test.visit_mutable(l, r, [&](int, int, char *v) {
                *v += 1;
            });
        } else if (opcode == 12) {
            test.empty_domain();
        } else {
            unreachable();
        }
    }
}

} /* namespace unittest */

