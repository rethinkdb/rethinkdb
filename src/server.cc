#include "server.hpp"
#include "db_cpu_info.hpp"
#include <sys/stat.h>

server_t::server_t(cmd_config_t *cmd_config, thread_pool_t *thread_pool)
    : cmd_config(cmd_config), thread_pool(thread_pool),
      log_controller(cmd_config),
      conn_acceptor(this) { }

void server_t::do_start() {
    
    assert_cpu();
    
    printf("Physical cores: %d\n", get_cpu_count());
    printf("Number of DB threads: %d\n", cmd_config->n_workers);
    printf("Total RAM: %ldMB\n", get_total_ram() / 1024 / 1024);
    printf("Free RAM: %ldMB (%.2f%%)\n",
           get_available_ram() / 1024 / 1024,
           (double)get_available_ram() / (double)get_total_ram() * 100.0f);
    printf("Shards total: %d\n", cmd_config->n_slices);
    printf("Max cache memory usage: %lldMB\n", (long long int)(cmd_config->max_cache_size / 1024 / 1024));
    
    do_start_loggers();
}

void server_t::do_start_loggers() {
    printf("Starting loggers...\n");
    if (log_controller.start(this)) on_logger_ready();
}

void server_t::on_logger_ready() {
    do_start_serializers();
}

void server_t::do_start_serializers() {
    
    if (strncmp(cmd_config->db_file_name, DATA_DIRECTORY, strlen(DATA_DIRECTORY)) == 0) {
        mkdir(DATA_DIRECTORY, 0777);
    }
    
    assert_cpu();
    
    printf("Starting serializers...\n");
    messages_out = cmd_config->n_serializers;
    for (int i = 0; i < cmd_config->n_serializers; i++) {
        do_on_cpu(i % get_num_db_cpus(), this, &server_t::start_a_serializer, i);
    }
}

bool server_t::start_a_serializer(int i) {
    
    char name[MAX_DB_FILE_NAME];
    int len = snprintf(name, MAX_DB_FILE_NAME, "%s_%d", cmd_config->db_file_name, i);
    // TODO: the below line is currently the only way to write to a block device,
    // we need a command line way to do it, this also requires consolidating to one
    // file
    //     int len = snprintf(name, MAX_DB_FILE_NAME, "/dev/sdb");
    check("Name too long", len == MAX_DB_FILE_NAME);
    
    serializers[i] = gnew<serializer_t>(cmd_config, name, BTREE_BLOCK_SIZE);
    
    if (serializers[i]->start(this)) on_serializer_ready(serializers[i]);
    return true;
}

void server_t::on_serializer_ready(serializer_t *ser) {
    
    ser->assert_cpu();
    do_on_cpu(home_cpu, this, &server_t::have_started_a_serializer);
}

bool server_t::have_started_a_serializer() {
    
    assert_cpu();
    messages_out--;
    assert(messages_out >= 0);
    if (messages_out == 0) {
        do_start_store();
    }
    return true;
}

void server_t::do_start_store() {

    assert_cpu();
    printf("Starting cache...\n");
    
    store = gnew<store_t>(cmd_config, serializers, cmd_config->n_serializers);
    if (store->start(this)) on_store_ready();
}

void server_t::on_store_ready() {

    assert_cpu();
    do_start_conn_acceptor();
}

void server_t::do_start_conn_acceptor() {
    
    assert_cpu();
    
    conn_acceptor.start();
    
    printf("Server started.\n");
    
    interrupt_message.server = this;
    thread_pool->set_interrupt_message(&interrupt_message);
}

void server_t::shutdown() {
    
    /* This can be called from any CPU! */
    
    cpu_message_t *old_interrupt_msg = thread_pool->set_interrupt_message(NULL);
    
    /* If the interrupt message already was NULL, that means that either shutdown() was for
    some reason called before we finished starting up or shutdown() was called twice and this
    is the second time. */
    if (!old_interrupt_msg) return;
    
    if (continue_on_cpu(home_cpu, old_interrupt_msg))
        call_later_on_this_cpu(old_interrupt_msg);
}

void server_t::do_shutdown() {
    
    assert_cpu();
    printf("Shutting down.\n");
    do_shutdown_conn_acceptor();
}

void server_t::do_shutdown_conn_acceptor() {
    printf("Shutting down connections...\n");
    if (conn_acceptor.shutdown(this)) on_conn_acceptor_shutdown();
}

void server_t::on_conn_acceptor_shutdown() {
    assert_cpu();
    do_shutdown_store();
}

void server_t::do_shutdown_store() {
    printf("Shutting down cache...\n");
    if (store->shutdown(this)) on_store_shutdown();
}

void server_t::on_store_shutdown() {
    assert_cpu();
    gdelete(store);
    store = NULL;
    do_shutdown_serializers();
}

void server_t::do_shutdown_serializers() {
    printf("Shutting down serializers...\n");
    messages_out = cmd_config->n_serializers;
    for (int i = 0; i < cmd_config->n_serializers; i++) {
        do_on_cpu(serializers[i]->home_cpu, this, &server_t::shutdown_a_serializer, i);
    }
}

bool server_t::shutdown_a_serializer(int i) {
    serializers[i]->assert_cpu();
    if (serializers[i]->shutdown(this)) on_serializer_shutdown(serializers[i]);
    return true;
}

void server_t::on_serializer_shutdown(serializer_t *ser) {
    ser->assert_cpu();
    gdelete(ser);
    do_on_cpu(home_cpu, this, &server_t::have_shutdown_a_serializer);
}

bool server_t::have_shutdown_a_serializer() {
    assert_cpu();
    messages_out--;
    assert(messages_out >= 0);
    if (messages_out == 0) {
        do_shutdown_loggers();
    }
    return true;
}

void server_t::do_shutdown_loggers() {
    printf("Shutting down loggers...\n");
    if (log_controller.shutdown(this)) do_message_flush();
}

void server_t::on_logger_shutdown() {
    assert_cpu();
    do_message_flush();
}

/* The penultimate step of shutting down is to make sure that all messages
have reached their destinations so that they can be freed. The way we do this
is to send one final message to each core; when those messages all get back
we know that all messages have been processed properly. Otherwise, logger
shutdown messages would get "stuck" in the message hub when it shut down,
leading to memory leaks. */

struct flush_message_t :
    public cpu_message_t
{
    bool returning;
    server_t *server;
    void on_cpu_switch() {
        if (returning) {
            server->on_message_flush();
            gdelete(this);
        } else {
            returning = true;
            if (continue_on_cpu(server->home_cpu, this)) on_cpu_switch();
        }
    }
};

void server_t::do_message_flush() {
    messages_out = get_num_cpus();
    for (int i = 0; i < get_num_cpus(); i++) {
        flush_message_t *m = gnew<flush_message_t>();
        m->returning = false;
        m->server = this;
        if (continue_on_cpu(i, m)) m->on_cpu_switch();
    }
}

void server_t::on_message_flush() {
    messages_out--;
    if (messages_out == 0) {
        do_stop_threads();
    }
}

void server_t::do_stop_threads() {
    
    assert_cpu();
    // This returns immediately, but will cause all of the threads to stop after we
    // return to the event queue.
    thread_pool->shutdown();
    delete this;
}


bool server_t::stop_gc(gc_stopped_callback_t *cb) {
    return true;
    //    cb->on_gc_stopped();
}

bool server_t::start_gc(gc_started_callback_t *cb) {
    return true;
    // cb->on_gc_started();
}

