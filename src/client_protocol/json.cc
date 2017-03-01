// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "client_protocol/json.hpp"

#include "arch/io/network.hpp"
#include "arch/timing.hpp"
#include "client_protocol/protocols.hpp"
#include "concurrency/pmap.hpp"
#include "containers/scoped.hpp"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rdb_protocol/rdb_backtrace.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_params.hpp"
#include "rdb_protocol/response.hpp"
#include "rdb_protocol/term_storage.hpp"
#include "utils.hpp"

scoped_ptr_t<ql::query_params_t> json_protocol_t::parse_query_from_buffer(
        scoped_array_t<char> &&buffer, size_t offset,
        ql::query_cache_t *query_cache, int64_t token,
        ql::response_t *error_out) {
    rapidjson::Document doc;
    doc.ParseInsitu(buffer.data() + offset);

    scoped_ptr_t<ql::query_params_t> res;
    if (!doc.HasParseError()) {
        try {
            res = make_scoped<ql::query_params_t>(token, query_cache,
                    scoped_ptr_t<ql::term_storage_t>(
                        new ql::json_term_storage_t(std::move(buffer), std::move(doc))));
        } catch (const ql::bt_exc_t &ex) {
            error_out->fill_error(Response::CLIENT_ERROR,
                                  ex.error_type,
                                  strprintf("Server could not parse query: %s",
                                            ex.message.c_str()),
                                  ex.bt_datum);
        }
    } else {
        error_out->fill_error(Response::CLIENT_ERROR,
                              Response::RESOURCE_LIMIT,
                              wire_protocol_t::unparseable_query_message,
                              ql::backtrace_registry_t::EMPTY_BACKTRACE);
    }
    return res;
}

scoped_ptr_t<ql::query_params_t> json_protocol_t::parse_query(
        tcp_conn_t *conn,
        signal_t *interruptor,
        ql::query_cache_t *query_cache) {
    int64_t token;
    uint32_t size;
    conn->read_buffered(&token, sizeof(token), interruptor);
    conn->read_buffered(&size, sizeof(size), interruptor);
    ql::response_t error;

#ifdef __s390x__
    token = __builtin_bswap64(token);
    size = __builtin_bswap32(size);
#endif

    if (size >= wire_protocol_t::TOO_LARGE_QUERY_SIZE) {
        error.fill_error(Response::CLIENT_ERROR,
                         Response::RESOURCE_LIMIT,
                         wire_protocol_t::too_large_query_message(size),
                         ql::backtrace_registry_t::EMPTY_BACKTRACE);

        if (size < wire_protocol_t::HARD_LIMIT_TOO_LARGE_QUERY_SIZE) {
            // Ignore all the extra data that the client is trying to send.
            // within reason. This is so it doesn't look like a broken pipe error.

            signal_timer_t read_timeout_interruptor{wire_protocol_t::TOO_LONG_QUERY_TIME};
            wait_any_t pop_interruptor(interruptor, &read_timeout_interruptor);
            conn->pop(size, &pop_interruptor);
        }

        send_response(&error, token, conn, interruptor);
        throw tcp_conn_read_closed_exc_t();
    }

    scoped_array_t<char> data(size + 1);
    // It's *usually* more efficient to do an un-buffered read here. The client is
    // usually not going to group multiple queries into the same network package
    // (especially not with tcp_nodelay set), and using the non-buffered `read` can
    // avoid an extra copy.
    conn->read(data.data(), size, interruptor);
    data[size] = 0; // Null terminate the string, which the json parser requires

    scoped_ptr_t<ql::query_params_t> res =
        parse_query_from_buffer(std::move(data), 0, query_cache, token, &error);

    if (!res.has()) {
        send_response(&error, token, conn, interruptor);
    }
    return res;
}

void write_response_internal(ql::response_t *response,
                             rapidjson::StringBuffer *buffer_out,
                             bool throw_errors) {
    rapidjson::Writer<rapidjson::StringBuffer> writer(*buffer_out);
    size_t start_offset = buffer_out->GetSize();

    try {
        writer.StartObject();
        writer.Key("t", 1);
        writer.Int(response->type());
        if (response->type() == Response::RUNTIME_ERROR &&
            response->error_type()) {
            writer.Key("e", 1);
            writer.Int(*response->error_type());
        }

        writer.Key("r", 1);
        writer.StartArray();
        const size_t PARALLELIZATION_THRESHOLD = 500;
        if (response->data().size() > PARALLELIZATION_THRESHOLD) {
            int64_t num_threads = std::min<int64_t>(16, get_num_db_threads());
            int32_t thread_offset = get_thread_id().threadnum;
            std::vector<rapidjson::StringBuffer> buffers(num_threads);

            size_t per_thread = response->data().size() / num_threads;
            pmap(num_threads, [&](int64_t m) {
                    int32_t target_thread =
                        (thread_offset + static_cast<int32_t>(m)) % get_num_db_threads();
                    on_thread_t rethreader((threadnum_t(target_thread)));
                    rapidjson::StringBuffer *thread_buffer = &buffers[m];
                    rapidjson::Writer<rapidjson::StringBuffer>
                        thread_writer(*thread_buffer);

                    thread_writer.StartArray();
                    size_t offset = per_thread * m;
                    size_t end = (m == num_threads - 1) ?
                        response->data().size() : (per_thread * (m + 1));

                    for (size_t i = offset; i < end; ++i) {
                        const size_t YIELD_INTERVAL = 2000;
                        if ((i + 1) % YIELD_INTERVAL == 0) {
                            coro_t::yield();
                        }
                        response->data()[i].write_json(&thread_writer);
                    }

                    thread_writer.EndArray();
                });

            for (const auto &buffer : buffers) {
                writer.SpliceArray(buffer);
            }
        } else {
            for (const auto &item : response->data()) {
                item.write_json(&writer);
            }
        }
        writer.EndArray();
        if (response->backtrace()) {
            writer.Key("b", 1);
            response->backtrace()->write_json(&writer);
        }
        if (response->profile()) {
            writer.Key("p", 1);
            response->profile()->write_json(&writer);
        }
        if (response->type() == Response::SUCCESS_PARTIAL ||
            response->type() == Response::SUCCESS_SEQUENCE) {
            writer.Key("n", 1);
            writer.StartArray();
            for (const auto &note : response->notes()) {
                writer.Int(note);
            }
            writer.EndArray();
        }
        writer.EndObject();
        guarantee(writer.IsComplete());
    } catch (const ql::base_exc_t &ex) {
        buffer_out->Pop(buffer_out->GetSize() - start_offset);
        response->fill_error(Response::RUNTIME_ERROR, Response::QUERY_LOGIC,
                             ex.what(), ql::backtrace_registry_t::EMPTY_BACKTRACE);
        write_response_internal(response, buffer_out, true);
    } catch (const std::exception &ex) {
        if (throw_errors) {
            throw;
        }

        buffer_out->Pop(buffer_out->GetSize() - start_offset);
        response->fill_error(Response::RUNTIME_ERROR, Response::INTERNAL,
            strprintf("Internal error in json_protocol_t::write: %s", ex.what()),
            ql::backtrace_registry_t::EMPTY_BACKTRACE);
        write_response_internal(response, buffer_out, true);
    }
}

// Small wrapper - in debug mode we would rather crash than send the error back
void json_protocol_t::write_response_to_buffer(ql::response_t *response,
                                               rapidjson::StringBuffer *buffer_out) {
#ifdef NDEBUG
    write_response_internal(response, buffer_out, false);
#else
    write_response_internal(response, buffer_out, true);
#endif
}

void json_protocol_t::send_response(ql::response_t *response,
                                    int64_t token,
                                    tcp_conn_t *conn,
                                    signal_t *interruptor) {
    uint32_t data_size; // filled in below
    const size_t prefix_size = sizeof(token) + sizeof(data_size);

    // Reserve space for the token and the size
    rapidjson::StringBuffer buffer;
    buffer.Push(prefix_size);

    write_response_to_buffer(response, &buffer);
    int64_t payload_size = buffer.GetSize() - prefix_size;
    guarantee(payload_size > 0);

    static_assert(std::is_same<decltype(wire_protocol_t::TOO_LARGE_RESPONSE_SIZE),
                               const uint32_t>::value,
                  "The largest response must fit in 32 bits.");

    if (payload_size >= wire_protocol_t::TOO_LARGE_RESPONSE_SIZE) {
        response->fill_error(Response::RUNTIME_ERROR,
                             Response::RESOURCE_LIMIT,
                             wire_protocol_t::too_large_response_message(payload_size),
                             ql::backtrace_registry_t::EMPTY_BACKTRACE);
        send_response(response, token, conn, interruptor);
        return;
    }

    // Fill in the token and size
    char *mutable_buffer = buffer.GetMutableBuffer();
#ifdef __s390x__
    token = __builtin_bswap64(token);
#endif
    for (size_t i = 0; i < sizeof(token); ++i) {
        mutable_buffer[i] = reinterpret_cast<const char *>(&token)[i];
    }

    data_size = static_cast<uint32_t>(payload_size);
#ifdef __s390x__
    data_size = __builtin_bswap32(data_size);
#endif
    for (size_t i = 0; i < sizeof(data_size); ++i) {
        mutable_buffer[i + sizeof(token)] =
            reinterpret_cast<const char *>(&data_size)[i];
    }

    conn->write(buffer.GetString(), buffer.GetSize(), interruptor);
}

