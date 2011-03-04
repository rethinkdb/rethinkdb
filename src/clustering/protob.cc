#include "clustering/protob.hpp"

void hd_packet(header::hdr hdr) {
    char buffer[1024];
    guarantee(hdr.SerializeToArray(buffer, 1024));
    print_hd(buffer, 0, hdr.ByteSize());
}


bool peek_protob(tcp_conn_t *conn, Message *msg) {
    header::hdr hdr, _hdr;
    hdr.Clear(); //not sure if this is needed
    _hdr.Clear();
    _hdr.set_size(0);

    const_charslice slc;
    for (;;) {
        slc = conn->peek(_hdr.ByteSize());

        guarantee(hdr.ParsePartialFromArray(slc.beg, _hdr.ByteSize()), "None header.hdr form data found on the socket when we were expecting a header\n");
        if (hdr.has_size()) 
            break;
        conn->read_more_buffered();
    }

    slc = conn->peek(hdr.size());
    guarantee(hdr.ParseFromArray(slc.beg, hdr.size()), "None header.hdr form data found on the socket when we were expecting a header\n");


    if (msg->GetTypeName().compare(hdr.type()) == 0) {
        guarantee(msg->ParseFromString(hdr.msg()));
        return true;
    } else {
        return false;
    }
}

void pop_protob(tcp_conn_t *conn, header::hdr *out_hdr) {
    header::hdr hdr, _hdr;
    hdr.Clear(); //not sure if this is needed
    _hdr.Clear();
    _hdr.set_size(0);

    const_charslice slc;
    for (;;) {
        const_charslice slc = conn->peek(_hdr.ByteSize());
        hdr.ParsePartialFromArray(slc.beg, _hdr.ByteSize());
        if (hdr.has_size()) break;
        conn->read_more_buffered();
    }

    slc = conn->peek(hdr.size());
    guarantee(hdr.ParseFromArray(slc.beg, hdr.size()), "None header.hdr form data found on the socket when we were expecting a header\n");

    /* fprintf(stderr, "Rcvd:\n");
    hd_packet(hdr);
    fprintf(stderr, "\n"); */

    if (out_hdr) {
        guarantee(out_hdr->ParseFromArray(slc.beg, hdr.size()), "Failed to parse a header::hdr, when we should have one");
    }

    conn->pop(hdr.size());
    //logINF("Pop:%ld\n", hdr.size());
}

bool read_protob(tcp_conn_t *conn, Message *msg) {
    if (peek_protob(conn, msg)) {
        pop_protob(conn);
        return true;
    } else {
        return false;
    }
}

void write_protob(tcp_conn_t *conn, Message *msg) {
    char buffer[1024];
    guarantee(msg->IsInitialized());

    header::hdr hdr;


    //TODO make this more efficient using zero copy streams
    hdr.set_type(msg->GetTypeName());
    std::string inner;
    guarantee(msg->SerializeToString(&inner));
    hdr.set_msg(inner);
    hdr.set_size(0);
    hdr.set_size(hdr.ByteSize());

    /* fprintf(stderr, "Send:\n");
    hd_packet(hdr);
    fprintf(stderr, "\n"); */

    guarantee(hdr.SerializeToArray(buffer, 1024));
    conn->write(buffer, hdr.ByteSize());
    //logINF("Snd:%d\n", hdr.ByteSize());

    header::hdr _hdr;
}
