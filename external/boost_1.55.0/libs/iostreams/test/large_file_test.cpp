/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Tests seeking with a file_descriptor using large file offsets.
 *
 * File:        libs/iostreams/test/large_file_test.cpp
 * Date:        Tue Dec 25 21:34:47 MST 2007
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 */

#include <cstdio>            // SEEK_SET, etc.
#include <ctime>
#include <string>
#include <boost/config.hpp>  // BOOST_STRINGIZE
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/detail/config/rtl.hpp>
#include <boost/iostreams/detail/config/windows_posix.hpp>
#include <boost/iostreams/detail/execute.hpp>
#include <boost/iostreams/detail/ios.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

    // OS-specific headers for low-level i/o.

#include <fcntl.h>       // file opening flags.
#include <sys/stat.h>    // file access permissions.
#ifdef BOOST_IOSTREAMS_WINDOWS
# include <io.h>         // low-level file i/o.
# define WINDOWS_LEAN_AND_MEAN
# include <windows.h>
# ifndef INVALID_SET_FILE_POINTER
#  define INVALID_SET_FILE_POINTER ((DWORD)-1)
# endif
#else
# include <sys/types.h>  // mode_t.
# include <unistd.h>     // low-level file i/o.
#endif  

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using boost::unit_test::test_suite;

//------------------Definition of constants-----------------------------------//

const stream_offset gigabyte = 1073741824;
const stream_offset file_size = // Some compilers complain about "8589934593"
    gigabyte * static_cast<stream_offset>(8) + static_cast<stream_offset>(1);
const int offset_list[] = 
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, // Seek by 1GB
      0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8,       // Seek by 2GB
         6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 
      0, 3, 1, 4, 2, 5, 3, 6, 4, 7, 5, 8,             // Seek by 3GB
         5, 7, 4, 6, 3, 5, 2, 4, 1,
      0, 4, 1, 5, 2, 6, 3, 7, 4, 8,                   // Seek by 4GB
         4, 7, 3, 6, 2, 5, 1, 4,
      0, 5, 1, 6, 2, 7, 3, 8, 3, 7, 2, 6, 1, 5,       // Seek by 5GB
      0, 6, 1, 7, 2, 8, 2, 7, 1, 6,                   // Seek by 6GB
      0, 7, 1, 8, 1, 7,                               // Seek by 7GB
      0, 8, 0 };                                      // Seek by 8GB
const int offset_list_length = sizeof(offset_list) / sizeof(int);
#ifdef LARGE_FILE_TEMP
# define BOOST_FILE_NAME BOOST_STRINGIZE(LARGE_FILE_TEMP)
# define BOOST_KEEP_FILE false
#else
# define BOOST_FILE_NAME BOOST_STRINGIZE(LARGE_FILE_KEEP)
# define BOOST_KEEP_FILE true
#endif

//------------------Definition of remove_large_file---------------------------//

// Removes the large file
void remove_large_file()
{
#ifdef BOOST_IOSTREAMS_WINDOWS
    DeleteFile(TEXT(BOOST_FILE_NAME));
#else
    unlink(BOOST_FILE_NAME);
#endif
}

//------------------Definition of large_file_exists---------------------------//

// Returns true if the large file exists, has the correct size, and has been
// modified since the last commit affecting this source file; if the file exists
// but is invalid, deletes the file.
bool large_file_exists()
{
    // Last mod date
    time_t last_mod;

#ifdef BOOST_IOSTREAMS_WINDOWS

    // Check existence
    WIN32_FIND_DATA info;
    HANDLE hnd = FindFirstFile(TEXT(BOOST_FILE_NAME), &info);
    if (hnd == INVALID_HANDLE_VALUE) 
        return false;

    // Check size
    FindClose(hnd);
    stream_offset size = 
        (static_cast<stream_offset>(info.nFileSizeHigh) << 32) + 
        static_cast<stream_offset>(info.nFileSizeLow);
    if (size != file_size) {
        remove_large_file();
        return false;
    }

    // Fetch last mod date
    SYSTEMTIME stime;
    if (!FileTimeToSystemTime(&info.ftLastWriteTime, &stime)) {
        remove_large_file();
        return false;    
    }
    tm ctime;
    ctime.tm_year = stime.wYear - 1900;
    ctime.tm_mon = stime.wMonth - 1;
    ctime.tm_mday = stime.wDay;
    ctime.tm_hour = stime.wHour;
    ctime.tm_min = stime.wMinute;
    ctime.tm_sec = stime.wSecond;
    ctime.tm_isdst = 0;
    last_mod = mktime(&ctime);

#else

    // Check existence
    struct BOOST_IOSTREAMS_FD_STAT info;
    if (BOOST_IOSTREAMS_FD_STAT(BOOST_FILE_NAME, &info))
        return false;

    // Check size
    if (info.st_size != file_size) {
        remove_large_file();
        return false;
    }

    // Fetch last mod date
    last_mod = info.st_mtime;

#endif

    // Fetch last mod date of this file ("large_file_test.cpp")
    string timestamp = 
        "$Date: 2009-05-20 12:41:20 -0700 (Wed, 20 May 2009) $";
    if (timestamp.size() != 53) { // Length of auto-generated SVN timestamp
        remove_large_file();
        return false;
    }
    tm commit;
    try {
        commit.tm_year = lexical_cast<int>(timestamp.substr(7, 4)) - 1900;
        commit.tm_mon = lexical_cast<int>(timestamp.substr(12, 2)) - 1;
        commit.tm_mday = lexical_cast<int>(timestamp.substr(15, 2));
        commit.tm_hour = lexical_cast<int>(timestamp.substr(18, 2));
        commit.tm_min = lexical_cast<int>(timestamp.substr(21, 2));
        commit.tm_sec = lexical_cast<int>(timestamp.substr(24, 2));
    } catch (const bad_lexical_cast&) {
        remove_large_file();
        return false;
    }

    // If last commit was two days or more before file timestamp, existing 
    // file is okay; otherwise, it must be regenerated (the two-day window 
    // compensates for time zone differences)
    return difftime(last_mod, mktime(&commit)) >= 60 * 60 * 48; 
}

//------------------Definition of map_large_file------------------------------//

// Initializes the large file by mapping it in small segments. This is an
// optimization for Win32; the straightforward implementation using WriteFile 
// and SetFilePointer (see the Borland workaropund below) is painfully slow.
bool map_large_file()
{
    for (stream_offset z = 0; z <= 8; ++z) {
        try {
            mapped_file_params params;
            params.path = BOOST_FILE_NAME;
            params.offset = z * gigabyte;
            params.length = 1;
            params.mode = BOOST_IOS::out;
            mapped_file file(params);
            file.begin()[0] = z + 1;
        } catch (const std::exception&) {
            remove_large_file();
            return false;
        }
    }
    return true;
}

//------------------Definition of create_large_file---------------------------//

// Creates and initializes the large file if it does not already exist. The file 
// looks like this:
//     
//     0     1GB   2GB   3GB   4GB   5GB   6GB   7GB   8GB
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     1.....2.....3.....4.....5.....6.....7.....8.....9
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// where the characters 1-9 appear at offsets that are multiples of 1GB and the
// dots represent uninitialized data.
bool create_large_file()
{
    // If file exists, has correct size, and is recent, we're done
    if (BOOST_KEEP_FILE && large_file_exists())
        return true;

#ifdef BOOST_IOSTREAMS_WINDOWS

    // Create file
    HANDLE hnd =
        CreateFile(
            TEXT(BOOST_FILE_NAME),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    if (!hnd)
        return false;

    // Set file pointer
    LONG off_low = static_cast<LONG>(file_size & 0xffffffff);
    LONG off_high = static_cast<LONG>(file_size >> 32);
    if ( SetFilePointer(hnd, off_low, &off_high, FILE_BEGIN) == 
            INVALID_SET_FILE_POINTER && 
         GetLastError() != NO_ERROR ) 
    {
        CloseHandle(hnd);
        remove_large_file();
        return false;
    }

    // Set file size
    if (!SetEndOfFile(hnd)) {
        CloseHandle(hnd);
        remove_large_file();
        return false;
    }

# if !defined(__BORLANDC__) || __BORLANDC__ < 0x582 || __BORLANDC__ >= 0x592

    // Close handle; all further access is via mapped_file
    CloseHandle(hnd);

    // Initialize file data
    return map_large_file();

# else // Borland >= 5.8.2 and Borland < 5.9.2

    // Initialize file data (very slow, even though only 9 writes are required)
    for (stream_offset z = 0; z <= 8; ++z) {

        // Seek
        LONG off_low = static_cast<LONG>((z * gigabyte) & 0xffffffff); // == 0
        LONG off_high = static_cast<LONG>((z * gigabyte) >> 32);
        if ( SetFilePointer(hnd, off_low, &off_high, FILE_BEGIN) == 
                INVALID_SET_FILE_POINTER && 
             GetLastError() != NO_ERROR ) 
        {
            CloseHandle(hnd);
            remove_large_file();
            return false;
        }

        // Write a character
        char buf[1] = { z + 1 };
        DWORD result;
        BOOL success = WriteFile(hnd, buf, 1, &result, NULL);
        if (!success || result != 1) {
            CloseHandle(hnd);
            remove_large_file();
            return false;
        }
    }

    // Close file
    CloseHandle(hnd);
    return true;

# endif // Borland workaround
#else // #ifdef BOOST_IOSTREAMS_WINDOWS

    // Create file
    int oflag = O_WRONLY | O_CREAT;
    #ifdef _LARGEFILE64_SOURCE
        oflag |= O_LARGEFILE;
    #endif
    mode_t pmode = 
        S_IRUSR | S_IWUSR |
        S_IRGRP | S_IWGRP |
        S_IROTH | S_IWOTH;
    int fd = BOOST_IOSTREAMS_FD_OPEN(BOOST_FILE_NAME, oflag, pmode);
    if (fd == -1)
        return false;

    // Set file size
    if (BOOST_IOSTREAMS_FD_TRUNCATE(fd, file_size)) {
        BOOST_IOSTREAMS_FD_CLOSE(fd);
        return false;
    }

# ifndef __CYGWIN__

    // Initialize file data
    for (int z = 0; z <= 8; ++z) {

        // Seek
        BOOST_IOSTREAMS_FD_OFFSET off = 
            BOOST_IOSTREAMS_FD_SEEK(
                fd,
                static_cast<BOOST_IOSTREAMS_FD_OFFSET>(z * gigabyte),
                SEEK_SET
            );
        if (off == -1) {
            BOOST_IOSTREAMS_FD_CLOSE(fd);
            return false;
        }

        // Write a character
        char buf[1] = { z + 1 };
        if (BOOST_IOSTREAMS_FD_WRITE(fd, buf, 1) == -1) {
            BOOST_IOSTREAMS_FD_CLOSE(fd);
            return false;
        }
    }

    // Close file
    BOOST_IOSTREAMS_FD_CLOSE(fd);
    return true;

# else // Cygwin

    // Close descriptor; all further access is via mapped_file
    BOOST_IOSTREAMS_FD_CLOSE(fd);

    // Initialize file data
    return map_large_file();

# endif 
#endif // #ifdef BOOST_IOSTREAMS_WINDOWS
}

//------------------Definition of large_file----------------------------------//

// RAII utility
class large_file {
public:
    large_file() { exists_ = create_large_file(); }
    ~large_file() { if (!BOOST_KEEP_FILE) remove_large_file(); }
    bool exists() const { return exists_; }
    const char* path() const { return BOOST_FILE_NAME; }
private:
    bool exists_;
};
                    
//------------------Definition of check_character-----------------------------//

// Verify that the given file contains the given character at the current 
// position
bool check_character(file_descriptor_source& file, char value)
{
    char           buf[1];
    int            amt;
    BOOST_CHECK_NO_THROW(amt = file.read(buf, 1));
    BOOST_CHECK_MESSAGE(amt == 1, "failed reading character");
    BOOST_CHECK_NO_THROW(file.seek(-1, BOOST_IOS::cur));
    return buf[0] == value;
}

//------------------Definition of large_file_test-----------------------------//

void large_file_test()
{
    BOOST_REQUIRE_MESSAGE(
        sizeof(stream_offset) >= 8,
        "large offsets not supported"
    );

    // Prepare file and file descriptor
    large_file              large;
    file_descriptor_source  file;
    BOOST_REQUIRE_MESSAGE(
        large.exists(), "failed creating file \"" << BOOST_FILE_NAME << '"'
    );
    BOOST_CHECK_NO_THROW(file.open(large.path(), BOOST_IOS::binary));

    // Test seeking using ios_base::beg
    for (int z = 0; z < offset_list_length; ++z) {
        char value = offset_list[z] + 1;
        stream_offset off = 
            static_cast<stream_offset>(offset_list[z]) * gigabyte;
        BOOST_CHECK_NO_THROW(file.seek(off, BOOST_IOS::beg));
        BOOST_CHECK_MESSAGE(
            check_character(file, value), 
            "failed validating seek"
        );
    }

    // Test seeking using ios_base::end
    for (int z = 0; z < offset_list_length; ++z) {
        char value = offset_list[z] + 1;
        stream_offset off = 
            -static_cast<stream_offset>(8 - offset_list[z]) * gigabyte - 1;
        BOOST_CHECK_NO_THROW(file.seek(off, BOOST_IOS::end));
        BOOST_CHECK_MESSAGE(
            check_character(file, value), 
            "failed validating seek"
        );
    }

    // Test seeking using ios_base::cur
    for (int next, cur = 0, z = 0; z < offset_list_length; ++z, cur = next) {
        next = offset_list[z];
        char value = offset_list[z] + 1;
        stream_offset off = static_cast<stream_offset>(next - cur) * gigabyte;
        BOOST_CHECK_NO_THROW(file.seek(off, BOOST_IOS::cur));
        BOOST_CHECK_MESSAGE(
            check_character(file, value), 
            "failed validating seek"
        );
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("execute test");
    test->add(BOOST_TEST_CASE(&large_file_test));
    return test;
}
