
#ifndef __SERIALIZER_LOG_METABLOCK_METABLOCK_MANAGER_HPP__
#define __SERIALIZER_LOG_METABLOCK_METABLOCK_MANAGER_HPP__

#include "serializer/log/extents/extent_manager.hpp"
#include "arch/arch.hpp"
#include <boost/crc.hpp>
#include <cstddef>
#include <deque>
#include "serializer/log/static_header.hpp"



#define MB_NEXTENTS 2
#define MB_EXTENT_SEPARATION 4 /* !< every MB_EXTENT_SEPARATIONth extent is for MB, up to MB_NEXTENTS many */

#define MB_BAD_VERSION (-1)
#define MB_START_VERSION 1



/* TODO support multiple concurrent writes */
const static char MB_MARKER_MAGIC[8] = {'m', 'e', 't', 'a', 'b', 'l', 'c', 'k'};
const static char MB_MARKER_CRC[4] = {'c', 'r', 'c', ':'};
const static char MB_MARKER_VERSION[8] = {'v', 'e', 'r', 's', 'i', 'o', 'n', ':'};

void initialize_metablock_offsets(off64_t extent_size, std::vector<off64_t> *offsets);



template<class metablock_t>
class metablock_manager_t : private iocallback_t {
    const static uint32_t poly = 0x1337BEEF;

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
        void set_crc() {
            _crc = crc();
        }           

        bool check_crc() {
            return (_crc == crc());
        }
    private:
        uint32_t crc() {
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
            head_t(metablock_manager_t *mgr);
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
    metablock_manager_t(extent_manager_t *em);
    ~metablock_manager_t();

public:
    /* Starts a new database without looking for existing metablocks */
    void start_new(direct_file_t *dbfile);
    
    /* Tries to load existing metablocks */
    struct metablock_read_callback_t {
        virtual void on_metablock_read() = 0;
    };
    bool start_existing(direct_file_t *dbfile, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb);
    
private:
    metablock_read_callback_t *read_callback;
    metablock_t *mb_out; /* !< where to put the metablock once we find it */
    bool *mb_found; /* where to put whether or not we found the metablock */

public:
    struct metablock_write_callback_t {
        virtual void on_metablock_write() = 0;
    };
    bool write_metablock(metablock_t *mb, metablock_write_callback_t *cb);
private:
    struct metablock_write_req_t {
        metablock_write_req_t(metablock_t *, metablock_write_callback_t *);
        metablock_t *mb;
        metablock_write_callback_t *cb;
    };

    metablock_write_callback_t *write_callback;

    std::deque<metablock_write_req_t> outstanding_writes;

public:
    void shutdown();

public:
    void read_next_metablock();

private:
    head_t head; /* !< keeps track of where we are in the extents */
    void on_io_complete(event_t *e);

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
