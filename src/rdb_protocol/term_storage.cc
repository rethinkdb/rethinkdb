// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/term_storage.hpp"

#include "arch/runtime/coroutines.hpp"
#include "rdb_protocol/optargs.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

const size_t MIN_TERM_TREE_STACK_SPACE = 16 * KILOBYTE;

const char *rapidjson_typestr(rapidjson::Type t) {
    switch (t) {
    case rapidjson::kNullType:   return "NULL";
    case rapidjson::kFalseType:  return "BOOL";
    case rapidjson::kTrueType:   return "BOOL";
    case rapidjson::kObjectType: return "OBJECT";
    case rapidjson::kArrayType:  return "ARRAY";
    case rapidjson::kStringType: return "STRING";
    case rapidjson::kNumberType: return "NUMBER";
    default:                     break;
    }
    unreachable();
}

bool query_type_is_valid(Query::QueryType query_type) {
    switch (query_type) {
    case Query::START:
    case Query::CONTINUE:
    case Query::STOP:
    case Query::NOREPLY_WAIT:
    case Query::SERVER_INFO:
        return true;
    default:
        return false;
    }
}

generated_term_t::generated_term_t(Term::TermType _type, backtrace_id_t _bt) :
        type(_type), bt(_bt) { }

raw_term_t::raw_term_t() { }

raw_term_t::raw_term_t(const term_variant_t &source) {
    class param_visitor_t : public boost::static_visitor<void> {
    public:
        explicit param_visitor_t(raw_term_t *_parent) : parent(_parent) { }
        void operator() (const rapidjson::Value *json_source) {
            parent->init_json(json_source);
        }
        void operator() (const counted_t<generated_term_t> &gen_source) {
            parent->info = gen_source;
        }
        raw_term_t *parent;
    } visitor(this);

    boost::apply_visitor(visitor, source);
}

void raw_term_t::init_json(const rapidjson::Value *src) {
    info = json_data_t();
    json_data_t *data = boost::get<json_data_t>(&info);
    data->source = src;

    r_sanity_check(src->IsArray());
    size_t size = src->Size();
    r_sanity_check(size >= 2 && size <= 4);

    const rapidjson::Value *raw_type = &(*src)[0];
    r_sanity_check(raw_type->IsInt());
    Term::TermType type = static_cast<Term::TermType>(raw_type->GetInt64());

    const rapidjson::Value *raw_bt = &(*src)[size - 1];
    r_sanity_check(raw_bt->IsInt());
    backtrace_id_t bt(raw_bt->GetUint());

    data->args = nullptr;
    data->optargs = nullptr;
    data->datum = nullptr;

    if (type == Term::DATUM) {
        rcheck_src(bt, size == 3, base_exc_t::LOGIC,
                   strprintf("Expected 3 items in array, but found %zu.", size));
        data->datum = &(*src)[1];
    } else {
        for (size_t i = 1; i < size - 1; ++i) {
            const rapidjson::Value *item = &(*src)[i];
            if (item->IsArray()) {
                data->args = item;
            } else {
                r_sanity_check(item->IsObject());
                data->optargs = item;
            }
        }
    }
}

size_t raw_term_t::num_args() const {
    size_t res = 0;
    visit_source(
        [&](const json_data_t &source) {
            if (source.args != nullptr) {
                res = source.args->Size();
            }
        },
        [&](const counted_t<generated_term_t> &source) {
            res = source->args.size();
        });
    return res;
}

size_t raw_term_t::num_optargs() const {
    size_t res = 0;
    visit_source(
        [&](const json_data_t &source) {
            if (source.optargs != nullptr) {
                res = source.optargs->MemberCount();
            }
        },
        [&](const counted_t<generated_term_t> &source) {
            res = source->optargs.size();
        });
    return res;
}

raw_term_t raw_term_t::arg(size_t index) const {
    raw_term_t res;
    visit_source(
        [&](const json_data_t &source) {
            guarantee(source.args->Size() > index);
            res.init_json(&(*source.args)[index]);
        },
        [&](const counted_t<generated_term_t> &source) {
            guarantee(source->args.size() > index);
            res = raw_term_t(source->args[index]);
        });
    return res;
}

boost::optional<raw_term_t> raw_term_t::optarg(const std::string &name) const {
    boost::optional<raw_term_t> res;
    visit_source(
        [&](const json_data_t &source) {
            if (source.optargs != nullptr) {
                auto it = source.optargs->FindMember(name.c_str());
                if (it != source.optargs->MemberEnd()) {
                    res = raw_term_t(&it->value);
                }
            }
        },
        [&](const counted_t<generated_term_t> &source) {
            auto it = source->optargs.find(name);
            if (it != source->optargs.end()) {
                res = raw_term_t(it->second);
            }
        });
    return res;
}

datum_t raw_term_t::datum(const configured_limits_t &limits,
                          reql_version_t version) const {
    datum_t res;
    visit_source(
        [&](const json_data_t &source) {
            if (source.datum != nullptr) {
                res = to_datum(*source.datum, limits, version);
            }
        },
        [&](const counted_t<generated_term_t> &source) {
            res = source->datum;
        });
    return res;
}

datum_t raw_term_t::datum() const {
    return datum(configured_limits_t::unlimited, reql_version_t::LATEST);
}

Term::TermType raw_term_t::type() const {
    Term::TermType res = Term::DATUM;
    visit_source(
        [&](const json_data_t& source) {
            res = static_cast<Term::TermType>((*source.source)[0].GetInt64());
        },
        [&](const counted_t<generated_term_t> &source) {
            res = source->type;
        });
    return res;
}

backtrace_id_t raw_term_t::bt() const {
    backtrace_id_t res;
    visit_source(
        [&](const json_data_t& source) {
            res = backtrace_id_t((*source.source)[source.source->Size() - 1].GetUint());
        },
        [&](const counted_t<generated_term_t> &source) {
            res = source->bt;
        });
    return res;
}

term_variant_t raw_term_t::get_src() const {
    term_variant_t res;
    visit_source(
        [&](const json_data_t& source) {
            res = source.source;
        },
        [&](const counted_t<generated_term_t> &source) {
            res = source;
        });
    return res;
}

Query::QueryType term_storage_t::query_type() const {
    r_sanity_check(false, "query_type() is unimplemented for this term_storage_t type");
    unreachable();
}

void term_storage_t::preprocess() {
    r_sanity_check(false, "preprocess() is unimplemented for this term_storage_t type");
    unreachable();
}

bool term_storage_t::static_optarg_as_bool(UNUSED const std::string &key,
                                           UNUSED bool default_value) const {
    r_sanity_check(false, "static_optarg_as_bool() is unimplemented "
                   "for this term_storage_t type");
    unreachable();
}

global_optargs_t term_storage_t::global_optargs() {
    r_sanity_check(false, "global_optargs() is unimplemented "
                   "for this term_storage_t type");
    unreachable();
}

const backtrace_registry_t &term_storage_t::backtrace_registry() const {
    return bt_reg;
}

json_term_storage_t::json_term_storage_t(scoped_array_t<char> &&_original_data,
                                         rapidjson::Document &&_query_json) :
        original_data(std::move(_original_data)),
        query_json(std::move(_query_json)) {
    // We throw `bt_exc_t`s here because we cannot use backtrace IDs until the
    // `preprocess` step has completed.
    if (!query_json.IsArray()) {
        throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
                       strprintf("Expected a query to be an array, but found %s.",
                                 rapidjson_typestr(query_json.GetType())),
                       backtrace_registry_t::EMPTY_BACKTRACE);
    }
    if (query_json.Size() == 0 || query_json.Size() > 3) {
        throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
                       strprintf("Expected 1 to 3 elements in the top-level query, "
                                 "but found %d.", query_json.Size()),
                       backtrace_registry_t::EMPTY_BACKTRACE);
    }

    if (!query_json[0].IsNumber()) {
        throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
                       strprintf("Expected a query type as a number, but found %s.",
                                 rapidjson_typestr(query_json[0].GetType())),
                       backtrace_registry_t::EMPTY_BACKTRACE);
    }
    if (!query_type_is_valid(static_cast<Query::QueryType>(query_json[0].GetInt()))) {
        throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
                       strprintf("Unrecognized QueryType: %d.", query_json[0].GetInt()),
                       backtrace_registry_t::EMPTY_BACKTRACE);
    }

    if (query_json.Size() >= 3) {
        if (!query_json[2].IsObject()) {
            throw bt_exc_t(Response::CLIENT_ERROR, Response::QUERY_LOGIC,
                           strprintf("Expected global optargs as an object, "
                                     "but found %s.",
                                     rapidjson_typestr(query_json[2].GetType())),
                           backtrace_registry_t::EMPTY_BACKTRACE);
        }
    }
}

Query::QueryType json_term_storage_t::query_type() const {
    return static_cast<Query::QueryType>(query_json[0].GetInt());
}

void json_term_storage_t::preprocess() {
    r_sanity_check(query_json.Size() >= 2);
    preprocess_term_tree(&query_json[1], &query_json.GetAllocator(), &bt_reg);
}

raw_term_t json_term_storage_t::root_term() const {
    r_sanity_check(query_json.Size() >= 2);
    return raw_term_t(&query_json[1]);
}

bool json_term_storage_t::static_optarg_as_bool(const std::string &key,
                                                bool default_value) const {
    r_sanity_check(query_json.IsArray());
    if (query_json.Size() < 3) {
        return default_value;
    }

    const rapidjson::Value *global_optargs = &query_json[2];
    r_sanity_check(global_optargs->IsObject());

    const auto it = global_optargs->FindMember(key.c_str());
    if (it == global_optargs->MemberEnd()) {
        return default_value;
    } else if (it->value.IsBool()) {
        return it->value.GetBool();
    } else if (!it->value.IsArray() ||
               it->value.Size() != 2 ||
               !it->value[0].IsNumber() ||
               static_cast<Term::TermType>(it->value[0].GetInt()) != Term::DATUM) {
        return default_value;
    } else if (!it->value[1].IsBool()) {
        return default_value;
    }
    return it->value[1].GetBool();

}

global_optargs_t json_term_storage_t::global_optargs() {
    auto &allocator = query_json.GetAllocator();
    rapidjson::Value *src;

    global_optargs_t res;
    r_sanity_check(query_json.IsArray());
    if (query_json.Size() >= 3) {
        src = &query_json[2];
        r_sanity_check(src->IsObject());
        for (auto it = src->MemberBegin(); it != src->MemberEnd(); ++it) {
            preprocess_global_optarg(&it->value, &allocator);
            res.add_optarg(raw_term_t(&it->value), it->name.GetString());
        }
    } else {
        query_json.PushBack(rapidjson::Value(rapidjson::kObjectType),
                            allocator);
        src = &query_json[query_json.Size() - 1];
    }

    // Create a default db global optarg
    if (!res.has_optarg("db")) {
        src->AddMember(rapidjson::Value("db", allocator),
                       rapidjson::Value(rapidjson::kArrayType),
                       allocator);
        auto it = src->FindMember("db");
        it->value.PushBack(rapidjson::Value(Term::DB), allocator);
        it->value.PushBack(rapidjson::Value(rapidjson::kArrayType), allocator);
        it->value[it->value.Size() - 1].PushBack(rapidjson::Value("test", allocator),
                                                 allocator);
        preprocess_global_optarg(&it->value, &allocator);
        res.add_optarg(raw_term_t(&it->value), it->name.GetString());
    }

    return res;
}

wire_term_storage_t::wire_term_storage_t(scoped_array_t<char> &&_original_data,
                                         rapidjson::Document &&_func_json) :
        original_data(std::move(_original_data)),
        func_json(std::move(_func_json)) {
    r_sanity_check(func_json.IsArray());
    r_sanity_check(func_json.Size() >= 1);
}

raw_term_t wire_term_storage_t::root_term() const {
    return raw_term_t(&func_json);
}

template <typename pb_t>
MUST_USE archive_result_t deserialize_protobuf(read_stream_t *s, pb_t *p) {
    CT_ASSERT(sizeof(int) == sizeof(int32_t));
    int32_t size;
    archive_result_t res = deserialize_universal(s, &size);
    if (bad(res)) { return res; }
    if (size < 0) { return archive_result_t::RANGE_ERROR; }
    scoped_array_t<char> data(size);
    int64_t read_res = force_read(s, data.data(), data.size());
    if (read_res != size) { return archive_result_t::SOCK_ERROR; }
    p->ParseFromArray(data.data(), data.size());
    return archive_result_t::SUCCESS;
}

template
MUST_USE archive_result_t deserialize_protobuf<Term>(read_stream_t *s, Term *bt);
template
MUST_USE archive_result_t deserialize_protobuf<Backtrace>(read_stream_t *s,
                                                          Backtrace *bt);

rapidjson::Value convert_datum(const Datum &src,
                               rapidjson::MAYBE_POOL_ALLOCATOR *allocator) {
    guarantee(src.has_type());
    switch (src.type()) {
    case Datum::R_NULL:
        return rapidjson::Value(rapidjson::kNullType);
    case Datum::R_BOOL:
        guarantee(src.has_r_bool());
        return rapidjson::Value(src.r_bool());
    case Datum::R_NUM:
        guarantee(src.has_r_num());
        return rapidjson::Value(src.r_num());
    case Datum::R_STR:
        guarantee(src.has_r_str());
        return rapidjson::Value(src.r_str(), *allocator);
    case Datum::R_ARRAY: {
        rapidjson::Value dest(rapidjson::kArrayType);
        for (int i = 0; i < src.r_array_size(); ++i) {
            call_with_enough_stack([&]() {
                    dest.PushBack(convert_datum(src.r_array(i), allocator),
                                  *allocator);
                }, MIN_TERM_TREE_STACK_SPACE);
        }
        return dest;
    }
    case Datum::R_OBJECT: {
        rapidjson::Value dest(rapidjson::kObjectType);
        for (int i = 0; i < src.r_object_size(); ++i) {
            call_with_enough_stack([&]() {
                    const Datum_AssocPair &item = src.r_object(i);
                    guarantee(item.has_key());
                    guarantee(item.has_val());
                    dest.AddMember(rapidjson::Value(item.key(), *allocator),
                                   convert_datum(item.val(), allocator), *allocator);
                }, MIN_TERM_TREE_STACK_SPACE);
        }
        return dest;
    }
    case Datum::R_JSON: {
        guarantee(src.has_r_str());
        rapidjson::Document doc(allocator);
        doc.Parse(src.r_str().c_str());
        return rapidjson::Value(std::move(doc));
    }
    default:
        unreachable();
    }
}

rapidjson::Value convert_term_tree(const Term &src,
                                   rapidjson::MAYBE_POOL_ALLOCATOR *allocator);

rapidjson::Value convert_optargs(const Term &src,
                                 rapidjson::MAYBE_POOL_ALLOCATOR *allocator) {
    rapidjson::Value dest(rapidjson::kObjectType);
    for (int i = 0; i < src.optargs_size(); ++i) {
        const Term_AssocPair &optarg = src.optargs(i);
        guarantee(optarg.has_key());
        guarantee(optarg.has_val());
        dest.AddMember(rapidjson::Value(optarg.key(), *allocator),
                       convert_term_tree(optarg.val(), allocator), *allocator);
    }
    return dest;
}

rapidjson::Value convert_term_tree(const Term &src,
                                   rapidjson::MAYBE_POOL_ALLOCATOR *allocator) {
    guarantee(src.has_type());

    rapidjson::Value dest(rapidjson::kArrayType);
    dest.PushBack(rapidjson::Value(static_cast<int>(src.type())), *allocator);

    switch(static_cast<int>(src.type())) {
    case Term::DATUM:
        guarantee(src.has_datum());
        dest.PushBack(convert_datum(src.datum(), allocator), *allocator);
        break;
    case Term::MAKE_OBJ:
        dest.PushBack(convert_optargs(src, allocator), *allocator);
        break;
    default:
        if (src.args_size() > 0) {
            dest.PushBack(rapidjson::Value(rapidjson::kArrayType), *allocator);
            rapidjson::Value *args = &dest[dest.Size() - 1];
            for (int i = 0; i < src.args_size(); ++i) {
                call_with_enough_stack([&]() {
                        args->PushBack(convert_term_tree(src.args(i), allocator),
                                       *allocator);
                    }, MIN_TERM_TREE_STACK_SPACE);
            }
        }
        if (src.optargs_size() > 0) {
            dest.PushBack(convert_optargs(src, allocator), *allocator);
        }
    }
    dest.PushBack(rapidjson::Value(backtrace_id_t::empty().get()), *allocator);
    return dest;
}

template <cluster_version_t W>
MUST_USE archive_result_t deserialize_term_tree(
        read_stream_t *s,
        scoped_ptr_t<term_storage_t> *term_storage_out) {
    Term body;
    archive_result_t res = deserialize_protobuf(s, &body);
    if (bad(res)) { return res; }

    rapidjson::Document doc;
    rapidjson::Value v = convert_term_tree(body, &doc.GetAllocator());
    doc.Swap(v);
    term_storage_out->init(new wire_term_storage_t(scoped_array_t<char>(),
                                                   std::move(doc)));
    return res;
}

template MUST_USE archive_result_t deserialize_term_tree<cluster_version_t::v1_14>(
        read_stream_t *, scoped_ptr_t<term_storage_t> *);
template MUST_USE archive_result_t deserialize_term_tree<cluster_version_t::v1_15>(
        read_stream_t *, scoped_ptr_t<term_storage_t> *);
template MUST_USE archive_result_t deserialize_term_tree<cluster_version_t::v1_16>(
        read_stream_t *, scoped_ptr_t<term_storage_t> *);
template MUST_USE archive_result_t deserialize_term_tree<cluster_version_t::v2_0>(
        read_stream_t *, scoped_ptr_t<term_storage_t> *);
template MUST_USE archive_result_t deserialize_term_tree<cluster_version_t::v2_1>(
        read_stream_t *, scoped_ptr_t<term_storage_t> *);

template <>
MUST_USE archive_result_t deserialize_term_tree<cluster_version_t::v2_2_is_latest>(
        read_stream_t *s,
        scoped_ptr_t<term_storage_t> *term_storage_out) {
    CT_ASSERT(sizeof(int) == sizeof(int32_t));
    int32_t size;
    archive_result_t res = deserialize_universal(s, &size);
    if (bad(res)) { return res; }
    if (size < 0) { return archive_result_t::RANGE_ERROR; }

    scoped_array_t<char> data(size);
    int64_t read_res = force_read(s, data.data(), data.size());
    if (read_res != size) { return archive_result_t::SOCK_ERROR; }

    rapidjson::Document doc;
    doc.ParseInsitu(data.data());

    term_storage_out->init(new wire_term_storage_t(std::move(data), std::move(doc)));
    return archive_result_t::SUCCESS;
}

void write_term(rapidjson::Writer<rapidjson::StringBuffer> *writer,
                const raw_term_t &term) {
    term_variant_t src = term.get_src();
    if (auto json = boost::get<const rapidjson::Value *>(&src)) {
        (*json)->Accept(*writer);
    } else if (boost::get<counted_t<generated_term_t> >(&src)) {
        writer->StartArray();
        writer->Int(term.type());
        datum_t datum = term.datum();

        if (term.type() == Term::DATUM) {
            r_sanity_check(term.num_args() == 0);
            r_sanity_check(term.num_optargs() == 0);
            datum.write_json(writer);
        } else {
            r_sanity_check(!datum.has());
            if (term.num_args() > 0) {
                writer->StartArray();
                for (size_t i = 0; i < term.num_args(); ++i) {
                    call_with_enough_stack([&]() {
                            write_term(writer, term.arg(i));
                        }, MIN_TERM_TREE_STACK_SPACE);
                }
                writer->EndArray();
            }
            if (term.num_optargs() > 0) {
                writer->StartObject();
                term.each_optarg([&](const raw_term_t &optarg, const std::string &name) {
                        writer->Key(name.c_str(), name.size(), true);
                        call_with_enough_stack([&]() {
                                write_term(writer, optarg);
                            }, MIN_TERM_TREE_STACK_SPACE);
                    });
                writer->EndObject();
            }
        }

        writer->Int(term.bt().get());
        writer->EndArray();
    } else {
        unreachable();
    }
}

template <cluster_version_t W>
void serialize_term_tree(write_message_t *wm, const raw_term_t &root_term) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    write_term(&writer, root_term);
    guarantee(writer.IsComplete());

    int32_t size = buffer.GetSize() + 1;
    serialize_universal(wm, size);

    wm->append(buffer.GetString(), size);
}

template void serialize_term_tree<cluster_version_t::LATEST_OVERALL>(
        write_message_t *, const raw_term_t &);

} // namespace ql
