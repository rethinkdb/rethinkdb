extern "C" {

    void *client_create(const char *server_str, int id, int num_clients, const char **opts);

    void client_start(void *client);

    void client_poll(void *client,
        int *queries_out, int *inserts_minus_deletes_out,
        float *worst_latency_out,
        int *n_latencies_inout, float *latencies_out);

    void client_stop(void *client);

    void client_destroy(void *client);
}