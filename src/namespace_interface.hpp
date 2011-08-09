#ifndef __NAMESPACE_INTERFACE_HPP__
#define __NAMESPACE_INTERFACE_HPP__

#include "concurrency/fifo_checker.hpp"

/* `namespace_interface_t` is the interface that the protocol-agnostic database
logic for handling queries exposes to the protocol-specific query parser. */
template<class protocol_t>
struct namespace_interface_t {

    virtual typename protocol_t::read_response_t read(typename protocol_t::read_t, order_token_t tok) = 0;
    virtual typename protocol_t::write_response_t write(typename protocol_t::write_t, order_token_t tok) = 0;

protected:
    virtual ~namespace_interface_t() { }
};

#endif /* __NAMESPACE_INTERFACE_HPP__ */
