#include "clustering/cluster.hpp"
#include "clustering/serialize.hpp"
#include "arch/arch.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "concurrency/cond_var.hpp"

struct config_t {
    int port;
    const char *contact_host;   // NULL to start a new cluster
    int contact_port;
};

struct cluster_peer_t {
    bool is_us;
    ip_address_t address;
    int port;
    tcp_conn_t *conn;   // NULL if it's us

    cluster_peer_t(config_t *config) {
        is_us = true;
        address = ip_address_t::us();
        port = config->port;
        conn = NULL;
    }

    cluster_peer_t(const ip_address_t &a, int p, tcp_conn_t *c) {
        is_us = false;
        address = a;
        port = p;
        conn = c;
    }

    ~cluster_peer_t() {
        if (!is_us) delete conn;
    }
};

struct cluster_t :
    public tcp_listener_callback_t
{
    /* Use shared_ptr for automatic deallocation */
    std::vector<boost::shared_ptr<cluster_peer_t> > peers;

    tcp_listener_t listener;

    cluster_t(config_t config) :
        listener(config.port)
    {

        ip_address_t our_ip = ip_address_t::us();

        if (config.contact_host) {

            debugf("Contacting host %d\n", config.contact_port);

            /* Get in touch with our specified contact */
            tcp_conn_t contact_conn(config.contact_host, config.contact_port);

            /* Build a list of all the nodes in the cluster. We also open a new connection to each
            one, including a duplicate connection to the node we first contacted. */
            contact_conn.write("LIST", 4);
            int npeers;
            unserialize(&contact_conn, &npeers);
            for (int i = 0; i < npeers; i++) {
                ip_address_t peer_address;
                unserialize(&contact_conn, &peer_address);
                int peer_port;
                unserialize(&contact_conn, &peer_port);
                peers.push_back(boost::make_shared<cluster_peer_t>(
                    peer_address,
                    peer_port,
                    new tcp_conn_t(peer_address, peer_port)
                    ));
            }

            /* Now introduce ourself to the cluster. */
            for (int i = 0; i < npeers; i++) {
                debugf("Greeting peer %d on port %d\n", i, peers[i]->port);
                peers[i]->conn->write("HELO", 4);
                serialize(peers[i]->conn, our_ip);
                serialize(peers[i]->conn, config.port);
            }
            for (int i = 0; i < npeers; i++) {
                char helo[4];
                peers[i]->conn->read(helo, 4);
                assert(!memcmp(helo, "HELO", 4));
            }
        }

        peers.push_back(boost::make_shared<cluster_peer_t>(&config));
        debugf("We are peer %d\n", peers.size() - 1);

        listener.set_callback(this);
    }

    void on_tcp_listener_accept(tcp_conn_t *conn) {

        debugf("Got connection\n");

        char msg[4];
        conn->read(msg, 4);

        if (!memcmp(msg, "LIST", 4)) {
            int npeers = peers.size();
            conn->write(&npeers, 4);

            for (int i = 0; i < (int)peers.size(); i++) {
                serialize(conn, peers[i]->address);
                serialize(conn, peers[i]->port);
            }
            debugf("It just wanted a list.\n");

        } else if (!memcmp(msg, "HELO", 4)) {

            ip_address_t other_addr;
            unserialize(conn, &other_addr);
            int other_port;
            unserialize(conn, &other_port);
            peers.push_back(boost::make_shared<cluster_peer_t>(other_addr, other_port, conn));

            conn->write("HELO", 4);   // Be polite, say HELO back

            debugf("Connection on port %d is %d\n", other_port, peers.size() - 1);
        }
    }

    ~cluster_t() {
    }
};

void cluster_main(config_t config, thread_pool_t *thread_pool) {

    cluster_t cluster(config);

    /* Wait for an order to shut down */
    struct : public thread_message_t, public cond_t {
        void on_thread_switch() { pulse(); }
    } interrupt_cond;
    thread_pool->set_interrupt_message(&interrupt_cond);
    interrupt_cond.wait();

    thread_pool->shutdown();
}

int run_cluster(int argc, char *argv[]) {

    struct : public thread_message_t {
        config_t config;
        thread_pool_t *thread_pool;
        void on_thread_switch() {
            coro_t::spawn(&cluster_main, config, thread_pool);
        }
    } starter;
    
    assert(!strcmp(argv[0], "cluster"));
    starter.config.port = atoi(argv[1]);
    if (argc >= 4) {
        starter.config.contact_host = argv[2];
        starter.config.contact_port = atoi(argv[3]);
    } else {
        starter.config.contact_host = NULL;
    }
    
    thread_pool_t thread_pool(8);
    starter.thread_pool = &thread_pool;
    thread_pool.run(&starter);

    return 0;
}