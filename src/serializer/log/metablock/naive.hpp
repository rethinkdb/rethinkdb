
#ifndef __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__
#define __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__

#include "../extents/extent_manager.hpp"
#include "arch/io.hpp"
#include <boost/crc.hpp>
#include <cstddef>

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

private:
    uint32_t  mb_written; /* !< how many metablocks have been written in this extent */
    uint32_t  extent; /* !< which of our extents we're on */
    /* \brief incr_mb_location returns true if we wrap around from the last extent to the first
     */
    bool incr_mb_location() {
        mb_written++;
        if (mb_written >= extent_manager->extent_size / DEVICE_BLOCK_SIZE) {
            mb_written = 0;
            extent += MB_EXTENT_SEPERATION;
            if (extent >= MB_NEXTENTS * MB_EXTENT_SEPERATION) {
                extent = 0;
                return true;
            }
        }
        return false;
    }
    void on_io_complete(event_t *e);
    
    crc_metablock_t *mb_buffer;
    bool            mb_buffer_in_use;   /* !< true: we're using the buffer, no one else can */
private:
    /* these are only used in the beginning when we want to find the metablock */
    crc_metablock_t *mb_buffer_last;    /* the last metablock we read */
    int             version;            /* !< only used during boot up */
    int             last_mb_written;    /* !< where the last mb was found */
    int             last_mb_extent;     /* !< where the last was found */ 
    
    extent_manager_t *extent_manager;
    
    enum state_t {
        state_unstarted,
        state_reading,
        state_ready,
        state_writing,
        state_shut_down
    } state;
    
    fd_t dbfd;
};

#include "naive.tcc"

#endif /* __SERIALIZER_LOG_METABLOCK_NAIVE_HPP__ */
