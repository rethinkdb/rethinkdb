struct txt_memcached_handler_t :
    public conn_handler_t,
    public net_conn_read_buffered_callback_t
{
    net_conn_t *conn;
    server_t *server;

    txt_memcached_handler_t(net_conn_t *, server_t *);
    void on_net_conn_read_buffered(const char *buffer, size_t size);
    void on_net_conn_close();
};