
#ifndef __BIN_MEMCACHED_HANDLER_HPP__
#define __BIN_MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "btree/key_value_store.hpp"
#include "logger.hpp"
#include <arpa/inet.h>
#include <endian.h>

class server_t;

/* The bin memcached handler is broken right now. We compile it so it doesn't bitrot but it isn't
used in the production server and it doesn't work very well. */

// TODO This shouldn't be necessary.
#define MAX_PACKET_SIZE 200

//Memcached spec section 4.2 and 4.3 state that flags need to be present in get and set responses
//it they don't make much indication of what they need to be but they use 0xdeadbeef so that's what we'll use
static const size_t extra_flags_length = 4;
static const byte extra_flags[extra_flags_length] = {0x00, 0x00, 0x00, 0x00};

class bin_memcached_handler_t :
    public request_handler_t
{

public:
    typedef request_handler_t::parse_result_t parse_result_t;
    using request_handler_t::conn_fsm;
    
public:
    bin_memcached_handler_t(server_t *server)
        : request_handler_t(), key((btree_key*)key_memory), server(server)
        {}
    
    virtual parse_result_t parse_request(event_t *event);

    //the fields in a memcached packet
public:
    typedef uint8_t     bin_magic_t;
    typedef uint8_t     bin_opcode_t;
    typedef uint16_t    bin_key_length_t;
    typedef uint8_t     bin_extra_length_t;
    typedef uint32_t    bin_value_length_t;
    typedef uint8_t     bin_data_type_t;
    typedef uint16_t    bin_status_t;
    typedef uint32_t    bin_total_body_length_t;
    typedef uint32_t    bin_opaque_t;
    typedef uint64_t    bin_cas_t;

    //subfields present in the extras
    typedef uint32_t    bin_flags_t;
    typedef uint32_t    bin_expr_time_t;
    typedef uint64_t    bin_delta_t;    
    typedef uint64_t    bin_init_val_t;
    

    //Magic Byte value
    enum bin_magic_code {
        bin_magic_request = 0x80,
        bin_magic_response = 0x81,
    };

    static bool is_valid_magic(bin_magic_t magic) {
        // This has to be maintained by hand
        return (magic == bin_magic_request || magic == bin_magic_response);
    }

    //response status
    enum bin_status_code {
        bin_status_no_error = 0x0000,
        bin_status_key_not_found = 0x0001,
        bin_status_key_exists = 0x0002,
        bin_status_value_too_large = 0x0003,
        bin_status_invalid_arguments = 0x0004,
        bin_status_item_not_stored = 0x0005,
        bin_status_incr_decr_on_non_numeric_value = 0x0006,
        bin_status_unknown_command = 0x0081,
        bin_status_out_of_memory = 0x0082,
    };

    bool is_valid_status(bin_status_t s) {
        // This has to be maintained by hand
        return (
            s == bin_status_no_error ||
            s == bin_status_key_not_found ||
            s == bin_status_key_exists ||
            s == bin_status_value_too_large ||
            s == bin_status_invalid_arguments ||
            s == bin_status_item_not_stored ||
            s == bin_status_incr_decr_on_non_numeric_value ||
            s == bin_status_unknown_command ||
            s == bin_status_out_of_memory);
    }

    //opcodes
    typedef enum {
        bin_opcode_get = 0x00,
        bin_opcode_set = 0x01,
        bin_opcode_add = 0x02,
        bin_opcode_replace = 0x03, 
        bin_opcode_delete = 0x04, 
        bin_opcode_increment = 0x05, 
        bin_opcode_decrement = 0x06, 
        bin_opcode_quit = 0x07, 
        bin_opcode_flush = 0x08, 
        bin_opcode_getq = 0x09, 
        bin_opcode_no_op = 0x0A, 
        bin_opcode_version = 0x0B, 
        bin_opcode_getk = 0x0C, 
        bin_opcode_getkq = 0x0D, 
        bin_opcode_append = 0x0E, 
        bin_opcode_prepend = 0x0F, 
        bin_opcode_stat = 0x10, 
        bin_opcode_setq = 0x11, 
        bin_opcode_addq = 0x12, 
        bin_opcode_replaceq = 0x13, 
        bin_opcode_deleteq = 0x14, 
        bin_opcode_incrementq = 0x15, 
        bin_opcode_decrementq = 0x16, 
        bin_opcode_quitq = 0x17, 
        bin_opcode_flushq = 0x18, 
        bin_opcode_appendq = 0x19, 
        bin_opcode_prependq = 0x1A, 
        num_opcode_code,
    } bin_opcode_code;

    static bool is_valid_opcode(bin_opcode_t opcode) {
        //of course this depends on the codes being sequential
        return (opcode < num_opcode_code);
    }

    typedef enum {
        bin_data_type_raw_bytes = 0x00,
        num_data_type_code,
    } bin_data_type_code;

    static bool is_valid_data_type(bin_data_type_t data_type) {
        //of course this depends on the codes being sequential
        for (unsigned int i = 0; i < num_data_type_code; i++) {
            if (data_type == i)
                return true;
        }
        return false;
    }

    //check if a packet requires no response
    static bool is_quiet_code(bin_opcode_t opcode) {
        switch (opcode) {
            case bin_opcode_getq:
            case bin_opcode_getkq:
            case bin_opcode_setq:
            case bin_opcode_addq:
            case bin_opcode_replaceq:
            case bin_opcode_deleteq:
            case bin_opcode_incrementq:
            case bin_opcode_decrementq:
            case bin_opcode_quitq:
            case bin_opcode_flushq:
            case bin_opcode_appendq:
            case bin_opcode_prependq:
                return true;
                break;
            default:
                return false;
                break;
        }
    }

    //check if a packet wants the key back
    static bool is_key_code(bin_opcode_t opcode) {
        switch (opcode) {
            case bin_opcode_getk:
            case bin_opcode_getkq:
                return true;
                break;
            default:
                return false;
                break;
        }
    }

    struct request_header_t {
        bin_magic_t             magic;
        bin_opcode_t            opcode;
        bin_key_length_t        key_length;
        bin_extra_length_t      extra_length;
        bin_data_type_t         data_type;
        uint16_t                reserved;
        bin_total_body_length_t total_body_length;
        bin_opaque_t            opaque;
        bin_cas_t               cas;               
    } __attribute__((__packed__));

    struct response_header_t {
        bin_magic_t             magic;
        bin_opcode_t            opcode;
        bin_key_length_t        key_length;
        bin_extra_length_t      extra_length;
        bin_data_type_t         data_type;
        bin_status_t            status;
        bin_total_body_length_t total_body_length;
        bin_opaque_t            opaque;
        bin_cas_t               cas;
    } __attribute__((__packed__));

    struct get_response_extras_t {
        bin_flags_t             flags;
    } __attribute__((__packed__));

    struct set_request_extras_t {
        bin_flags_t             flags;
        bin_expr_time_t         expr_time;
    } __attribute__((__packed__));

    struct incr_decr_request_extras_t {
        bin_delta_t             delta;
        bin_init_val_t          init_val;
        bin_expr_time_t         expr_time;
    } __attribute__((__packed__));

    struct flush_request_extras_t {
        bin_expr_time_t         expr_time;
    };

    struct packet_t
        {
        public:
            packet_t() :data(NULL) {}
            packet_t(byte *data) :data(data) {}
            ~packet_t() {}
            byte *data;

            bool is_request() {
                if (magic() == bin_magic_request)
                    return true;
                else if (magic() == bin_magic_response)
                    return false;
                else {
                    crash("Packet has corrupted magic number"); // FIXME we should never crash on input data
                }
            }

            //sick of looking up which one i defined
            bool is_response() {return !is_request();}

            //TODO these require the response and request headers to have indentical structures we should fix this up
            //accessors
            bin_magic_t magic() {
                return ((request_header_t *)(data))->magic;
            }

            bin_opcode_t opcode() {
                return ((request_header_t *)(data))->opcode;
            }

            bin_key_length_t key_length() {
                return ntoh(((request_header_t *)(data))->key_length);
            }

            bin_extra_length_t extra_length() {
                return ((request_header_t *)(data))->extra_length;
            }

            bin_data_type_t data_type() {
                return ((request_header_t *)(data))->data_type;
            }

            bin_total_body_length_t total_body_length() {
                return ntoh(((request_header_t *)(data))->total_body_length);
            }

            bin_value_length_t value_length() {
                return total_body_length() - (key_length() + extra_length()); 
            }

            uint16_t reserved() {
                assert(is_request());
                return ((request_header_t *)(data))->reserved;
            }

            bin_status_t status() {
                assert(is_response());
                return ntoh(((response_header_t *)(data))->status);
            }

            bin_opaque_t opaque() {
                return ((request_header_t *)(data))->opaque;
            }

            bin_cas_t cas() {
                return ((request_header_t *)(data))->cas;
            }
            
            byte* extras() {
                if (is_request())
                    return data + sizeof(request_header_t);
                else
                    return data + sizeof(response_header_t);
            }

            void set_extras(const byte *data,const bin_extra_length_t length) {
                //make space
                memmove(value() + (length - extra_length()), value(), value_length());
                memmove(key() + (length - extra_length()), key(), key_length());

                //copy in the new data
                memcpy(extras(), data, length);

                //update extra length
                extra_length(length);
            }    
            
            //accessors for values in the extras
            bin_flags_t flags() {
                switch (opcode()) {
                    case bin_opcode_get:
                    case bin_opcode_getq:
                    case bin_opcode_getk:
                    case bin_opcode_getkq:
                        if (is_response() && extra_length() == sizeof(get_response_extras_t)) {
                            get_response_extras_t *e = (get_response_extras_t *) extras();
                            return e->flags;
                        } else {
                            goto error_breakout;
                        }
                        break;
                    case bin_opcode_set:
                    case bin_opcode_setq:
                        if (is_request() && extra_length() == sizeof(set_request_extras_t)) {
                            set_request_extras_t *e = (set_request_extras_t *) extras();
                            return e->flags;
                        } else {
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
error_breakout:
                crash_or_trap("Trying to get flags from something that doesn't have flags"); // FIXME handle errors nicely
            }

            bin_expr_time_t expr_time() {
                switch (opcode()) {
                    case bin_opcode_set:
                    case bin_opcode_setq:
                        if (is_request() && extra_length() == sizeof(set_request_extras_t)) {
                            set_request_extras_t *e = (set_request_extras_t *) extras();
                            return ntoh(e->expr_time);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    case bin_opcode_increment:
                    case bin_opcode_decrement:
                    case bin_opcode_incrementq:
                    case bin_opcode_decrementq:
                        if (is_request() && extra_length() == sizeof(incr_decr_request_extras_t)) {
                            incr_decr_request_extras_t *e = (incr_decr_request_extras_t *) extras();
                            return ntoh(e->expr_time);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    case bin_opcode_flush:
                    case bin_opcode_flushq:
                        if (is_request() && extra_length() == sizeof(flush_request_extras_t)) {
                            flush_request_extras_t *e = (flush_request_extras_t *) extras();
                            return ntoh(e->expr_time);
                        } else {
                            //note this means that you need to to check if the flush actually has flags first
                            //on the flush flags are optional
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
error_breakout:
                crash_or_trap("Trying to get expr from something that doesn't have expr");   // FIXME handle errors nicely
            }
            
            bin_delta_t delta() {
                switch (opcode()) {
                    case bin_opcode_increment:
                    case bin_opcode_decrement:
                    case bin_opcode_incrementq:
                    case bin_opcode_decrementq:
                        if (is_request() && extra_length() == sizeof(incr_decr_request_extras_t)) {
                            incr_decr_request_extras_t *e = (incr_decr_request_extras_t *) extras();
                            return ntoh(e->delta);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
error_breakout:
                crash_or_trap("Trying to get delta from something that doesn't have delta"); // FIXME handle errors nicely
            }

            bin_init_val_t init_val() {
                switch (opcode()) {
                    case bin_opcode_increment:
                    case bin_opcode_decrement:
                    case bin_opcode_incrementq:
                    case bin_opcode_decrementq:
                        if (is_request() && extra_length() == sizeof(incr_decr_request_extras_t)) {
                            incr_decr_request_extras_t *e = (incr_decr_request_extras_t *) extras();
                            return ntoh(e->init_val);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
error_breakout:
                crash_or_trap("Trying to get init_val from something that doesn't have init_val");   // FIXME handle errors nicely
            }

            //setters
            void magic(bin_magic_t value) {
                ((request_header_t *)(data))->magic = value;
            }

            void opcode(bin_opcode_t value) {
                ((request_header_t *)(data))->opcode = value;
            }

            void total_body_length(bin_total_body_length_t value) {
                ((request_header_t *)(data))->total_body_length = htonl(value);
            }

            void key_length(bin_key_length_t value) {
                total_body_length((total_body_length() - key_length()) + value);
                ((request_header_t *)(data))->key_length = htons(value);
            }

            void extra_length(bin_extra_length_t value) {
                total_body_length((total_body_length() - extra_length()) + value);
                ((request_header_t *)(data))->extra_length = value;
            }

            void value_length(bin_value_length_t value) {
                total_body_length((total_body_length() - value_length()) + value);
            }

            void data_type(bin_data_type_t value) {
                ((request_header_t *)(data))->data_type = value;
            }

            void reserved(uint16_t value) {
                assert(is_request());
                ((request_header_t *)(data))->reserved = value;
            }

            void status(bin_status_t value) {
                assert(is_response());
                ((response_header_t *)(data))->status = htons(value);
            }

            void opaque(bin_opaque_t value){
                ((request_header_t *)(data))->opaque = value;
            }

            void cas(bin_cas_t value){
                ((request_header_t *)(data))->cas = value;
            }


            size_t size() {
                if (is_request())
                    return sizeof(request_header_t) + total_body_length();
                else 
                    return sizeof(response_header_t) + total_body_length();
            }
            
            //load the key into an allocated key
            void key(btree_key *key) {
                key->size = key_length();
                if (is_request()) {
                    memcpy(key->contents, data + sizeof(request_header_t) + extra_length(), key_length());
                } else {
                    memcpy(key->contents, data + sizeof(response_header_t) + extra_length(), key_length());
                }
            }

            byte *key() {
                if (is_request())
                    return data + sizeof(request_header_t) + extra_length();
                else
                    return data + sizeof(response_header_t) + extra_length();
            }

            void set_key(const byte *data, unsigned long size) {
                //make space
                memmove(value() + (size - key_length()), value(), value_length());
                
                //copy in the new data
                memcpy(this->key(), data, size);

                //update the total and key length
                key_length(size);
            }

            void set_key(btree_key *key) {
                set_key(key->contents, key->size);
            }

            //load the value into an allocated value

            byte *value() {
                if (is_request())
                    return data + sizeof(request_header_t) + extra_length() + key_length();
                else
                    return data + sizeof(response_header_t) + extra_length() + key_length();
            }

            void set_value(const byte* data, unsigned long size) {
                memcpy(value(), data, size);
                value_length((bin_value_length_t) size);
            }

            //setters for values in the extras
            void flags(bin_flags_t flags) {
                switch (opcode()) {
                    case bin_opcode_get:
                    case bin_opcode_getq:
                    case bin_opcode_getk:
                    case bin_opcode_getkq:
                        if (is_response() && extra_length() == sizeof(get_response_extras_t)) {
                            get_response_extras_t *e = (get_response_extras_t *) extras();
                            e->flags = flags;
                        } else {
                            goto error_breakout;
                        }
                        break;
                    case bin_opcode_set:
                    case bin_opcode_setq:
                        if (is_request() && extra_length() == sizeof(set_request_extras_t)) {
                            set_request_extras_t *e = (set_request_extras_t *) extras();
                            e->flags = flags;
                        } else {
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
                return;
error_breakout:
                crash_or_trap("Trying to set flags in a packet that doesn't have flags");    // FIXME handle errors nicely
            }

            void expr_time(bin_expr_time_t expr_time) {
                switch (opcode()) {
                    case bin_opcode_set:
                    case bin_opcode_setq:
                        if (is_request() && extra_length() == sizeof(set_request_extras_t)) {
                            set_request_extras_t *e = (set_request_extras_t *) extras();

                            e->expr_time = hton(expr_time);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    case bin_opcode_increment:
                    case bin_opcode_decrement:
                    case bin_opcode_incrementq:
                    case bin_opcode_decrementq:
                        if (is_request() && extra_length() == sizeof(incr_decr_request_extras_t)) {
                            incr_decr_request_extras_t *e = (incr_decr_request_extras_t *) extras();
                            e->expr_time = hton(expr_time);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    case bin_opcode_flush:
                    case bin_opcode_flushq:
                        if (is_request() && extra_length() == sizeof(flush_request_extras_t)) {
                            flush_request_extras_t *e = (flush_request_extras_t *) extras();
                            e->expr_time = hton(expr_time);
                        } else {
                            //note this means that you need to to check if the flush actually has flags first
                            //on the flush flags are optional
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
                return;
error_breakout:
                crash_or_trap("Trying to set expr_time in something that doesn't have expr");    // FIXME handle errors nicely
            }
            
            void delta(bin_delta_t delta) {
                switch (opcode()) {
                    case bin_opcode_increment:
                    case bin_opcode_decrement:
                    case bin_opcode_incrementq:
                    case bin_opcode_decrementq:
                        if (is_request() && extra_length() == sizeof(incr_decr_request_extras_t)) {
                            incr_decr_request_extras_t *e = (incr_decr_request_extras_t *) extras();
                            e->delta = hton(delta);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
                return;
error_breakout:
                crash_or_trap("Trying to set delta from something that doesn't have delta"); // FIXME handle errors nicely
            }

            void init_val(bin_init_val_t init_val) {
                switch (opcode()) {
                    case bin_opcode_increment:
                    case bin_opcode_decrement:
                    case bin_opcode_incrementq:
                    case bin_opcode_decrementq:
                        if (is_request() && extra_length() == sizeof(incr_decr_request_extras_t)) {
                            incr_decr_request_extras_t *e = (incr_decr_request_extras_t *) extras();
                            e->init_val = hton(e->init_val);
                        } else {
                            goto error_breakout;
                        }
                        break;
                    default:
                        goto error_breakout;
                        break;
                }
error_breakout:
                crash_or_trap("Trying to get expr from something that doesn't have expr");   // FIXME handle errors nicely
            }

            bool is_valid_request() {
                bool valid = true;
                valid = valid && (magic() == (bin_magic_t) bin_magic_request);
                valid = valid && bin_memcached_handler_t::is_valid_opcode(opcode());
                valid = valid && bin_memcached_handler_t::is_valid_data_type(data_type());

                //do opcode specific tests
                switch (opcode()) {
                    case bin_opcode_get:
                    case bin_opcode_getq:
                    case bin_opcode_getk:
                    case bin_opcode_getkq:
                        //must have key
                        //must not have value
                        //may have extras
                        valid = valid && (key_length() > 0);
                        valid = valid && (value_length() == 0);
                        break;
                    case bin_opcode_set:
                    case bin_opcode_setq:
                    case bin_opcode_add:
                    case bin_opcode_addq:
                    case bin_opcode_replace:
                    case bin_opcode_replaceq:
                        //must have a key
                        //must have a value
                        //must have extras
                        valid = valid && (key_length() > 0);
                        valid = valid && (value_length() > 0);
                        valid = valid && (extra_length() > 0);
                        break;
                    case bin_opcode_delete:
                    case bin_opcode_deleteq:
                        //must have a key
                        //must not have a value
                        //must not have extras
                        valid = valid && (key_length() > 0);
                        valid = valid && (value_length() == 0);
                        valid = valid && (extra_length() == 0);
                        break;
                    case bin_opcode_increment:
                    case bin_opcode_decrement:
                    case bin_opcode_incrementq:
                    case bin_opcode_decrementq:
                        //must have a key
                        //must not have a value
                        //must have extras
                        valid = valid && (key_length() > 0);
                        valid = valid && (value_length() == 0);
                        valid = valid && (extra_length() > 0);
                        break;
                    case bin_opcode_append:
                    case bin_opcode_appendq:
                    case bin_opcode_prepend:
                    case bin_opcode_prependq:
                        //must have a key
                        //must have a value
                        //must not have extras
                        valid = valid && (key_length() > 0);
                        valid = valid && (value_length() > 0);
                        valid = valid && (extra_length() == 0);
                        break;
                    case bin_opcode_quit:
                    case bin_opcode_quitq:
                        //must not have a key
                        //must not have a value
                        //must not have extras
                        valid = valid && (key_length() == 0);
                        valid = valid && (value_length() == 0);
                        valid = valid && (extra_length() == 0);
                        break;
                    case bin_opcode_flush:
                    case bin_opcode_flushq:
                        //must not have a key
                        //must not have a value
                        //may have extras
                        valid = valid && (key_length() == 0);
                        valid = valid && (value_length() == 0);
                        break;
                    case bin_opcode_no_op:
                        //must not have a key
                        //must not have a value
                        //must not have extras
                        valid = valid && (key_length() == 0);
                        valid = valid && (value_length() == 0);
                        valid = valid && (extra_length() == 0);
                        break;
                    case bin_opcode_version:
                        //must not have a key
                        //must not have a value
                        //must not have extras
                        valid = valid && (key_length() == 0);
                        valid = valid && (value_length() == 0);
                        valid = valid && (extra_length() == 0);
                        break;
                    case bin_opcode_stat:
                        //may have a key
                        //must not have a value
                        //must not have extras
                        valid = valid && (value_length() == 0);
                        valid = valid && (extra_length() == 0);
                        break;
                    default:
                        valid = valid && false;
                        break;
                }
                return valid;
            }

            void print() {
#ifndef NDEBUG
                mlog_start(DBG);
                mlogf("\n");
                //header
                mlogf("+---------------+---------------+---------------+---------------+ <--- HEADER\n");
                mlogf("|Magic: 0x%x\t|Opcode: 0x%x\t|Key Length: %u\t\t\t|\n", magic(), opcode(), key_length());
                mlogf("+---------------+---------------+---------------+---------------+\n");
                if (is_request())
                    mlogf("|Ex. L: %u\t|Data type: 0x%x\t|Reserved: %u\t\t\t|\n", extra_length(), data_type(), reserved());
                else
                    mlogf("|Ex. L: %u\t|Data type: 0x%x\t|Status: 0x%x\t\t\t|\n", extra_length(), data_type(), status());
                mlogf("+---------------+---------------+---------------+---------------+\n");
                mlogf("|Total body length: %u\t\t\t\t\t\t|\n", total_body_length());
                mlogf("+---------------+---------------+---------------+---------------+\n");
                mlogf("|Opaque: %u\t\t\t\t\t\t\t|\n", opaque());
                mlogf("+---------------+---------------+---------------+---------------+\n");
                mlogf("|CAS: %lu\t\t\t\t\t\t\t\t|\n", cas());
                mlogf("|                                                               |\n");

                //extras
                mlogf("+---------------+---------------+---------------+---------------+ <-- EXTRAS\n");
                for (unsigned int i = 0; i < extra_length(); i++) {
                    if (i % 4 == 0 && i != 0)
                        mlogf("|\n+---------------+---------------+---------------+---------------+\n");
                    mlogf("|0x%x\t\t", (uint8_t) extras()[i]);
                }

                //key
                mlogf("|\n+---------------+---------------+---------------+---------------+ <-- KEY\n");
                for (unsigned int i = 0; i < key_length(); i++) {
                    if (i % 4 == 0 && i != 0)
                        mlogf("|\n+---------------+---------------+---------------+---------------+\n");
                    mlogf("|%c\t\t", key()[i]);
                }

                //value
                mlogf("|\n+---------------+---------------+---------------+---------------+ <-- VALUE\n");
                for (unsigned int i = 0; i < value_length(); i++) {
                    if (i % 4 == 0 && i != 0)
                        mlogf("|\n+---------------+---------------+---------------+---------------+\n");
                    mlogf("|%c\t\t", value()[i]);
                }
                mlogf("|\n+---------------+---------------+---------------+---------------+\n");
                mlog_end();
#endif
            }
    };


private:
    //storage_command cmd;
    char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
    btree_key * const key;
    
    parse_result_t read_data();
    
    parse_result_t malformed_request();
    parse_result_t unimplemented_request();
    
    parse_result_t dispatch_appropriate_fsm(packet_t *pkt);
    parse_result_t no_op(packet_t *pkt);
    parse_result_t stat(packet_t *pkt);
    parse_result_t version(packet_t *pkt);

public:
    server_t *server;
};

#endif // __BIN_MEMCACHED_HANDLER_HPP__
