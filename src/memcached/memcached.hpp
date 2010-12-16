

struct txt_memcached_handler_t :
    public conn_handler_t,
    public net_conn_read_buffered_callback_t,
    public send_buffer_callback_t
{
    net_conn_t *conn;
    server_t *server;
    send_buffer_t send_buffer;

    txt_memcached_handler_t(net_conn_t *, server_t *);

    bool have_quit;
    void quit();   // Called by conn_acceptor_t when the server is being shut down

    // Called to start the process of reading a new command off the socket
    void read_next_command();

    // These just forward to send_buffer
    void write(const char *buffer, size_t bytes); 
    void writef(const char *buffer, ...);

    void on_send_buffer_flush();
    void on_send_buffer_socket_closed();
    void on_net_conn_read_buffered(const char *buffer, size_t size);
    void on_net_conn_close();
};