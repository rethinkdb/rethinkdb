#include "load_balancer.hpp"
#include "logger.hpp"


elb_t::elb_t(role_t role, int port)
    : role(role), port(port), conn_acceptor()
{
    switch (role) {
    case master:
        conn_acceptor.reset(new conn_acceptor_t(port, &handler)); break;
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
                conn_acceptor.reset(new conn_acceptor_t(port, &handler));
            } catch (conn_acceptor_t::address_in_use_exc_t) {
                logERR("Attempted to listen on port: %d, for elb connections and failed. Something else is listening this will produce very unpredictable load balancing and should be fixed\n");
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
