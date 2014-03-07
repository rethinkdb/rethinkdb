/*
 * This file has been donated to Jam.
 */

/*
 * Craig W. McPheeters, Alias|Wavefront.
 *
 * hcache.c hcache.h - handle cacheing of #includes in source files.
 *
 * Create a cache of files scanned for headers. When starting jam, look for the
 * cache file and load it if present. When finished the binding phase, create a
 * new header cache. The cache contains files, their timestamps and the header
 * files found in their scan. During the binding phase of jam, look in the
 * header cache first for the headers contained in a file. If the cache is
 * present and valid, use its contents. This results in dramatic speedups with
 * large projects (e.g. 3min -> 1min startup for one project.)
 *
 * External routines:
 *    hcache_init() - read and parse the local .jamdeps file.
 *    hcache_done() - write a new .jamdeps file.
 *    hcache()      - return list of headers on target. Use cache or do a scan.
 *
 * The dependency file format is an ASCII file with 1 line per target. Each line
 * has the following fields:
 * @boundname@ timestamp_sec timestamp_nsec @file@ @file@ @file@ ...
 */

#ifdef OPT_HEADER_CACHE_EXT

#include "jam.h"
#include "hcache.h"

#include "hash.h"
#include "headers.h"
#include "lists.h"
#include "modules.h"
#include "object.h"
#include "parse.h"
#include "regexp.h"
#include "rules.h"
#include "search.h"
#include "timestamp.h"
#include "variable.h"

typedef struct hcachedata HCACHEDATA ;

struct hcachedata
{
    OBJECT     * boundname;
    timestamp    time;
    LIST       * includes;
    LIST       * hdrscan;    /* the HDRSCAN value for this target */
    int          age;        /* if too old, we will remove it from cache */
    HCACHEDATA * next;
};


static struct hash * hcachehash = 0;
static HCACHEDATA * hcachelist = 0;

static int queries = 0;
static int hits = 0;

#define CACHE_FILE_VERSION "version 5"
#define CACHE_RECORD_HEADER "header"
#define CACHE_RECORD_END "end"


/*
 * Return the name of the header cache file. May return NULL.
 *
 * The user sets this by setting the HCACHEFILE variable in a Jamfile. We cache
 * the result so the user can not change the cache file during header scanning.
 */

static const char * cache_name( void )
{
    static OBJECT * name = 0;
    if ( !name )
    {
        LIST * const hcachevar = var_get( root_module(), constant_HCACHEFILE );

        if ( !list_empty( hcachevar ) )
        {
            TARGET * const t = bindtarget( list_front( hcachevar ) );

            pushsettings( root_module(), t->settings );
            /* Do not expect the cache file to be generated, so pass 0 as the
             * third argument to search. Expect the location to be specified via
             * LOCATE, so pass 0 as the fourth arugment.
             */
            object_free( t->boundname );
            t->boundname = search( t->name, &t->time, 0, 0 );
            popsettings( root_module(), t->settings );

            name = object_copy( t->boundname );
        }
    }
    return name ? object_str( name ) : 0;
}


/*
 * Return the maximum age a cache entry can have before it is purged from the
 * cache.
 */

static int cache_maxage( void )
{
    int age = 100;
    LIST * const var = var_get( root_module(), constant_HCACHEMAXAGE );
    if ( !list_empty( var ) )
    {
        age = atoi( object_str( list_front( var ) ) );
        if ( age < 0 )
            age = 0;
    }
    return age;
}


/*
 * Read a netstring. The caveat is that the string can not contain ASCII 0. The
 * returned value is as returned by object_new().
 */

OBJECT * read_netstring( FILE * f )
{
    unsigned long len;
    static char * buf = NULL;
    static unsigned long buf_len = 0;

    if ( fscanf( f, " %9lu", &len ) != 1 )
        return NULL;
    if ( fgetc( f ) != (int)'\t' )
        return NULL;

    if ( len > 1024 * 64 )
        return NULL;  /* sanity check */

    if ( len > buf_len )
    {
        unsigned long new_len = buf_len * 2;
        if ( new_len < len )
            new_len = len;
        buf = (char *)BJAM_REALLOC( buf, new_len + 1 );
        if ( buf )
            buf_len = new_len;
    }

    if ( !buf )
        return NULL;

    if ( fread( buf, 1, len, f ) != len )
        return NULL;
    if ( fgetc( f ) != (int)'\n' )
        return NULL;

    buf[ len ] = 0;
    return object_new( buf );
}


/*
 * Write a netstring.
 */

void write_netstring( FILE * f, char const * s )
{
    if ( !s )
        s = "";
    fprintf( f, "%lu\t%s\n", (long unsigned)strlen( s ), s );
}


void hcache_init()
{
    FILE       * f;
    OBJECT     * version = 0;
    int          header_count = 0;
    const char * hcachename;

    if ( hcachehash )
        return;

    hcachehash = hashinit( sizeof( HCACHEDATA ), "hcache" );

    if ( !( hcachename = cache_name() ) )
        return;

    if ( !( f = fopen( hcachename, "rb" ) ) )
        return;

    version = read_netstring( f );

    if ( !version || strcmp( object_str( version ), CACHE_FILE_VERSION ) )
        goto bail;

    while ( 1 )
    {
        HCACHEDATA   cachedata;
        HCACHEDATA * c;
        OBJECT * record_type = 0;
        OBJECT * time_secs_str = 0;
        OBJECT * time_nsecs_str = 0;
        OBJECT * age_str = 0;
        OBJECT * includes_count_str = 0;
        OBJECT * hdrscan_count_str = 0;
        int      i;
        int      count;
        LIST   * l;
        int      found;

        cachedata.boundname = 0;
        cachedata.includes = 0;
        cachedata.hdrscan = 0;

        record_type = read_netstring( f );
        if ( !record_type )
        {
            fprintf( stderr, "invalid %s\n", hcachename );
            goto cleanup;
        }
        if ( !strcmp( object_str( record_type ), CACHE_RECORD_END ) )
        {
            object_free( record_type );
            break;
        }
        if ( strcmp( object_str( record_type ), CACHE_RECORD_HEADER ) )
        {
            fprintf( stderr, "invalid %s with record separator <%s>\n",
                hcachename, record_type ? object_str( record_type ) : "<null>" );
            goto cleanup;
        }

        cachedata.boundname = read_netstring( f );
        time_secs_str       = read_netstring( f );
        time_nsecs_str      = read_netstring( f );
        age_str             = read_netstring( f );
        includes_count_str  = read_netstring( f );

        if ( !cachedata.boundname || !time_secs_str || !time_nsecs_str ||
            !age_str || !includes_count_str )
        {
            fprintf( stderr, "invalid %s\n", hcachename );
            goto cleanup;
        }

        timestamp_init( &cachedata.time, atoi( object_str( time_secs_str ) ),
            atoi( object_str( time_nsecs_str ) ) );
        cachedata.age = atoi( object_str( age_str ) ) + 1;

        count = atoi( object_str( includes_count_str ) );
        for ( l = L0, i = 0; i < count; ++i )
        {
            OBJECT * const s = read_netstring( f );
            if ( !s )
            {
                fprintf( stderr, "invalid %s\n", hcachename );
                list_free( l );
                goto cleanup;
            }
            l = list_push_back( l, s );
        }
        cachedata.includes = l;

        hdrscan_count_str = read_netstring( f );
        if ( !hdrscan_count_str )
        {
            fprintf( stderr, "invalid %s\n", hcachename );
            goto cleanup;
        }

        count = atoi( object_str( hdrscan_count_str ) );
        for ( l = L0, i = 0; i < count; ++i )
        {
            OBJECT * const s = read_netstring( f );
            if ( !s )
            {
                fprintf( stderr, "invalid %s\n", hcachename );
                list_free( l );
                goto cleanup;
            }
            l = list_push_back( l, s );
        }
        cachedata.hdrscan = l;

        c = (HCACHEDATA *)hash_insert( hcachehash, cachedata.boundname, &found )
            ;
        if ( !found )
        {
            c->boundname = cachedata.boundname;
            c->includes  = cachedata.includes;
            c->hdrscan   = cachedata.hdrscan;
            c->age       = cachedata.age;
            timestamp_copy( &c->time, &cachedata.time );
        }
        else
        {
            fprintf( stderr, "can not insert header cache item, bailing on %s"
                "\n", hcachename );
            goto cleanup;
        }

        c->next = hcachelist;
        hcachelist = c;

        ++header_count;

        object_free( record_type );
        object_free( time_secs_str );
        object_free( time_nsecs_str );
        object_free( age_str );
        object_free( includes_count_str );
        object_free( hdrscan_count_str );
        continue;

cleanup:

        if ( record_type ) object_free( record_type );
        if ( time_secs_str ) object_free( time_secs_str );
        if ( time_nsecs_str ) object_free( time_nsecs_str );
        if ( age_str ) object_free( age_str );
        if ( includes_count_str ) object_free( includes_count_str );
        if ( hdrscan_count_str ) object_free( hdrscan_count_str );

        if ( cachedata.boundname ) object_free( cachedata.boundname );
        if ( cachedata.includes ) list_free( cachedata.includes );
        if ( cachedata.hdrscan ) list_free( cachedata.hdrscan );

        goto bail;
    }

    if ( DEBUG_HEADER )
        printf( "hcache read from file %s\n", hcachename );

bail:
    if ( version )
        object_free( version );
    fclose( f );
}


void hcache_done()
{
    FILE       * f;
    HCACHEDATA * c;
    int          header_count = 0;
    const char * hcachename;
    int          maxage;

    if ( !hcachehash )
        return;

    if ( !( hcachename = cache_name() ) )
        goto cleanup;

    if ( !( f = fopen( hcachename, "wb" ) ) )
        goto cleanup;

    maxage = cache_maxage();

    /* Print out the version. */
    write_netstring( f, CACHE_FILE_VERSION );

    c = hcachelist;
    for ( c = hcachelist; c; c = c->next )
    {
        LISTITER iter;
        LISTITER end;
        char time_secs_str[ 30 ];
        char time_nsecs_str[ 30 ];
        char age_str[ 30 ];
        char includes_count_str[ 30 ];
        char hdrscan_count_str[ 30 ];

        if ( maxage == 0 )
            c->age = 0;
        else if ( c->age > maxage )
            continue;

        sprintf( includes_count_str, "%lu", (long unsigned)list_length(
            c->includes ) );
        sprintf( hdrscan_count_str, "%lu", (long unsigned)list_length(
            c->hdrscan ) );
        sprintf( time_secs_str, "%lu", (long unsigned)c->time.secs );
        sprintf( time_nsecs_str, "%lu", (long unsigned)c->time.nsecs );
        sprintf( age_str, "%lu", (long unsigned)c->age );

        write_netstring( f, CACHE_RECORD_HEADER );
        write_netstring( f, object_str( c->boundname ) );
        write_netstring( f, time_secs_str );
        write_netstring( f, time_nsecs_str );
        write_netstring( f, age_str );
        write_netstring( f, includes_count_str );
        for ( iter = list_begin( c->includes ), end = list_end( c->includes );
            iter != end; iter = list_next( iter ) )
            write_netstring( f, object_str( list_item( iter ) ) );
        write_netstring( f, hdrscan_count_str );
        for ( iter = list_begin( c->hdrscan ), end = list_end( c->hdrscan );
            iter != end; iter = list_next( iter ) )
            write_netstring( f, object_str( list_item( iter ) ) );
        fputs( "\n", f );
        ++header_count;
    }
    write_netstring( f, CACHE_RECORD_END );

    if ( DEBUG_HEADER )
        printf( "hcache written to %s.   %d dependencies, %.0f%% hit rate\n",
            hcachename, header_count, queries ? 100.0 * hits / queries : 0 );

    fclose ( f );

cleanup:
    for ( c = hcachelist; c; c = c->next )
    {
        list_free( c->includes );
        list_free( c->hdrscan );
        object_free( c->boundname );
    }

    hcachelist = 0;
    if ( hcachehash )
        hashdone( hcachehash );
    hcachehash = 0;
}


LIST * hcache( TARGET * t, int rec, regexp * re[], LIST * hdrscan )
{
    HCACHEDATA * c;

    ++queries;

    if ( ( c = (HCACHEDATA *)hash_find( hcachehash, t->boundname ) ) )
    {
        if ( !timestamp_cmp( &c->time, &t->time ) )
        {
            LIST * const l1 = hdrscan;
            LIST * const l2 = c->hdrscan;
            LISTITER iter1 = list_begin( l1 );
            LISTITER const end1 = list_end( l1 );
            LISTITER iter2 = list_begin( l2 );
            LISTITER const end2 = list_end( l2 );
            while ( iter1 != end1 && iter2 != end2 )
            {
                if ( !object_equal( list_item( iter1 ), list_item( iter2 ) ) )
                    iter1 = end1;
                else
                {
                    iter1 = list_next( iter1 );
                    iter2 = list_next( iter2 );
                }
            }
            if ( iter1 != end1 || iter2 != end2 )
            {
                if ( DEBUG_HEADER )
                {
                    printf( "HDRSCAN out of date in cache for %s\n",
                        object_str( t->boundname ) );
                    printf(" real  : ");
                    list_print( hdrscan );
                    printf( "\n cached: " );
                    list_print( c->hdrscan );
                    printf( "\n" );
                }

                list_free( c->includes );
                list_free( c->hdrscan );
                c->includes = L0;
                c->hdrscan = L0;
            }
            else
            {
                if ( DEBUG_HEADER )
                    printf( "using header cache for %s\n", object_str(
                        t->boundname ) );
                c->age = 0;
                ++hits;
                return list_copy( c->includes );
            }
        }
        else
        {
            if ( DEBUG_HEADER )
                printf ("header cache out of date for %s\n", object_str(
                    t->boundname ) );
            list_free( c->includes );
            list_free( c->hdrscan );
            c->includes = L0;
            c->hdrscan = L0;
        }
    }
    else
    {
        int found;
        c = (HCACHEDATA *)hash_insert( hcachehash, t->boundname, &found );
        if ( !found )
        {
            c->boundname = object_copy( t->boundname );
            c->next = hcachelist;
            hcachelist = c;
        }
    }

    /* 'c' points at the cache entry. Its out of date. */
    {
        LIST * const l = headers1( L0, t->boundname, rec, re );

        timestamp_copy( &c->time, &t->time );
        c->age = 0;
        c->includes = list_copy( l );
        c->hdrscan = list_copy( hdrscan );

        return l;
    }
}

#endif  /* OPT_HEADER_CACHE_EXT */
