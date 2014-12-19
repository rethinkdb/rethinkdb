// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/serialize_datum.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "containers/archive/buffer_stream.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "containers/counted.hpp"
#include "containers/shared_buffer.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {


enum class datum_serialized_type_t {
    R_ARRAY = 1,
    R_BOOL = 2,
    R_NULL = 3,
    DOUBLE = 4,
    R_OBJECT = 5,
    R_STR = 6,
    INT_NEGATIVE = 7,
    INT_POSITIVE = 8,
    R_BINARY = 9,
    BUF_R_ARRAY = 10,
    BUF_R_OBJECT = 11
};

// Objects and arrays use different word sizes for storing offsets,
// depending on the total serialized size of the datum. These are the options.
enum class datum_offset_size_t {
    U8BIT,
    U16BIT,
    U32BIT,
    U64BIT
};

// For efficiency reasons, we pre-compute the serialized sizes of the elements
// of arrays and objects during serialization. This avoids recomputing them,
// which in the worst-case could lead to quadratic serialization costs.
// This is the data structure in which we store the sizes.
struct size_tree_node_t {
    size_t size;
    std::vector<size_tree_node_t> child_sizes;
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(datum_serialized_type_t, int8_t,
                                      datum_serialized_type_t::R_ARRAY,
                                      datum_serialized_type_t::BUF_R_OBJECT);

serialization_result_t datum_serialize(write_message_t *wm,
                                       datum_serialized_type_t type) {
    serialize<cluster_version_t::LATEST_OVERALL>(wm, type);
    return serialization_result_t::SUCCESS;
}

MUST_USE archive_result_t datum_deserialize(read_stream_t *s,
                                            datum_serialized_type_t *type) {
    return deserialize<cluster_version_t::LATEST_OVERALL>(s, type);
}

/* Forward declarations */
size_t datum_serialized_size(const datum_t &datum,
                             check_datum_serialization_errors_t check_errors,
                             std::vector<size_tree_node_t> *child_sizes_out);
serialization_result_t datum_serialize(
        write_message_t *wm,
        const datum_t &datum,
        check_datum_serialization_errors_t check_errors,
        const size_tree_node_t &precomputed_size);

// Some of the following looks like it duplicates code of other deserialization
// functions.  It does. Keeping this separate means that we don't have to worry
// about whether datum serialization has changed from cluster version to cluster
// version.

/* Helper functions shared by datum_array_* and datum_object_* */

// Keep in sync with offset_table_serialized_size
void serialize_offset_table(write_message_t *wm,
                            datum_t::type_t datum_type,
                            const std::vector<size_tree_node_t> &elem_sizes,
                            datum_offset_size_t offset_size) {
    rassert(datum_type == datum_t::R_OBJECT || datum_t::R_ARRAY);
    const size_t num_elements =
        datum_type == datum_t::R_OBJECT
        ? elem_sizes.size() / 2
        : elem_sizes.size();


    // num_elements
    serialize_varint_uint64(wm, num_elements);

    // the offset table
    size_t next_offset = 0;
    for (size_t i = 1; i < num_elements; ++i) {
        if (datum_type == datum_t::R_OBJECT) {
            next_offset += elem_sizes[(i-1)*2].size; // The key
            next_offset += elem_sizes[(i-1)*2+1].size; // The value
        } else {
            next_offset += elem_sizes[i-1].size;
        }
        switch (offset_size) {
        case datum_offset_size_t::U8BIT:
            guarantee(next_offset <= std::numeric_limits<uint8_t>::max());
            serialize_universal(wm, static_cast<uint8_t>(next_offset));
            break;
        case datum_offset_size_t::U16BIT:
            guarantee(next_offset <= std::numeric_limits<uint16_t>::max());
            serialize_universal(wm, static_cast<uint16_t>(next_offset));
            break;
        case datum_offset_size_t::U32BIT:
            guarantee(next_offset <= std::numeric_limits<uint32_t>::max());
            serialize_universal(wm, static_cast<uint32_t>(next_offset));
            break;
        case datum_offset_size_t::U64BIT:
            guarantee(next_offset <= std::numeric_limits<uint64_t>::max());
            serialize_universal(wm, static_cast<uint64_t>(next_offset));
            break;
        default:
            unreachable();
        }
    }
}

// Keep in sync with offset_table_serialized_size
datum_offset_size_t get_offset_size_from_inner_size(uint64_t inner_size) {
    if (inner_size <= std::numeric_limits<uint8_t>::max()) {
        return datum_offset_size_t::U8BIT;
    } else if (inner_size <= std::numeric_limits<uint16_t>::max()) {
        return datum_offset_size_t::U16BIT;
    } else if (inner_size <= std::numeric_limits<uint32_t>::max()) {
        return datum_offset_size_t::U32BIT;
    } else if (inner_size <= std::numeric_limits<uint64_t>::max()) {
        return datum_offset_size_t::U64BIT;
    }
    unreachable();
}

size_t read_inner_serialized_size_from_buf(const shared_buf_ref_t<char> &buf) {
    buffer_read_stream_t s(buf.get(), buf.get_safety_boundary());
    uint64_t sz = 0;
    guarantee_deserialization(deserialize_varint_uint64(&s, &sz), "datum buffer size");
    guarantee(sz <= std::numeric_limits<size_t>::max());
    return static_cast<size_t>(sz);
}

// Keep in sync with serialize_offset_table
size_t offset_table_serialized_size(size_t num_elements,
                                    size_t remaining_inner_size,
                                    datum_offset_size_t *offset_size_out) {
    rassert(offset_size_out != NULL);

    // The size of num_elements
    size_t num_elements_sz = varint_uint64_serialized_size(num_elements);

    // The size of the offset table
    if (num_elements > 1) {
        // We pick the bit width of the offset table based on the total inner
        // serialized size of the object, that is num_elements_sz +
        // remaining_inner_size + offsets_sz.
        // The advantage of that is that we can later determine the offset size
        // just from looking at the inner size (which we serialize explicitly).

        size_t offsets_sz;
        // Try 8 bit offsets first
        *offset_size_out = datum_offset_size_t::U8BIT;
        offsets_sz = (num_elements - 1) * serialize_universal_size_t<uint8_t>::value;
        // Do we have to raise to 16 bit?
        if (remaining_inner_size + num_elements_sz + offsets_sz >
                std::numeric_limits<uint8_t>::max()) {
            *offset_size_out = datum_offset_size_t::U16BIT;
            offsets_sz = (num_elements - 1) * serialize_universal_size_t<uint16_t>::value;
        }
        // Do we have to raise to 32 bit?
        if (remaining_inner_size + num_elements_sz + offsets_sz >
                std::numeric_limits<uint16_t>::max()) {
            *offset_size_out = datum_offset_size_t::U32BIT;
            offsets_sz = (num_elements - 1) * serialize_universal_size_t<uint32_t>::value;
        }
        // Do we have to raise to 64 bit?
        if (remaining_inner_size + num_elements_sz + offsets_sz >
                std::numeric_limits<uint32_t>::max()) {
            *offset_size_out = datum_offset_size_t::U64BIT;
            offsets_sz = (num_elements - 1) * serialize_universal_size_t<uint64_t>::value;
        }
        return num_elements_sz + offsets_sz;
    } else {
        *offset_size_out = datum_offset_size_t::U8BIT;
        return num_elements_sz;
    }
}

// Keep in sync with datum_object_serialize
// Keep in sync with datum_array_serialize
size_t datum_array_inner_serialized_size(
        const datum_t &datum,
        const std::vector<size_tree_node_t> &child_sizes,
        datum_offset_size_t *offset_size_out) {

    // The size of all (keys/)values
    size_t elem_sz = 0;
    for (size_t i = 0; i < child_sizes.size(); ++i) {
        elem_sz += child_sizes[i].size;
    }

    // Plus the offset table size
    const size_t num_elements =
        datum.get_type() == datum_t::R_OBJECT
        ? datum.obj_size()
        : datum.arr_size();
    return elem_sz + offset_table_serialized_size(num_elements,
                                                  elem_sz,
                                                  offset_size_out);
}


// Keep in sync with datum_array_serialize.
size_t datum_array_serialized_size(const datum_t &datum,
                                   check_datum_serialization_errors_t check_errors,
                                   std::vector<size_tree_node_t> *element_sizes_out) {
    size_t sz = 0;

    // Can we use an existing serialization?
    const shared_buf_ref_t<char> *existing_buf_ref = datum.get_buf_ref();
    if (existing_buf_ref != NULL
        && check_errors == check_datum_serialization_errors_t::NO) {

        // We don't initialize element_sizes_out, but that's ok. We don't need it
        // if there already is a serialization.
        sz += read_inner_serialized_size_from_buf(*existing_buf_ref);
    } else {
        std::vector<size_tree_node_t> elem_sizes;
        elem_sizes.reserve(datum.arr_size());
        for (size_t i = 0; i < datum.arr_size(); ++i) {
            auto elem = datum.get(i);
            size_tree_node_t elem_size;
            elem_size.size = datum_serialized_size(elem, check_errors,
                                                   &elem_size.child_sizes);
            elem_sizes.push_back(std::move(elem_size));
        }
        datum_offset_size_t offset_size;
        sz += datum_array_inner_serialized_size(datum, elem_sizes, &offset_size);

        if (element_sizes_out != NULL) {
            *element_sizes_out = std::move(elem_sizes);
        }
    }

    // The inner serialized size
    sz += varint_uint64_serialized_size(sz);

    return sz;
}


// Keep in sync with datum_array_serialized_size.
// Keep in sync with datum_get_element_offset.
// Keep in sync with datum_get_array_size.
serialization_result_t datum_array_serialize(
        write_message_t *wm,
        const datum_t &datum,
        check_datum_serialization_errors_t check_errors,
        const size_tree_node_t &precomputed_sizes) {

    // Can we use an existing serialization?
    const shared_buf_ref_t<char> *existing_buf_ref = datum.get_buf_ref();
    if (existing_buf_ref != NULL
        && check_errors == check_datum_serialization_errors_t::NO) {

        // Subtract 1 for the type byte, which we don't have to rewrite
        wm->append(existing_buf_ref->get(), precomputed_sizes.size - 1);
        return serialization_result_t::SUCCESS;
    }

    // The inner serialized size
    datum_offset_size_t offset_size;
    serialize_varint_uint64(wm,
        datum_array_inner_serialized_size(datum, precomputed_sizes.child_sizes,
                                          &offset_size));

    serialize_offset_table(wm, datum.get_type(), precomputed_sizes.child_sizes,
                           offset_size);

    // The elements
    serialization_result_t res = serialization_result_t::SUCCESS;
    rassert(precomputed_sizes.child_sizes.size() == datum.arr_size());
    for (size_t i = 0; i < datum.arr_size(); ++i) {
        auto elem = datum.get(i);
        const size_tree_node_t &child_size = precomputed_sizes.child_sizes[i];
        res = res | datum_serialize(wm, elem, check_errors, child_size);
    }

    return res;
}

// For legacy R_ARRAY datums. BUF_R_ARRAY datums are not deserialized through this.
MUST_USE archive_result_t
datum_deserialize(read_stream_t *s, std::vector<datum_t> *v) {
    v->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (bad(res)) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    v->resize(sz);
    for (uint64_t i = 0; i < sz; ++i) {
        res = datum_deserialize(s, &(*v)[i]);
        if (bad(res)) { return res; }
    }

    return archive_result_t::SUCCESS;
}

// Keep in sync with datum_object_serialize.
size_t datum_object_serialized_size(const datum_t &datum,
                                    check_datum_serialization_errors_t check_errors,
                                    std::vector<size_tree_node_t> *child_sizes_out) {
    size_t sz = 0;

    // Can we use an existing serialization?
    const shared_buf_ref_t<char> *existing_buf_ref = datum.get_buf_ref();
    if (existing_buf_ref != NULL
        && check_errors == check_datum_serialization_errors_t::NO) {

        // We don't initialize element_sizes_out, but that's ok. We don't need it
        // if there already is a serialization.
        sz += read_inner_serialized_size_from_buf(*existing_buf_ref);
    } else {
        std::vector<size_tree_node_t> child_sizes;
        child_sizes.reserve(datum.obj_size() * 2);
        for (size_t i = 0; i < datum.obj_size(); ++i) {
            auto pair = datum.get_pair(i);
            size_tree_node_t key_size;
            key_size.size = datum_serialized_size(pair.first);
            size_tree_node_t val_size;
            val_size.size = datum_serialized_size(pair.second, check_errors,
                                                  &val_size.child_sizes);
            child_sizes.push_back(std::move(key_size));
            child_sizes.push_back(std::move(val_size));
        }
        datum_offset_size_t offset_size;
        sz += datum_array_inner_serialized_size(datum, child_sizes, &offset_size);

        if (child_sizes_out != NULL) {
            *child_sizes_out = std::move(child_sizes);
        }
    }

    // The inner serialized size
    sz += varint_uint64_serialized_size(sz);

    return sz;
}

// Keep in sync with datum_object_serialized_size.
// Keep in sync with datum_get_element_offset.
// Keep in sync with datum_get_array_size.
// Keep in sync with datum_deserialize_pair_from_buf.
serialization_result_t datum_object_serialize(
        write_message_t *wm,
        const datum_t &datum,
        check_datum_serialization_errors_t check_errors,
        const size_tree_node_t &precomputed_sizes) {

    // Can we use an existing serialization?
    const shared_buf_ref_t<char> *existing_buf_ref = datum.get_buf_ref();
    if (existing_buf_ref != NULL
        && check_errors == check_datum_serialization_errors_t::NO) {

        // Subtract 1 for the type byte, which we don't have to rewrite
        wm->append(existing_buf_ref->get(), precomputed_sizes.size - 1);
        return serialization_result_t::SUCCESS;
    }

    // The inner serialized size
    datum_offset_size_t offset_size;
    serialize_varint_uint64(wm,
        datum_array_inner_serialized_size(datum, precomputed_sizes.child_sizes,
                                          &offset_size));

    serialize_offset_table(wm, datum.get_type(), precomputed_sizes.child_sizes,
                           offset_size);

    // The pairs
    serialization_result_t res = serialization_result_t::SUCCESS;
    rassert(precomputed_sizes.child_sizes.size() == datum.obj_size() * 2);
    for (size_t i = 0; i < datum.obj_size(); ++i) {
        auto pair = datum.get_pair(i);
        const size_tree_node_t &val_size = precomputed_sizes.child_sizes[i*2+1];
        res = res | datum_serialize(wm, pair.first);
        res = res | datum_serialize(wm, pair.second, check_errors, val_size);
    }

    return res;
}

// For legacy R_OBJECT datums. BUF_R_OBJECT datums are not deserialized through this.
MUST_USE archive_result_t datum_deserialize(
        read_stream_t *s,
        std::vector<std::pair<datum_string_t, datum_t> > *m) {
    m->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (bad(res)) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    m->reserve(static_cast<size_t>(sz));

    for (uint64_t i = 0; i < sz; ++i) {
        std::pair<datum_string_t, datum_t> p;
        res = datum_deserialize(s, &p.first);
        if (bad(res)) { return res; }
        res = datum_deserialize(s, &p.second);
        if (bad(res)) { return res; }
        m->push_back(std::move(p));
    }

    return archive_result_t::SUCCESS;
}


size_t datum_serialized_size(const datum_t &datum,
                             check_datum_serialization_errors_t check_errors) {
    return datum_serialized_size(datum, check_errors, NULL);
}

size_t datum_serialized_size(const datum_t &datum,
                             check_datum_serialization_errors_t check_errors,
                             std::vector<size_tree_node_t> *child_sizes_out) {
    rassert(child_sizes_out == NULL || child_sizes_out->empty());
    r_sanity_check(datum.has());
    // Update datum_object_serialize() and datum_array_serialize() if the size of
    // the type prefix should ever change.
    size_t sz = 1; // 1 byte for the type
    switch (datum.get_type()) {
    case datum_t::R_ARRAY: {
        sz += datum_array_serialized_size(datum, check_errors, child_sizes_out);
    } break;
    case datum_t::R_BINARY: {
        sz += datum_serialized_size(datum.as_binary());
    } break;
    case datum_t::R_BOOL: {
        sz += serialize_universal_size_t<bool>::value;
    } break;
    case datum_t::R_NULL: break;
    case datum_t::R_NUM: {
        double d = datum.as_num();
        int64_t i;
        if (number_as_integer(d, &i)) {
            sz += varint_uint64_serialized_size(i < 0 ? -i : i);
        } else {
            sz += serialize_universal_size_t<double>::value;
        }
    } break;
    case datum_t::R_OBJECT: {
        sz += datum_object_serialized_size(datum, check_errors, child_sizes_out);
    } break;
    case datum_t::R_STR: {
        sz += datum_serialized_size(datum.as_str());
    } break;
    case datum_t::UNINITIALIZED: // fallthru
    default:
        unreachable();
    }
    return sz;
}

serialization_result_t datum_serialize(
        write_message_t *wm,
        const datum_t &datum,
        check_datum_serialization_errors_t check_errors,
        const size_tree_node_t &precomputed_size) {
#ifndef NDEBUG
    const size_t pre_serialization_size = wm->size();
#endif
    serialization_result_t res = serialization_result_t::SUCCESS;

    r_sanity_check(datum.has());
    switch (datum.get_type()) {
    case datum_t::R_ARRAY: {
        res = res | datum_serialize(wm, datum_serialized_type_t::BUF_R_ARRAY);
        if (datum.arr_size() > 100000)
            res = res | serialization_result_t::ARRAY_TOO_BIG;
        res = res | datum_array_serialize(wm, datum, check_errors, precomputed_size);
    } break;
    case datum_t::R_BINARY: {
        datum_serialize(wm, datum_serialized_type_t::R_BINARY);
        const datum_string_t &value = datum.as_binary();
        datum_serialize(wm, value);
    } break;
    case datum_t::R_BOOL: {
        res = res | datum_serialize(wm, datum_serialized_type_t::R_BOOL);
        bool value = datum.as_bool();
        serialize_universal(wm, value);
    } break;
    case datum_t::R_NULL: {
        res = res | datum_serialize(wm, datum_serialized_type_t::R_NULL);
    } break;
    case datum_t::R_NUM: {
        double value = datum.as_num();
        int64_t i;
        if (number_as_integer(value, &i)) {
            // We serialize the signed-zero double, -0.0, with INT_NEGATIVE.

            // so we can use `signbit` in a GCC 4.4.3-compatible way
            using namespace std;  // NOLINT(build/namespaces)
            if (signbit(value)) {
                res = res | datum_serialize(wm, datum_serialized_type_t::INT_NEGATIVE);
                serialize_varint_uint64(wm, -i);
            } else {
                res = res | datum_serialize(wm, datum_serialized_type_t::INT_POSITIVE);
                serialize_varint_uint64(wm, i);
            }
        } else {
            res = res | datum_serialize(wm, datum_serialized_type_t::DOUBLE);
            serialize_universal(wm, value);
        }
    } break;
    case datum_t::R_OBJECT: {
        res = res | datum_serialize(wm, datum_serialized_type_t::BUF_R_OBJECT);
        res = res | datum_object_serialize(wm, datum, check_errors, precomputed_size);
    } break;
    case datum_t::R_STR: {
        res = res | datum_serialize(wm, datum_serialized_type_t::R_STR);
        const datum_string_t &value = datum.as_str();
        res = res | datum_serialize(wm, value);
    } break;
    case datum_t::UNINITIALIZED: // fallthru
    default:
        unreachable();
    }

    rassert(wm->size() == pre_serialization_size + precomputed_size.size);
    return res;
}

serialization_result_t datum_serialize(
        write_message_t *wm,
        const datum_t &datum,
        check_datum_serialization_errors_t check_errors) {
    // Precompute serialized sizes
    size_tree_node_t size;
    size.size = datum_serialized_size(datum, check_errors, &size.child_sizes);

    return datum_serialize(wm, datum, check_errors, size);
}

archive_result_t datum_deserialize(read_stream_t *s, datum_t *datum) {
    // Datums on disk should always be read no matter how stupid big
    // they are; there's no way to fix the problem otherwise.
    // Similarly we don't want to reject array reads from cluster
    // nodes that are within the user spec but larger than the default
    // 100,000 limit.
    ql::configured_limits_t limits = ql::configured_limits_t::unlimited;
    datum_serialized_type_t type;
    archive_result_t res = datum_deserialize(s, &type);
    if (bad(res)) {
        return res;
    }

    switch (type) {
    case datum_serialized_type_t::R_ARRAY: {
        std::vector<datum_t> value;
        res = datum_deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            *datum = datum_t(std::move(value), limits);
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_BINARY: {
        datum_string_t value;
        res = datum_deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            *datum = datum_t::binary(std::move(value));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_BOOL: {
        bool value;
        res = deserialize_universal(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            *datum = datum_t::boolean(value);
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_NULL: {
        *datum = datum_t::null();
    } break;
    case datum_serialized_type_t::DOUBLE: {
        double value;
        res = deserialize_universal(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            *datum = datum_t(value);
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::INT_NEGATIVE:  // fall through
    case datum_serialized_type_t::INT_POSITIVE: {
        uint64_t unsigned_value;
        res = deserialize_varint_uint64(s, &unsigned_value);
        if (bad(res)) {
            return res;
        }
        if (unsigned_value > max_dbl_int) {
            return archive_result_t::RANGE_ERROR;
        }
        const double d = unsigned_value;
        double value;
        if (type == datum_serialized_type_t::INT_NEGATIVE) {
            // This might deserialize the signed-zero double, -0.0.
            value = -d;
        } else {
            value = d;
        }
        try {
            *datum = datum_t(value);
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_OBJECT: {
        std::vector<std::pair<datum_string_t, datum_t> > value;
        res = datum_deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            *datum = datum_t(std::move(value));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_STR: {
        datum_string_t value;
        res = datum_deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            *datum = datum_t(std::move(value));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::BUF_R_ARRAY: // fallthru
    case datum_serialized_type_t::BUF_R_OBJECT:
    {
        // First read the serialized size of the buffer
        uint64_t ser_size;
        res = deserialize_varint_uint64(s, &ser_size);
        if (bad(res)) {
            return res;
        }
        const size_t ser_size_sz = varint_uint64_serialized_size(ser_size);
        if (ser_size > std::numeric_limits<size_t>::max() - ser_size_sz
            || ser_size > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())
                          - ser_size_sz) {
            return archive_result_t::RANGE_ERROR;
        }

        // Then read the data into a shared_buf_t
        counted_t<shared_buf_t> buf = shared_buf_t::create(static_cast<size_t>(ser_size) + ser_size_sz);
        serialize_varint_uint64_into_buf(ser_size, reinterpret_cast<uint8_t *>(buf->data()));
        int64_t num_read = force_read(s, buf->data() + ser_size_sz, ser_size);
        if (num_read == -1) {
            return archive_result_t::SOCK_ERROR;
        }
        if (static_cast<uint64_t>(num_read) < ser_size) {
            return archive_result_t::SOCK_EOF;
        }

        // ...from which we create the datum_t
        datum_t::type_t dtype = type == datum_serialized_type_t::BUF_R_ARRAY
                                ? datum_t::R_ARRAY
                                : datum_t::R_OBJECT;
        try {
            *datum = datum_t(dtype, shared_buf_ref_t<char>(std::move(buf), 0));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    default:
        return archive_result_t::RANGE_ERROR;
    }

    return archive_result_t::SUCCESS;
}

datum_t datum_deserialize_from_buf(const shared_buf_ref_t<char> &buf, size_t at_offset) {
    // Peek into the buffer to find out the type of the datum in there.
    // If it's a string, buf_object or buf_array, we just create a datum from a
    // child buf_ref and are done.
    // Otherwise we create a buffer_read_stream_t and deserialize the datum from
    // there.
    buf.guarantee_in_boundary(at_offset);
    buffer_read_stream_t read_stream(buf.get() + at_offset,
                                     buf.get_safety_boundary() - at_offset);
    datum_serialized_type_t type = datum_serialized_type_t::R_NULL;
    guarantee_deserialization(datum_deserialize(&read_stream, &type), "datum type from buf");

    switch (type) {
    case datum_serialized_type_t::R_STR: {
        const size_t data_offset = at_offset + static_cast<size_t>(read_stream.tell());
        return datum_t(datum_string_t(buf.make_child(data_offset)));
    }
    case datum_serialized_type_t::BUF_R_ARRAY: {
        const size_t data_offset = at_offset + static_cast<size_t>(read_stream.tell());
        return datum_t(datum_t::R_ARRAY, buf.make_child(data_offset));
    }
    case datum_serialized_type_t::BUF_R_OBJECT: {
        const size_t data_offset = at_offset + static_cast<size_t>(read_stream.tell());
        return datum_t(datum_t::R_OBJECT, buf.make_child(data_offset));
    }
    case datum_serialized_type_t::R_BINARY: {
        const size_t data_offset = at_offset + static_cast<size_t>(read_stream.tell());
        return datum_t(datum_t::construct_binary_t(),
                       datum_string_t(buf.make_child(data_offset)));
    }
    case datum_serialized_type_t::R_ARRAY: // fallthru
    case datum_serialized_type_t::R_BOOL: // fallthru
    case datum_serialized_type_t::R_NULL: // fallthru
    case datum_serialized_type_t::DOUBLE: // fallthru
    case datum_serialized_type_t::R_OBJECT: // fallthru
    case datum_serialized_type_t::INT_NEGATIVE: // fallthru
    case datum_serialized_type_t::INT_POSITIVE: {
        buffer_read_stream_t data_read_stream(buf.get() + at_offset,
                                              buf.get_safety_boundary() - at_offset);
        datum_t res;
        guarantee_deserialization(datum_deserialize(&data_read_stream, &res),
                                  "datum from buf");
        return res;
    }
    default:
        unreachable();
    }
}

std::pair<datum_string_t, datum_t> datum_deserialize_pair_from_buf(
        const shared_buf_ref_t<char> &buf, size_t at_offset) {
    datum_string_t key(buf.make_child(at_offset));
    // Relies on the fact that the datum_string_t serialization format hasn't
    // changed, specifically that we would still get the same size if we re-serialized
    // the datum_string_t now.
    const size_t key_ser_size = datum_serialized_size(key);

    datum_t value(datum_deserialize_from_buf(buf, at_offset + key_ser_size));

    return std::make_pair(std::move(key), std::move(value));
}

/* The format of `array` is:
     varint ser_size
     varint num_elements
     ... */
size_t datum_get_array_size(const shared_buf_ref_t<char> &array) {
    buffer_read_stream_t sz_read_stream(array.get(), array.get_safety_boundary());
    uint64_t ser_size;
    guarantee_deserialization(deserialize_varint_uint64(&sz_read_stream, &ser_size),
                              "datum decode array");
    uint64_t num_elements = 0;
    guarantee_deserialization(deserialize_varint_uint64(&sz_read_stream, &num_elements),
                              "datum decode array");
    guarantee(num_elements <= std::numeric_limits<size_t>::max());
    return static_cast<size_t>(num_elements);
}

/* The format of `array` is:
     varint ser_size
     varint num_elements
     uint*_t offsets[num_elements - 1] // counted from `data`, first element omitted
     T data[num_elements] */
size_t datum_get_element_offset(const shared_buf_ref_t<char> &array, size_t index) {
    buffer_read_stream_t sz_read_stream(array.get(), array.get_safety_boundary());
    uint64_t ser_size = 0;
    guarantee_deserialization(deserialize_varint_uint64(&sz_read_stream, &ser_size),
                              "datum decode array");
    const datum_offset_size_t offset_size = get_offset_size_from_inner_size(ser_size);
    size_t serialized_offset_size;
    switch (offset_size) {
    case datum_offset_size_t::U8BIT:
        serialized_offset_size = serialize_universal_size_t<uint8_t>::value; break;
    case datum_offset_size_t::U16BIT:
        serialized_offset_size = serialize_universal_size_t<uint16_t>::value; break;
    case datum_offset_size_t::U32BIT:
        serialized_offset_size = serialize_universal_size_t<uint32_t>::value; break;
    case datum_offset_size_t::U64BIT:
        serialized_offset_size = serialize_universal_size_t<uint64_t>::value; break;
    default:
        unreachable();
    }

    uint64_t num_elements = 0;
    guarantee_deserialization(deserialize_varint_uint64(&sz_read_stream, &num_elements),
                              "datum decode array");
    guarantee(num_elements <= std::numeric_limits<size_t>::max());
    const size_t sz = static_cast<size_t>(num_elements);

    guarantee(index < sz);

    rassert(sz > 0);
    const size_t data_offset =
        static_cast<size_t>(sz_read_stream.tell())
        + (num_elements - 1) * serialized_offset_size;

    if (index == 0) {
        return data_offset;
    } else {
        const size_t element_offset_offset =
            static_cast<size_t>(sz_read_stream.tell())
            + (index - 1) * serialized_offset_size;

        array.guarantee_in_boundary(element_offset_offset);
        buffer_read_stream_t read_stream(
            array.get() + element_offset_offset,
            array.get_safety_boundary() - element_offset_offset);

        uint64_t element_offset;
        switch (offset_size) {
        case datum_offset_size_t::U8BIT: {
            uint8_t off;
            guarantee_deserialization(deserialize_universal(&read_stream, &off),
                                      "datum decode array offset");
            element_offset = off;
        } break;
        case datum_offset_size_t::U16BIT: {
            uint16_t off;
            guarantee_deserialization(deserialize_universal(&read_stream, &off),
                                      "datum decode array offset");
            element_offset = off;
        } break;
        case datum_offset_size_t::U32BIT: {
            uint32_t off;
            guarantee_deserialization(deserialize_universal(&read_stream, &off),
                                      "datum decode array offset");
            element_offset = off;
        } break;
        case datum_offset_size_t::U64BIT: {
            uint64_t off;
            guarantee_deserialization(deserialize_universal(&read_stream, &off),
                                      "datum decode array offset");
            element_offset = off;
        } break;
        }
        guarantee(element_offset <= std::numeric_limits<size_t>::max(),
                  "Datum too large for this architecture.");

        return data_offset + static_cast<size_t>(element_offset);
    }
}

size_t datum_serialized_size(const datum_string_t &s) {
    const size_t s_size = s.size();
    return varint_uint64_serialized_size(s_size) + s_size;
}

serialization_result_t datum_serialize(write_message_t *wm, const datum_string_t &s) {
    const size_t s_size = s.size();
    serialize_varint_uint64(wm, static_cast<uint64_t>(s_size));
    wm->append(s.data(), s_size);
    return serialization_result_t::SUCCESS;
}

MUST_USE archive_result_t datum_deserialize(
        read_stream_t *s,
        datum_string_t *out) {
    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res != archive_result_t::SUCCESS) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    const size_t str_offset = varint_uint64_serialized_size(sz);
    counted_t<shared_buf_t> buf =
        shared_buf_t::create(str_offset + static_cast<size_t>(sz));
    serialize_varint_uint64_into_buf(sz, reinterpret_cast<uint8_t *>(buf->data()));
    int64_t num_read = force_read(s, buf->data() + str_offset, sz);
    if (num_read == -1) {
        return archive_result_t::SOCK_ERROR;
    }
    if (static_cast<uint64_t>(num_read) < sz) {
        return archive_result_t::SOCK_EOF;
    }

    *out = datum_string_t(shared_buf_ref_t<char>(std::move(buf), 0));

    return archive_result_t::SUCCESS;
}


template <cluster_version_t W>
void serialize(write_message_t *wm,
               const empty_ok_t<const datum_t> &datum) {
    const datum_t *pointer = datum.get();
    const bool has = pointer->has();
    serialize<W>(wm, has);
    if (has) {
        serialize<W>(wm, *pointer);
    }
}

INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(empty_ok_t<const datum_t>);

template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s,
                             empty_ok_ref_t<datum_t> datum) {
    bool has;
    archive_result_t res = deserialize<W>(s, &has);
    if (bad(res)) {
        return res;
    }

    datum_t *pointer = datum.get();

    if (!has) {
        pointer->reset();
        return archive_result_t::SUCCESS;
    } else {
        return deserialize<W>(s, pointer);
    }
}

template archive_result_t deserialize<cluster_version_t::v1_13>(
        read_stream_t *s,
        empty_ok_ref_t<datum_t> datum);
template archive_result_t deserialize<cluster_version_t::v1_13_2>(
        read_stream_t *s,
        empty_ok_ref_t<datum_t> datum);
template archive_result_t deserialize<cluster_version_t::v1_14>(
        read_stream_t *s,
        empty_ok_ref_t<datum_t> datum);
template archive_result_t deserialize<cluster_version_t::v1_15>(
        read_stream_t *s,
        empty_ok_ref_t<datum_t> datum);
template archive_result_t deserialize<cluster_version_t::v1_16_is_latest>(
        read_stream_t *s,
        empty_ok_ref_t<datum_t> datum);


}  // namespace ql
