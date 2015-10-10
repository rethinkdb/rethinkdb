# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ctypes import *
import struct
import os
import datetime
import uuid

from .utils import *

libc = cdll.LoadLibrary('/usr/lib/libc.dylib')

# Constants
FSOPT_NOFOLLOW         = 0x00000001
FSOPT_NOINMEMUPDATE    = 0x00000002
FSOPT_REPORT_FULLSIZE  = 0x00000004
FSOPT_PACK_INVAL_ATTRS = 0x00000008

VOL_CAPABILITIES_FORMAT     = 0
VOL_CAPABILITIES_INTERFACES = 1

VOL_CAP_FMT_PERSISTENTOBJECTIDS       = 0x00000001
VOL_CAP_FMT_SYMBOLICLINKS             = 0x00000002
VOL_CAP_FMT_HARDLINKS                 = 0x00000004
VOL_CAP_FMT_JOURNAL                   = 0x00000008
VOL_CAP_FMT_JOURNAL_ACTIVE            = 0x00000010
VOL_CAP_FMT_NO_ROOT_TIMES             = 0x00000020
VOL_CAP_FMT_SPARSE_FILES              = 0x00000040
VOL_CAP_FMT_ZERO_RUNS                 = 0x00000080
VOL_CAP_FMT_CASE_SENSITIVE            = 0x00000100
VOL_CAP_FMT_CASE_PRESERVING           = 0x00000200
VOL_CAP_FMT_FAST_STATFS               = 0x00000400
VOL_CAP_FMT_2TB_FILESIZE              = 0x00000800
VOL_CAP_FMT_OPENDENYMODES             = 0x00001000
VOL_CAP_FMT_HIDDEN_FILES              = 0x00002000
VOL_CAP_FMT_PATH_FROM_ID              = 0x00004000
VOL_CAP_FMT_NO_VOLUME_SIZES           = 0x00008000
VOL_CAP_FMT_DECMPFS_COMPRESSION       = 0x00010000
VOL_CAP_FMT_64BIT_OBJECT_IDS          = 0x00020000

VOL_CAP_INT_SEARCHFS                  = 0x00000001
VOL_CAP_INT_ATTRLIST                  = 0x00000002
VOL_CAP_INT_NFSEXPORT                 = 0x00000004
VOL_CAP_INT_READDIRATTR               = 0x00000008
VOL_CAP_INT_EXCHANGEDATA              = 0x00000010
VOL_CAP_INT_COPYFILE                  = 0x00000020
VOL_CAP_INT_ALLOCATE                  = 0x00000040
VOL_CAP_INT_VOL_RENAME                = 0x00000080
VOL_CAP_INT_ADVLOCK                   = 0x00000100
VOL_CAP_INT_FLOCK                     = 0x00000200
VOL_CAP_INT_EXTENDED_SECURITY         = 0x00000400
VOL_CAP_INT_USERACCESS                = 0x00000800
VOL_CAP_INT_MANLOCK                   = 0x00001000
VOL_CAP_INT_NAMEDSTREAMS              = 0x00002000
VOL_CAP_INT_EXTENDED_ATTR             = 0x00004000

ATTR_CMN_NAME                         = 0x00000001
ATTR_CMN_DEVID                        = 0x00000002
ATTR_CMN_FSID                         = 0x00000004
ATTR_CMN_OBJTYPE                      = 0x00000008
ATTR_CMN_OBJTAG                       = 0x00000010
ATTR_CMN_OBJID                        = 0x00000020
ATTR_CMN_OBJPERMANENTID               = 0x00000040
ATTR_CMN_PAROBJID                     = 0x00000080
ATTR_CMN_SCRIPT                       = 0x00000100
ATTR_CMN_CRTIME                       = 0x00000200
ATTR_CMN_MODTIME                      = 0x00000400
ATTR_CMN_CHGTIME                      = 0x00000800
ATTR_CMN_ACCTIME                      = 0x00001000
ATTR_CMN_BKUPTIME                     = 0x00002000
ATTR_CMN_FNDRINFO                     = 0x00004000
ATTR_CMN_OWNERID                      = 0x00008000
ATTR_CMN_GRPID                        = 0x00010000
ATTR_CMN_ACCESSMASK                   = 0x00020000
ATTR_CMN_FLAGS                        = 0x00040000
ATTR_CMN_USERACCESS                   = 0x00200000
ATTR_CMN_EXTENDED_SECURITY            = 0x00400000
ATTR_CMN_UUID                         = 0x00800000
ATTR_CMN_GRPUUID                      = 0x01000000
ATTR_CMN_FILEID                       = 0x02000000
ATTR_CMN_PARENTID                     = 0x04000000
ATTR_CMN_FULLPATH                     = 0x08000000
ATTR_CMN_ADDEDTIME                    = 0x10000000
ATTR_CMN_RETURNED_ATTRS               = 0x80000000
ATTR_CMN_ALL_ATTRS                    = 0x9fe7ffff

ATTR_VOL_FSTYPE				          = 0x00000001
ATTR_VOL_SIGNATURE			          = 0x00000002
ATTR_VOL_SIZE				          = 0x00000004
ATTR_VOL_SPACEFREE			          = 0x00000008
ATTR_VOL_SPACEAVAIL			          = 0x00000010
ATTR_VOL_MINALLOCATION			      = 0x00000020
ATTR_VOL_ALLOCATIONCLUMP		      = 0x00000040
ATTR_VOL_IOBLOCKSIZE			      = 0x00000080
ATTR_VOL_OBJCOUNT			          = 0x00000100
ATTR_VOL_FILECOUNT			          = 0x00000200
ATTR_VOL_DIRCOUNT			          = 0x00000400
ATTR_VOL_MAXOBJCOUNT			      = 0x00000800
ATTR_VOL_MOUNTPOINT			          = 0x00001000
ATTR_VOL_NAME				          = 0x00002000
ATTR_VOL_MOUNTFLAGS			          = 0x00004000
ATTR_VOL_MOUNTEDDEVICE			      = 0x00008000
ATTR_VOL_ENCODINGSUSED			      = 0x00010000
ATTR_VOL_CAPABILITIES			      = 0x00020000
ATTR_VOL_UUID				          = 0x00040000
ATTR_VOL_ATTRIBUTES			          = 0x40000000
ATTR_VOL_INFO				          = 0x80000000
ATTR_VOL_ALL_ATTRS                    = 0xc007ffff

ATTR_DIR_LINKCOUNT			          = 0x00000001
ATTR_DIR_ENTRYCOUNT			          = 0x00000002
ATTR_DIR_MOUNTSTATUS			      = 0x00000004
DIR_MNTSTATUS_MNTPOINT		          = 0x00000001
DIR_MNTSTATUS_TRIGGER			      = 0x00000002
ATTR_DIR_ALL_ATTRS                    = 0x00000007

ATTR_FILE_LINKCOUNT			          = 0x00000001
ATTR_FILE_TOTALSIZE			          = 0x00000002
ATTR_FILE_ALLOCSIZE			          = 0x00000004
ATTR_FILE_IOBLOCKSIZE			      = 0x00000008
ATTR_FILE_DEVTYPE			          = 0x00000020
ATTR_FILE_DATALENGTH			      = 0x00000200
ATTR_FILE_DATAALLOCSIZE			      = 0x00000400
ATTR_FILE_RSRCLENGTH			      = 0x00001000
ATTR_FILE_RSRCALLOCSIZE			      = 0x00002000

ATTR_FILE_ALL_ATTRS                   = 0x0000362f

ATTR_FORK_TOTALSIZE			          = 0x00000001
ATTR_FORK_ALLOCSIZE			          = 0x00000002
ATTR_FORK_ALL_ATTRS                   = 0x00000003

# These can't be used
ATTR_FILE_FORKCOUNT			          = 0x00000080
ATTR_FILE_FORKLIST			          = 0x00000100
ATTR_CMN_NAMEDATTRCOUNT			      = 0x00080000
ATTR_CMN_NAMEDATTRLIST			      = 0x00100000
ATTR_FILE_DATAEXTENTS			      = 0x00000800
ATTR_FILE_RSRCEXTENTS			      = 0x00004000
ATTR_FILE_CLUMPSIZE			          = 0x00000010
ATTR_FILE_FILETYPE			          = 0x00000040

class attrlist(Structure):
    _fields_ = [('bitmapcount', c_ushort),
                ('reserved', c_ushort),
                ('commonattr', c_uint),
                ('volattr', c_uint),
                ('dirattr', c_uint),
                ('fileattr', c_uint),
                ('forkattr', c_uint)]

class attribute_set_t(Structure):
    _fields_ = [('commonattr', c_uint),
                ('volattr', c_uint),
                ('dirattr', c_uint),
                ('fileattr', c_uint),
                ('forkattr', c_uint)]

class fsobj_id_t(Structure):
    _fields_ = [('fid_objno', c_uint),
                ('fid_generation', c_uint)]

class timespec(Structure):
    _fields_ = [('tv_sec', c_long),
                ('tv_nsec', c_long)]

class attrreference_t(Structure):
    _fields_ = [('attr_dataoffset', c_int),
                ('attr_length', c_uint)]

class fsid_t(Structure):
    _fields_ = [('val', c_uint * 2)]

class guid_t(Structure):
    _fields_ = [('g_guid', c_byte*16)]

class kauth_ace(Structure):
    _fields_ = [('ace_applicable', guid_t),
                ('ace_flags', c_uint)]

class kauth_acl(Structure):
    _fields_ = [('acl_entrycount', c_uint),
                ('acl_flags', c_uint),
                ('acl_ace', kauth_ace * 128)]

class kauth_filesec(Structure):
    _fields_ = [('fsec_magic', c_uint),
                ('fsec_owner', guid_t),
                ('fsec_group', guid_t),
                ('fsec_acl', kauth_acl)]

class diskextent(Structure):
    _fields_ = [('startblock', c_uint),
                ('blockcount', c_uint)]

OSType = c_uint
UInt16 = c_ushort
SInt16 = c_short
SInt32 = c_int

class Point(Structure):
    _fields_ = [('x', SInt16),
                ('y', SInt16)]
class Rect(Structure):
    _fields_ = [('x', SInt16),
                ('y', SInt16),
                ('w', SInt16),
                ('h', SInt16)]
class FileInfo(Structure):
    _fields_ = [('fileType', OSType),
                ('fileCreator', OSType),
                ('finderFlags', UInt16),
                ('location', Point),
                ('reservedField', UInt16),
                ('reserved1', SInt16 * 4),
                ('extendedFinderFlags', UInt16),
                ('reserved2', SInt16),
                ('putAwayFolderID', SInt32)]
class FolderInfo(Structure):
    _fields_ = [('windowBounds', Rect),
                ('finderFlags', UInt16),
                ('location', Point),
                ('reservedField', UInt16),
                ('scrollPosition', Point),
                ('reserved1', SInt32),
                ('extendedFinderFlags', UInt16),
                ('reserved2', SInt16),
                ('putAwayFolderID', SInt32)]
class FinderInfo(Union):
    _fields_ = [('fileInfo', FileInfo),
                ('folderInfo', FolderInfo)]

extentrecord = diskextent * 8
        
vol_capabilities_set_t = c_uint * 4

class vol_capabilities_attr_t(Structure):
    _fields_ = [('capabilities', vol_capabilities_set_t),
                ('valid', vol_capabilities_set_t)]

class vol_attributes_attr_t(Structure):
    _fields_ = [('validattr', attribute_set_t),
                ('nativeattr', attribute_set_t)]

dev_t = c_uint

fsobj_type_t = c_uint

VNON = 0
VREG = 1
VDIR = 2
VBLK = 3
VCHR = 4
VLNK = 5
VSOCK = 6
VFIFO = 7
VBAD = 8
VSTR = 9
VCPLX = 10

fsobj_tag_t = c_uint

VT_NON = 0
VT_UFS = 1
VT_NFS = 2
VT_MFS = 3
VT_MSDOSFS = 4
VT_LFS = 5
VT_LOFS = 6
VT_FDESC = 7
VT_PORTAL = 8
VT_NULL = 9
VT_UMAP = 10
VT_KERNFS = 11
VT_PROCFS = 12
VT_AFS = 13
VT_ISOFS = 14
VT_UNION = 15
VT_HFS = 16
VT_ZFS = 17
VT_DEVFS = 18
VT_WEBDAV = 19
VT_UDF = 20
VT_AFP = 21
VT_CDDA = 22
VT_CIFS = 23
VT_OTHER = 24

fsfile_type_t = c_uint
fsvolid_t = c_uint
text_encoding_t = c_uint
uid_t = c_uint
gid_t = c_uint
int32_t = c_int
uint32_t = c_uint
int64_t = c_longlong
uint64_t = c_ulonglong
off_t = c_long
size_t = c_ulong
uuid_t = c_byte*16

NAME_MAX = 255
PATH_MAX = 1024

class struct_statfs(Structure):
    _fields_ = [('f_bsize', uint32_t),
                ('f_iosize', int32_t),
                ('f_blocks', uint64_t),
                ('f_bfree', uint64_t),
                ('f_bavail', uint64_t),
                ('f_files', uint64_t),
                ('f_ffree', uint64_t),
                ('f_fsid', fsid_t),
                ('f_owner', uid_t),
                ('f_type', uint32_t),
                ('f_flags', uint32_t),
                ('f_fssubtype', uint32_t),
                ('f_fstypename', c_char * 16),
                ('f_mntonname', c_char * PATH_MAX),
                ('f_mntfromname', c_char * PATH_MAX),
                ('f_reserved', uint32_t * 8)]

# Calculate the maximum number of bytes required for the attribute buffer
_attr_info = (
    # Common attributes
    (0, ATTR_CMN_RETURNED_ATTRS, sizeof(attribute_set_t)),
    (0, ATTR_CMN_NAME, sizeof(attrreference_t) + NAME_MAX * 3 + 1),
    (0, ATTR_CMN_DEVID, sizeof(dev_t)),
    (0, ATTR_CMN_FSID, sizeof(fsid_t)),
    (0, ATTR_CMN_OBJTYPE, sizeof(fsobj_type_t)),
    (0, ATTR_CMN_OBJTAG, sizeof(fsobj_tag_t)),
    (0, ATTR_CMN_OBJPERMANENTID, sizeof(fsobj_id_t)),
    (0, ATTR_CMN_PAROBJID, sizeof(fsobj_id_t)),
    (0, ATTR_CMN_SCRIPT, sizeof(text_encoding_t)),
    (0, ATTR_CMN_CRTIME, sizeof(timespec)),
    (0, ATTR_CMN_MODTIME, sizeof(timespec)),
    (0, ATTR_CMN_CHGTIME, sizeof(timespec)),
    (0, ATTR_CMN_ACCTIME, sizeof(timespec)),
    (0, ATTR_CMN_BKUPTIME, sizeof(timespec)),
    (0, ATTR_CMN_FNDRINFO, sizeof(FinderInfo)),
    (0, ATTR_CMN_OWNERID, sizeof(uid_t)),
    (0, ATTR_CMN_GRPID, sizeof(gid_t)),
    (0, ATTR_CMN_ACCESSMASK, sizeof(uint32_t)),
    (0, ATTR_CMN_NAMEDATTRCOUNT, None),
    (0, ATTR_CMN_NAMEDATTRLIST, None),
    (0, ATTR_CMN_FLAGS, sizeof(uint32_t)),
    (0, ATTR_CMN_USERACCESS, sizeof(uint32_t)),
    (0, ATTR_CMN_EXTENDED_SECURITY, sizeof(attrreference_t) + sizeof(kauth_filesec)),
    (0, ATTR_CMN_UUID, sizeof(guid_t)),
    (0, ATTR_CMN_GRPUUID, sizeof(guid_t)),
    (0, ATTR_CMN_FILEID, sizeof(uint64_t)),
    (0, ATTR_CMN_PARENTID, sizeof(uint64_t)),
    (0, ATTR_CMN_FULLPATH, sizeof(attrreference_t) + PATH_MAX),
    (0, ATTR_CMN_ADDEDTIME, sizeof(timespec)),

    # Volume attributes
    (1, ATTR_VOL_FSTYPE, sizeof(uint32_t)),
    (1, ATTR_VOL_SIGNATURE, sizeof(uint32_t)),
    (1, ATTR_VOL_SIZE, sizeof(off_t)),
    (1, ATTR_VOL_SPACEFREE, sizeof(off_t)),
    (1, ATTR_VOL_SPACEAVAIL, sizeof(off_t)),
    (1, ATTR_VOL_MINALLOCATION, sizeof(off_t)),
    (1, ATTR_VOL_ALLOCATIONCLUMP, sizeof(off_t)),
    (1, ATTR_VOL_IOBLOCKSIZE, sizeof(uint32_t)),
    (1, ATTR_VOL_OBJCOUNT, sizeof(uint32_t)),
    (1, ATTR_VOL_FILECOUNT, sizeof(uint32_t)),
    (1, ATTR_VOL_DIRCOUNT, sizeof(uint32_t)),
    (1, ATTR_VOL_MAXOBJCOUNT, sizeof(uint32_t)),
    (1, ATTR_VOL_MOUNTPOINT, sizeof(attrreference_t) + PATH_MAX),
    (1, ATTR_VOL_NAME, sizeof(attrreference_t) + NAME_MAX + 1),
    (1, ATTR_VOL_MOUNTFLAGS, sizeof(uint32_t)),
    (1, ATTR_VOL_MOUNTEDDEVICE, sizeof(attrreference_t) + PATH_MAX),
    (1, ATTR_VOL_ENCODINGSUSED, sizeof(c_ulonglong)),
    (1, ATTR_VOL_CAPABILITIES, sizeof(vol_capabilities_attr_t)),
    (1, ATTR_VOL_UUID, sizeof(uuid_t)),
    (1, ATTR_VOL_ATTRIBUTES, sizeof(vol_attributes_attr_t)),

    # Directory attributes
    (2, ATTR_DIR_LINKCOUNT, sizeof(uint32_t)),
    (2, ATTR_DIR_ENTRYCOUNT, sizeof(uint32_t)),
    (2, ATTR_DIR_MOUNTSTATUS, sizeof(uint32_t)),

    # File attributes
    (3, ATTR_FILE_LINKCOUNT, sizeof(uint32_t)),
    (3, ATTR_FILE_TOTALSIZE, sizeof(off_t)),
    (3, ATTR_FILE_ALLOCSIZE, sizeof(off_t)),
    (3, ATTR_FILE_IOBLOCKSIZE, sizeof(uint32_t)),
    (3, ATTR_FILE_CLUMPSIZE, sizeof(uint32_t)),
    (3, ATTR_FILE_DEVTYPE, sizeof(uint32_t)),
    (3, ATTR_FILE_FILETYPE, sizeof(uint32_t)),
    (3, ATTR_FILE_FORKCOUNT, sizeof(uint32_t)),
    (3, ATTR_FILE_FORKLIST, None),
    (3, ATTR_FILE_DATALENGTH, sizeof(off_t)),
    (3, ATTR_FILE_DATAALLOCSIZE, sizeof(off_t)),
    (3, ATTR_FILE_DATAEXTENTS, sizeof(extentrecord)),
    (3, ATTR_FILE_RSRCLENGTH, sizeof(off_t)),
    (3, ATTR_FILE_RSRCALLOCSIZE, sizeof(off_t)),
    (3, ATTR_FILE_RSRCEXTENTS, sizeof(extentrecord)),

    # Fork attributes
    (4, ATTR_FORK_TOTALSIZE, sizeof(off_t)),
    (4, ATTR_FORK_ALLOCSIZE, sizeof(off_t))
    )
    
def _attrbuf_size(attrs):
    size = 4
    for entry in _attr_info:
        if attrs[entry[0]] & entry[1]:
            if entry[2] is None:
                raise ValueError('Unsupported attribute (%u, %x)'
                                 % (entry[0], entry[1]))
            size += entry[2]
    return size

_getattrlist = libc.getattrlist
_getattrlist.argtypes = [c_char_p, POINTER(attrlist), c_void_p, c_ulong, c_ulong]
_getattrlist.restype = c_int

_fgetattrlist = libc.fgetattrlist
_fgetattrlist.argtypes = [c_int, POINTER(attrlist), c_void_p, c_ulong, c_ulong]
_fgetattrlist.restype = c_int

_statfs = libc['statfs$INODE64']
_statfs.argtypes = [c_char_p, POINTER(struct_statfs)]
_statfs.restype = c_int

_fstatfs = libc['fstatfs$INODE64']
_fstatfs.argtypes = [c_int, POINTER(struct_statfs)]
_fstatfs.restype = c_int

def _datetime_from_timespec(ts):
    td = datetime.timedelta(seconds=ts.tv_sec + 1.0e-9 * ts.tv_nsec)
    return unix_epoch + td

def _decode_utf8_nul(sz):
    nul = sz.find('\0')
    if nul > -1:
        sz = sz[:nul]
    return sz.decode('utf-8')

def _decode_attrlist_result(buf, attrs, options):
    result = []

    assert len(buf) >= 4
    total_size = uint32_t.from_buffer(buf, 0).value
    assert total_size <= len(buf)

    offset = 4

    # Common attributes
    if attrs[0] & ATTR_CMN_RETURNED_ATTRS:
        a = attribute_set_t.from_buffer(buf, offset)
        result.append(a)
        offset += sizeof (attribute_set_t)
        if not (options & FSOPT_PACK_INVAL_ATTRS):
            attrs = [a.commonattr, a.volattr, a.dirattr, a.fileattr, a.forkattr]
    if attrs[0] & ATTR_CMN_NAME:
        a = attrreference_t.from_buffer(buf, offset)
        ofs = offset + a.attr_dataoffset
        name = _decode_utf8_nul(buf[ofs:ofs+a.attr_length])
        offset += sizeof (attrreference_t)
        result.append(name)
    if attrs[0] & ATTR_CMN_DEVID:
        a = dev_t.from_buffer(buf, offset)
        offset += sizeof(dev_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_FSID:
        a = fsid_t.from_buffer(buf, offset)
        offset += sizeof(fsid_t)
        result.append(a)
    if attrs[0] & ATTR_CMN_OBJTYPE:
        a = fsobj_type_t.from_buffer(buf, offset)
        offset += sizeof(fsobj_type_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_OBJTAG:
        a = fsobj_tag_t.from_buffer(buf, offset)
        offset += sizeof(fsobj_tag_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_OBJID:
        a = fsobj_id_t.from_buffer(buf, offset)
        offset += sizeof(fsobj_id_t)
        result.append(a)
    if attrs[0] & ATTR_CMN_OBJPERMANENTID:
        a = fsobj_id_t.from_buffer(buf, offset)
        offset += sizeof(fsobj_id_t)
        result.append(a)
    if attrs[0] & ATTR_CMN_PAROBJID:
        a = fsobj_id_t.from_buffer(buf, offset)
        offset += sizeof(fsobj_id_t)
        result.append(a)
    if attrs[0] & ATTR_CMN_SCRIPT:
        a = text_encoding_t.from_buffer(buf, offset)
        offset += sizeof(text_encoding_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_CRTIME:
        a = timespec.from_buffer(buf, offset)
        offset += sizeof(timespec)
        result.append(_datetime_from_timespec(a))
    if attrs[0] & ATTR_CMN_MODTIME:
        a = timespec.from_buffer(buf, offset)
        offset += sizeof(timespec)
        result.append(_datetime_from_timespec(a))
    if attrs[0] & ATTR_CMN_CHGTIME:
        a = timespec.from_buffer(buf, offset)
        offset += sizeof(timespec)
        result.append(_datetime_from_timespec(a))
    if attrs[0] & ATTR_CMN_ACCTIME:
        a = timespec.from_buffer(buf, offset)
        offset += sizeof(timespec)
        result.append(_datetime_from_timespec(a))
    if attrs[0] & ATTR_CMN_BKUPTIME:
        a = timespec.from_buffer(buf, offset)
        offset += sizeof(timespec)
        result.append(_datetime_from_timespec(a))
    if attrs[0] & ATTR_CMN_FNDRINFO:
        a = FinderInfo.from_buffer(buf, offset)
        offset += sizeof(FinderInfo)
        result.append(a)
    if attrs[0] & ATTR_CMN_OWNERID:
        a = uid_t.from_buffer(buf, offset)
        offset += sizeof(uid_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_GRPID:
        a = gid_t.from_buffer(buf, offset)
        offset += sizeof(gid_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_ACCESSMASK:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_FLAGS:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_USERACCESS:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_EXTENDED_SECURITY:
        a = attrreference_t.from_buffer(buf, offset)
        ofs = offset + a.attr_dataoffset
        offset += sizeof(attrreference_t)
        ec = uint32_t.from_buffer(buf, ofs + 36).value
        class kauth_acl(Structure):
            _fields_ = [('acl_entrycount', c_uint),
                        ('acl_flags', c_uint),
                        ('acl_ace', kauth_ace * ec)]
        class kauth_filesec(Structure):
            _fields_ = [('fsec_magic', c_uint),
                        ('fsec_owner', guid_t),
                        ('fsec_group', guid_t),
                        ('fsec_acl', kauth_acl)]
        a = kauth_filesec.from_buffer(buf, ofs)
        result.append(a)
    if attrs[0] & ATTR_CMN_UUID:
        result.append(uuid.UUID(bytes=buf[offset:offset+16]))
        offset += sizeof(guid_t)
    if attrs[0] & ATTR_CMN_GRPUUID:
        result.append(uuid.UUID(bytes=buf[offset:offset+16]))
        offset += sizeof(guid_t)
    if attrs[0] & ATTR_CMN_FILEID:
        a = uint64_t.from_buffer(buf, offset)
        offset += sizeof(uint64_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_PARENTID:
        a = uint64_t.from_buffer(buf, offset)
        offset += sizeof(uint64_t)
        result.append(a.value)
    if attrs[0] & ATTR_CMN_FULLPATH:
        a = attrreference_t.from_buffer(buf, offset)
        ofs = offset + a.attr_dataoffset
        path = _decode_utf8_nul(buf[ofs:ofs+a.attr_length])
        offset += sizeof (attrreference_t)
        result.append(path)
    if attrs[0] & ATTR_CMN_ADDEDTIME:
        a = timespec.from_buffer(buf, offset)
        offset += sizeof(timespec)
        result.append(_datetime_from_timespec(a))

    # Volume attributes
    if attrs[1] & ATTR_VOL_FSTYPE:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_SIGNATURE:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_SIZE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_SPACEFREE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_SPACEAVAIL:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_MINALLOCATION:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_ALLOCATIONCLUMP:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_IOBLOCKSIZE:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_OBJCOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_FILECOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_DIRCOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_MAXOBJCOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_MOUNTPOINT:
        a = attrreference_t.from_buffer(buf, offset)
        ofs = offset + a.attr_dataoffset
        path = _decode_utf8_nul(buf[ofs:ofs+a.attr_length])
        offset += sizeof (attrreference_t)
        result.append(path)
    if attrs[1] & ATTR_VOL_NAME:
        a = attrreference_t.from_buffer(buf, offset)
        ofs = offset + a.attr_dataoffset
        name = _decode_utf8_nul(buf[ofs:ofs+a.attr_length])
        offset += sizeof (attrreference_t)
        result.append(name)
    if attrs[1] & ATTR_VOL_MOUNTFLAGS:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_MOUNTEDDEVICE:
        a = attrreference_t.from_buffer(buf, offset)
        ofs = offset + a.attr_dataoffset
        path = _decode_utf8_nul(buf[ofs:ofs+a.attr_length])
        offset += sizeof (attrreference_t)
        result.append(path)
    if attrs[1] & ATTR_VOL_ENCODINGSUSED:
        a = c_ulonglong.from_buffer(buf, offset)
        offset += sizeof(c_ulonglong)
        result.append(a.value)
    if attrs[1] & ATTR_VOL_CAPABILITIES:
        a = vol_capabilities_attr_t.from_buffer(buf, offset)
        offset += sizeof(vol_capabilities_attr_t)
        result.append(a)
    if attrs[1] & ATTR_VOL_UUID:
        result.append(uuid.UUID(bytes=buf[offset:offset+16]))
        offset += sizeof(uuid_t)
    if attrs[1] & ATTR_VOL_ATTRIBUTES:
        a = vol_attributes_attr_t.from_buffer(buf, offset)
        offset += sizeof(vol_attributes_attr_t)
        result.append(a)

    # Directory attributes
    if attrs[2] & ATTR_DIR_LINKCOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[2] & ATTR_DIR_ENTRYCOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[2] & ATTR_DIR_MOUNTSTATUS:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)

    # File attributes
    if attrs[3] & ATTR_FILE_LINKCOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_TOTALSIZE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_ALLOCSIZE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_IOBLOCKSIZE:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_CLUMPSIZE:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_DEVTYPE:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_FILETYPE:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_FORKCOUNT:
        a = uint32_t.from_buffer(buf, offset)
        offset += sizeof(uint32_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_DATALENGTH:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_DATAALLOCSIZE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_DATAEXTENTS:
        a = extentrecord.from_buffer(buf, offset)
        offset += sizeof(extentrecord)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_RSRCLENGTH:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_RSRCALLOCSIZE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[3] & ATTR_FILE_RSRCEXTENTS:
        a = extentrecord.from_buffer(buf, offset)
        offset += sizeof(extentrecord)
        result.append(a.value)

    # Fork attributes
    if attrs[4] & ATTR_FORK_TOTALSIZE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
    if attrs[4] & ATTR_FORK_ALLOCSIZE:
        a = off_t.from_buffer(buf, offset)
        offset += sizeof(off_t)
        result.append(a.value)
        
    return result

# Sadly, ctypes.get_errno() seems not to work
__error = libc.__error
__error.restype = POINTER(c_int)

def _get_errno():
    return __error().contents.value
                
def getattrlist(path, attrs, options):
    attrs = list(attrs)
    if attrs[1]:
        attrs[1] |= ATTR_VOL_INFO
    alist = attrlist(bitmapcount=5,
                     commonattr=attrs[0],
                     volattr=attrs[1],
                     dirattr=attrs[2],
                     fileattr=attrs[3],
                     forkattr=attrs[4])

    bufsize = _attrbuf_size(attrs)
    buf = create_string_buffer(bufsize)

    ret = _getattrlist(path, byref(alist), buf, bufsize,
                       options | FSOPT_REPORT_FULLSIZE)

    if ret < 0:
        err = _get_errno()
        raise OSError(err, os.strerror(err), path)

    return _decode_attrlist_result(buf, attrs, options)

def fgetattrlist(fd, attrs, options):
    if hasattr(fd, 'fileno'):
        fd = fd.fileno()
    attrs = list(attrs)
    if attrs[1]:
        attrs[1] |= ATTR_VOL_INFO
    alist = attrlist(bitmapcount=5,
                     commonattr=attrs[0],
                     volattr=attrs[1],
                     dirattr=attrs[2],
                     fileattr=attrs[3],
                     forkattr=attrs[4])

    bufsize = _attrbuf_size(attrs)
    buf = create_string_buffer(bufsize)

    ret = _fgetattrlist(fd, byref(alist), buf, bufsize,
                        options | FSOPT_REPORT_FULLSIZE)

    if ret < 0:
        err = _get_errno()
        raise OSError(err, os.strerror(err))

    return _decode_attrlist_result(buf, attrs, options)

def statfs(path):
    result = struct_statfs()
    ret = _statfs(path, byref(result))
    if ret < 0:
        err = _get_errno()
        raise OSError(err, os.strerror(err), path)
    return result

def fstatfs(fd):
    if hasattr(fd, 'fileno'):
        fd = fd.fileno()
    result = struct_statfs()
    ret = _fstatfs(fd, byref(result))
    if ret < 0:
        err = _get_errno()
        raise OSError(err, os.strerror(err))
    return result
