#ifndef __CLUSTER_COUNCIL_DELEGATE_HPP__
#define __CLUSTER_COUNCIL_DELEGATE_HPP__

template<class Council>
class council_delegate_t : public cluster_delegate_t {
private:
    Council council;
public:
    council_delegate_t() {};

    council_delegate_t(typename Council::address_t addr)
        : council(addr)
    { }

    int introduction_ser_size() {
        return ser_size(Council::address_t(&council));
    }
    void introduce_new_node(cluster_outpipe_t *p) {
        ::serialize(p, Council::address_t(&council));
    }

    static council_delegate_t<Council> *construct(cluster_inpipe_t *p) {
        typename Council::address_t addr;
        ::unserialize(p, addr);
        p->done();
        return new council_delegate_t<Council>(addr);
    }
};

#endif
