extern "C" {

    void *protocol_create(const char *server_str);
    void protocol_destroy(void *);

    void *client_create(int id, int num_clients, int keysize_min, int keysize_max);
    void client_start(void *client);
    void client_lock(void *client);
    void client_unlock(void *client);
    void client_stop(void *client);
    void client_destroy(void *client);

    void *op_create_read(void *client, void *protocol, int freq, int batchfactor_min, int batchfactor_max);
    void *op_create_insert(void *client, void *protocol, int freq, int size_min, int size_max);
    void *op_create_update(void *client, void *protocol, int freq, int size_min, int size_max);
    void *op_create_delete(void *client, void *protocol, int freq);
    void *op_create_appendprepend(void *client, void *protocol, int freq, int is_append, int size_min, int size_max);
    void *op_create_rangeread(void *client, void *protocol, int freq, int rangesize_min, int rangesize_max);
    void op_poll(void *op, int *queries_out, float *worstlatency_out, int *samples_count_inout, float *samples_out);
    void op_reset(void *op);
    void op_destroy(void *op);

    void py_initialize_mysql_table(const char *server_str, int max_key, int max_value);
}