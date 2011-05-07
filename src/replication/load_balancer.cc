#include "load_balancer.hpp"
#include "logger.hpp"

void ignore(UNUSED tcp_conn_t *) {
    /* This function gets called when the ELB port is connected to. It returns
    immediately, which means the connection is disconnected immediately. */
}

elb_t::elb_t(role_t role_, int port_)
    : role(role_), port(port_), conn_acceptor()
{
    switch (role) {
    case master:
        conn_acceptor.reset(new conn_acceptor_t(port, &ignore)); break;
    case slave:
        break;
    default:
        unreachable();
        break;
    }
}

elb_t::~elb_t() {
    conn_acceptor.reset();
}

void elb_t::on_failure() {
    switch (role) {
        case master:
            unreachable();
            break;
        case slave:
            try {
                conn_acceptor.reset(new conn_acceptor_t(port, &ignore));
            } catch (conn_acceptor_t::address_in_use_exc_t) {
                logERR("Attempted to listen on port: %d, for elb connections and failed. Something else is listening this will produce very unpredictable load balancing and should be fixed\n", port);
            }
            break;
        default:
            unreachable();
            break;
    }
}

void elb_t::on_resume() {
    switch (role) {
        case master:
            unreachable();
            break;
        case slave:
            conn_acceptor.reset();
            break;
        default:
            unreachable();
            break;
    }
}
