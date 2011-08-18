#ifndef __CLUSTERING_REGISTRAR_HPP__
#define __CLUSTERING_REGISTRAR_HPP__

template<class protocol_t>
struct registrar_t {

    registrar_t(mailbox_cluster_t *cluster);

    registrar_metadata_t<protocol_t> get_business_card() {
    }

    

private:
    
};

#endif /* __CLUSTERING_REGISTRAR_HPP__ */
