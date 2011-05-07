
#ifndef __SERIALIZER_LOG_METABLOCK_METABLOCK_MANAGER_HPP__
#define __SERIALIZER_LOG_METABLOCK_METABLOCK_MANAGER_HPP__

#include "serializer/log/extents/extent_manager.hpp"
#include "arch/arch.hpp"
#include <boost/crc.hpp>
#include <cstddef>
#include <deque>
#include "serializer/log/static_header.hpp"
#include "concurrency/mutex.hpp"



#define MB_NEXTENTS 2
#define MB_EXTENT_SEPARATION 4 /* !< every MB_EXTENT_SEPARATIONth extent is for MB, up to MB_NEXTENTS many */

#define MB_BAD_VERSION (-1)
#define MB_START_VERSION 1



/* TODO support multiple concurrent writes */
static const char MB_MARKER_MAGIC[8] = {'m', 'e', 't', 'a', 'b', 'l', 'c', 'k'};
static const char MB_MARKER_CRC[4] = {'c', 'r', 'c', ':'};
static const char MB_MARKER_VERSION[8] = {'v', 'e', 'r', 's', 'i', 'o', 'n', ':'};

void initialize_metablock_offsets(off64_t extent_size, std::vector<off64_t> *offsets);



template<class metablock_t>
class metablock_manager_t {
public:
    typedef int64_t metablock_version_t;

    // This is stored directly to disk.  Changing it will change the disk format.
    struct crc_metablock_t {
        char magic_marker[sizeof(MB_MARKER_MAGIC)];
        char crc_marker[sizeof(MB_MARKER_CRC)];
        uint32_t _crc;            /* !< cyclic redundancy check */
        char version_marker[sizeof(MB_MARKER_VERSION)];
        metablock_version_t version;
        metablock_t metablock;
    public:
        void prepare(metablock_t *mb, metablock_version_t vers) {
            metablock = *mb;
            memcpy(magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC));
            memcpy(crc_marker, MB_MARKER_CRC, sizeof(MB_MARKER_CRC));
            memcpy(version_marker, MB_MARKER_VERSION, sizeof(MB_MARKER_VERSION));
            version = vers;
            _crc = compute_own_crc();
        }
        bool check_crc() {
            return (_crc == compute_own_crc());
        }
    private:
        uint32_t compute_own_crc() {
            boost::crc_32_type crc_computer;
            crc_computer.process_bytes(&version, sizeof(version));
            crc_computer.process_bytes(&metablock, sizeof(metablock));
            return crc_computer.checksum();
        }
    };
/* \brief struct head_t is used to keep track of where we are writing or reading the metablock from
 */
private:
    struct head_t {
        private:
            uint32_t mb_slot;
            uint32_t saved_mb_slot;
        public:
            bool wraparound; /* !< whether or not we've wrapped around the edge (used during startup) */
        public:
            explicit head_t(metablock_manager_t *mgr);
            metablock_manager_t *mgr;

            /* \brief handles moving along successive mb slots
             */
            void operator++(int);
            /* \brief return the offset we should be writing to
             */
            off64_t offset();
            /* \brief save the state to be loaded later (used to save the last known uncorrupted metablock)
             */
            void push();
            /* \brief load a previously saved state (stack has max depth one)
             */
            void pop();
    };

public:
    explicit metablock_manager_t(extent_manager_t *em);
    ~metablock_manager_t();

public:
    /* Clear metablock slots and write an initial metablock to the database file */
    static void create(direct_file_t *dbfile, off64_t extent_size, metablock_t *initial);
    
    /* Tries to load existing metablocks */
    void co_start_existing(direct_file_t *dbfile, bool *mb_found, metablock_t *mb_out);
    struct metablock_read_callback_t {
        virtual void on_metablock_read() = 0;
        virtual ~metablock_read_callback_t() {}
    };
private:
    void start_existing_callback(direct_file_t *dbfile, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb);
public:
    bool start_existing(direct_file_t *dbfile, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb);

public:
    struct metablock_write_callback_t {
        virtual void on_metablock_write() = 0;
        virtual ~metablock_write_callback_t() {}
    };
    bool write_metablock(metablock_t *mb, file_t::account_t *io_account, metablock_write_callback_t *cb);
private:
    void write_metablock_callback(metablock_t *mb, file_t::account_t *io_account, metablock_write_callback_t *cb);
public:
    void co_write_metablock(metablock_t *mb, file_t::account_t *io_account);

private:
    mutex_t write_lock;

public:
    void shutdown();

public:
    void read_next_metablock();

private:
    head_t head; /* !< keeps track of where we are in the extents */
    void on_io_complete();

    metablock_version_t next_version_number;

    crc_metablock_t *mb_buffer;
    bool mb_buffer_in_use;   /* !< true: we're using the buffer, no one else can */

    // Just some compartmentalization to make this mildly cleaner.
    struct startup {
        /* these are only used in the beginning when we want to find the metablock */
        crc_metablock_t *mb_buffer_last;
        metablock_version_t version;
    } startup_values;
        
    // swaps &mb_buffer and &mb_buffer_last.
    void swap_buffers();

    extent_manager_t *extent_manager;
    
    std::vector<off64_t> metablock_offsets;
    
    enum state_t {
        state_unstarted,
        state_reading,
        state_ready,
        state_writing,
        state_shut_down,
    } state;
    
    direct_file_t *dbfile;
};

#include "metablock_manager.tcc"

#endif /* __SERIALIZER_LOG_METABLOCK_METABLOCK_MANAGER_HPP__ */
