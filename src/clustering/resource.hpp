#ifndef __CLUSTERING_RESOURCE_HPP__
#define __CLUSTERING_RESOURCE_HPP__

/* It's a common paradigm to have some resource that you find out about through
metadata, and need to monitor in case it goes offline. These types facilitate
that. */

template<class business_card_t>
class resource_metadata_t {
public:
    bool is_alive;
    peer_id_t peer;
    business_card_t contact_info;

    resource_metadata_t() : is_alive(false) { }
    resource_metadata_t(mailbox_cluster_t *cluster, const business_card_t &ci) :
        is_alive(true), peer(cluster->get_me()), contact_info(ci) { }
};

class resource_lost_exc_t : public std::exception {
public:
    resource_lost_exc_t() { }
    const char *what() const throw () {
        return "access to resource interrupted";
    }
};

template<class business_card_t>
class resource_access_t {

public:
    /* Starts monitoring the resource. Throws `resource_lost_exc_t` if the
    resource is inaccessible. */
    resource_access_t(
        mailbox_cluster_t *cluster,
        metadata_read_view_t<resource_metadata_t<business_card_t> > *view)
    {
        ...
    }

    /* Returns a `signal_t *` that will be pulsed when access to the resource is
    lost. */
    signal_t *get_signal();

    /* Returns the resource contact info if the resource is accessible. Throws
    `resource_lost_exc_t` if it's not. */
    business_card_t access();

private:
    ...
};

#endif /* __CLUSTERING_RESOURCE_HPP__ */
