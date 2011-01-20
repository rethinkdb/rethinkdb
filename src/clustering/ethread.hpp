
#ifndef __ETHREAD_HPP__
#define __ETHREAD_HPP__

/* A message_serializer_id_t is the first part of any message. It specifies what type of message
it is and how to handle it. */
typedef uint8_t message_serializer_id_t;

/* A message_serializer_id_t identifies a message_serializer_t. The message_serializer_t knows how
to turn the message into a byte stream and reconstruct the message_t from a byte stream. Typically
a message_serializer_t is associated with a single subclass of message_t. */
struct message_serializer_t {

    /* The message_serializer_id_t must be unique among all message_serializer_t objects. It's used
    to decide which one to use to reconstruct a message when it arrives over the network. */
    message_serializer_t(message_serializer_id_t);
    
    // The message_t is guaranteed to have been constructed with this message_serializer_t.
    // Typically, you would static_cast the message_t to the particular type you were expecting and
    // then serialize it in a type-specific way. write() is responsible for deleting the message_t
    // that is passed to it!
    virtual void serialize(tcp_conn_t *, message_t *) = 0;
    
    // 'delete' will eventually be called on the return value from read()
    virtual message_t *unserialize(tcp_conn_t *) = 0;
};

struct message_t {
    message_t(message_serializer_t *);
    virtual void run() = 0;
};

struct ethread_t {
    /* send() returns immediately and it destroys the message in question. */
    virtual void send(message_t *) = 0;
};

/* In general, you should not use message_serializer_t and message_t directly; instead, you should
use templated classes built on top of them. With that in mind, here is an example of how to use
message_serializer_t and message_t to print "Hello: %d" on an arbitrary ethread:
    
    #define HELLO_MESSAGE_ID 1   // Must be unique
    
    // In global scope
    struct hello_serializer_t : public message_serializer_t {
        void serialize(tcp_conn_t *conn, message_t *msg);
        message_t *unserialize(tcp_conn_t *conn);
    } hello_serializer(HELLO_MESSAGE_ID);
    
    struct hello_message_t : public message_t {
        int i;
        hello_message_t(int i) : message_t(&hello_serializer), i(i) { }
        void run() {
            printf("Hello: %d\n", i);
        }
    };
    
    void hello_serializer_t::serialize(tcp_conn_t *conn, message_t *msg); {
        int i = static_cast<hello_message_t>(msg)->i;
        conn->write(&i, sizeof(int);
    }
    message_t *hello_serialize_t::unserialize(tcp_conn_t *conn); {
        int i;
        conn->read(&i, sizeof(int));
        return new hello_message_t(i);
    }
    
    // To send a message, call some_ethread->send(new hello_message_t(some_number))

This is a lot of work for a simple hello. So we introduce templates that make it easier to create
new message types. Using the templates, this example could be done as follows:

    #define HELLO_MESSAGE_ID 1   // Must be unique
    
    // In global scope
    struct : public async_message_class_t<int> {
        void run(int i) {
            printf("Hello: %d\n", i);
        }
    } hello_message(HELLO_MESSAGE_ID);
    
    // To send a message, call some_ethread->send(hello_message(some_number))

Clearly this is much simpler. The serialize() and unserialize() functions must be overloaded for
any data types that you want to pass as arguments; they are already overloaded for most commonly
used data types. */

/* Zero-argument asynchronous message class */

template<>
class async_message_class_t :
    private message_serializer_t
{
private:
    struct msg_t : public message_t {
        async_message_class_t *parent;
        msg_t(async_message_class_t *p) : message_t(p), parent(p) { }
        void run() {
            p->run();
        }
    };
    virtual void serialize(tcp_conn_t *conn, message_t *message) {
        /* Nothing to do */
    }
    virtual message_t *unserialize(tcp_conn_t *conn) {
        return (*this)();
    }
    virtual void run() = 0;
public:
    message_t *operator()() {
        return new msg_t(this);
    }
};

/* One-argument asynchronous message class */

template<class arg1_t>
class async_message_class_t :
    private message_serializer_t
{
private:
    struct msg_t : public message_t {
        async_message_class_t *parent;
        arg1_t arg1;
        msg_t(async_message_class_t *p, arg1_t a1) : message_t(p), parent(p), arg1(a1) { }
        void run() {
            p->run(arg1);
        }
    };
    virtual void serialize(tcp_conn_t *conn, message_t *message) {
        msg_t *m = static_cast<msg_t>(message);
        ::serialize(conn, m->arg1);
    }
    virtual message_t *unserialize(tcp_conn_t *conn) {
        arg1_t arg1;
        ::unserialize(conn, &arg1);
        return (*this)(arg1);
    }
    virtual void run(arg1_t) = 0;
public:
    message_t *operator()(arg1_t a1) {
        return new msg_t(this, a1);
    }
};

/* Two-argument asynchronous message class */

template<class arg1_t, class arg2_t>
class async_message_class_t :
    private message_serializer_t
{
private:
    struct msg_t : public message_t {
        async_message_class_t *parent;
        arg1_t arg1;
        arg2_t arg2;
        msg_t(async_message_class_t *p, arg1_t a1, arg2_t a2)
            : message_t(p), parent(p), arg1(a1), arg2(a2) { }
        void run() {
            p->run(arg1, arg2);
        }
    };
    virtual void serialize(tcp_conn_t *conn, message_t *message) {
        msg_t *m = static_cast<msg_t>(message);
        ::serialize(conn, m->arg1);
        ::serialize(conn, m->arg2);
    }
    virtual message_t *unserialize(tcp_conn_t *conn) {
        arg1_t arg1;
        ::unserialize(conn, &arg1);
        arg2_t arg2;
        ::unserialize(conn, &arg2);
        return (*this)(arg1, arg2);
    }
    virtual void run(arg1_t, arg2_t) = 0;
public:
    message_t *operator()(arg1_t a1, arg2_t a2) {
        return new msg_t(this, a1, a2);
    }
};

/* Zero-argument synchronous message class */



#endif /* __ETHREAD_HPP__ */
