
#ifndef __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__
#define __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__

#include "../extents/extent_manager.hpp"
#include "arch/io.hpp"
#include <boost/crc.hpp>
#include <cstddef>
#include "serializer/log/static_header.hpp"

#define mb_marker_magic     "metablock"
#define mb_marker_crc       "crc:"
#define mb_marker_version   "version:"

#define MB_NEXTENTS 2 /* !< number of extents must be HARD coded */
#define MB_EXTENT_SEPERATION 4 /* !< every MB_EXTENT_SEPERATIONth extent is for MB, up to MB_EXTENT many */

/* TODO support multiple concurrent writes */

template<class metablock_t>
class naive_metablock_manager_t : private iocallback_t {
    const static uint32_t poly = 0x1337BEEF;

private:
    struct crc_metablock_t {
#ifdef SERIALIZER_MARKERS
        char magic_marker[sizeof(mb_marker_magic)];
#endif
#ifdef SERIALIZER_MARKERS
        char crc_marker[sizeof(mb_marker_crc)];
#endif
        uint32_t            _crc;            /* !< cyclic redundancy check */
#ifdef SERIALIZER_MARKERS
        char version_marker[sizeof(mb_marker_crc)];
#endif
        int             version;
        metablock_t     metablock;
    public:
        uint32_t crc() {
            //TODO this doesn't do the version
            boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
            crc_computer.process_bytes(&metablock, sizeof(metablock));
            //crc_computer.process_bytes(&version, sizeof(version)); for some reason this causes crc to be wrong
            return crc_computer.checksum();
        }
        void set_crc() {
            _crc = crc();
        }           

        bool check_crc() {
            return (_crc == crc());
        }
    };
/* \brief struct head_t is used to keep track of where we are writing or reading the metablock from
 */
private:
    struct head_t {
        private:
            uint32_t mb_slot; /* !< how many metablocks have been written in this extent */
            uint32_t extent; /* !< which of our extents we're on */
            uint32_t saved_mb_slot;
            uint32_t saved_extent;
        public:
            size_t extent_size;
        public:
            bool wraparound; /* !< whether or not we've wrapped around the edge (used during startup) */
        public:
            head_t();

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
    naive_metablock_manager_t(extent_manager_t *em);
    ~naive_metablock_manager_t();

public:
    struct metablock_read_callback_t {
        virtual void on_metablock_read() = 0;
    };
    bool start(fd_t dbfd, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb);
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
    metablock_write_callback_t *write_callback;

public:
    void shutdown();

public:
    void write_headers();
    void read_headers();

private:
    head_t head; /* !< keeps track of where we are in the extents */
    void on_io_complete(event_t *e);
    
    crc_metablock_t *mb_buffer;
    bool            mb_buffer_in_use;   /* !< true: we're using the buffer, no one else can */
private:
    /* these are only used in the beginning when we want to find the metablock */
    crc_metablock_t *mb_buffer_last;    /* the last metablock we read */
    int             version;            /* !< only used during boot up */
    
    extent_manager_t *extent_manager;
    
    enum state_t {
        state_unstarted,
        state_reading,
        state_reading_header,
        state_writing_header,
        state_ready,
        state_writing,
        state_shut_down,
    } state;
    
    fd_t dbfd;
private:
    static_header *hdr;
    int hdr_ref_count;
};

#include "naive.tcc"

#endif /* __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__ */
