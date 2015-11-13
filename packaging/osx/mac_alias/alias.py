# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from __future__ import division

import struct
import datetime
import io
import re
import os
import os.path
import stat
import sys

if sys.platform == 'darwin':
    from . import osx

from .utils import *
    
ALIAS_KIND_FILE = 0
ALIAS_KIND_FOLDER = 1

ALIAS_HFS_VOLUME_SIGNATURE = b'H+'

ALIAS_FIXED_DISK = 0
ALIAS_NETWORK_DISK = 1
ALIAS_400KB_FLOPPY_DISK = 2
ALIAS_800KB_FLOPPY_DISK = 3
ALIAS_1_44MB_FLOPPY_DISK = 4
ALIAS_EJECTABLE_DISK = 5

ALIAS_NO_CNID = 0xffffffff

class AppleShareInfo (object):
    def __init__(self, zone=None, server=None, user=None):
        #: The AppleShare zone
        self.zone = zone
        #: The AFP server
        self.server = server
        #: The username
        self.user = user

    def __repr__(self):
        return 'AppleShareInfo(%r,%r,%r)' % (self.zone, self.server, self.user)
    
class VolumeInfo (object):
    def __init__(self, name, creation_date, fs_type, disk_type,
                 attribute_flags, fs_id, appleshare_info=None,
                 driver_name=None, posix_path=None, disk_image_alias=None,
                 dialup_info=None, network_mount_info=None):
        #: The name of the volume on which the target resides
        self.name = name
        
        #: The creation date of the target's volume
        self.creation_date = creation_date
        
        #: The filesystem type (a two character code, e.g. ``b'H+'`` for HFS+)
        self.fs_type = fs_type
        
        #: The type of disk; should be one of
        #:
        #:   * ALIAS_FIXED_DISK
        #:   * ALIAS_NETWORK_DISK
        #:   * ALIAS_400KB_FLOPPY_DISK
        #:   * ALIAS_800KB_FLOPPY_DISK
        #:   * ALIAS_1_44MB_FLOPPY_DISK
        #:   * ALIAS_EJECTABLE_DISK
        self.disk_type = disk_type

        #: Filesystem attribute flags (from HFS volume header)
        self.attribute_flags = attribute_flags

        #: Filesystem identifier
        self.fs_id = fs_id

        #: AppleShare information (for automatic remounting of network shares)
        #: *(optional)*
        self.appleshare_info = appleshare_info

        #: Driver name (*probably* contains a disk driver name on older Macs)
        #: *(optional)*
        self.driver_name = driver_name

        #: POSIX path of the mount point of the target's volume
        #: *(optional)*
        self.posix_path = posix_path

        #: :class:`Alias` object pointing at the disk image on which the
        #: target's volume resides *(optional)*
        self.disk_image_alias = disk_image_alias

        #: Dialup information (for automatic establishment of dialup connections)
        self.dialup_info = dialup_info

        #: Network mount information (for automatic remounting)
        self.network_mount_info = network_mount_info

    def __repr__(self):
        args = ['name', 'creation_date', 'fs_type', 'disk_type',
                'attribute_flags', 'fs_id']
        values = []
        for a in args:
            v = getattr(self, a)
            values.append(repr(v))
            
        kwargs = ['appleshare_info', 'driver_name', 'posix_path',
                  'disk_image_alias', 'dialup_info', 'network_mount_info']
        for a in kwargs:
            v = getattr(self, a)
            if v is not None:
                values.append('%s=%r' % (a, v))
        return 'VolumeInfo(%s)' % ','.join(values)
    
class TargetInfo (object):
    def __init__(self, kind, filename, folder_cnid, cnid, creation_date,
                 creator_code, type_code, levels_from=-1, levels_to=-1,
                 folder_name=None, cnid_path=None, carbon_path=None,
                 posix_path=None, user_home_prefix_len=None):
        #: Either ALIAS_KIND_FILE or ALIAS_KIND_FOLDER
        self.kind = kind
        
        #: The filename of the target
        self.filename = filename

        #: The CNID (Catalog Node ID) of the target's containing folder;
        #: CNIDs are similar to but different than traditional UNIX inode
        #: numbers
        self.folder_cnid = folder_cnid

        #: The CNID (Catalog Node ID) of the target
        self.cnid = cnid

        #: The target's *creation* date.
        self.creation_date = creation_date

        #: The target's Mac creator code (a four-character binary string)
        self.creator_code = creator_code

        #: The target's Mac type code (a four-character binary string)
        self.type_code = type_code

        #: The depth of the alias? Always seems to be -1 on OS X.
        self.levels_from = levels_from

        #: The depth of the target? Always seems to be -1 on OS X.
        self.levels_to = levels_to

        #: The (POSIX) name of the target's containing folder. *(optional)*
        self.folder_name = folder_name

        #: The path from the volume root as a sequence of CNIDs. *(optional)*
        self.cnid_path = cnid_path

        #: The Carbon path of the target *(optional)*
        self.carbon_path = carbon_path

        #: The POSIX path of the target relative to the volume root.  Note
        #: that this may or may not have a leading '/' character, but it is
        #: always relative to the containing volume. *(optional)*
        self.posix_path = posix_path

        #: If the path points into a user's home folder, the number of folders
        #: deep that we go before we get to that home folder. *(optional)*
        self.user_home_prefix_len = user_home_prefix_len

    def __repr__(self):
        args = ['kind', 'filename', 'folder_cnid', 'cnid', 'creation_date',
                'creator_code', 'type_code']
        values = []
        for a in args:
            v = getattr(self, a)
            values.append(repr(v))

        if self.levels_from != -1:
            values.append('levels_from=%r' % self.levels_from)
        if self.levels_to != -1:
            values.append('levels_to=%r' % self.levels_to)

        kwargs = ['folder_name', 'cnid_path', 'carbon_path',
                  'posix_path', 'user_home_prefix_len']
        for a in kwargs:
            v = getattr(self, a)
            values.append('%s=%r' % (a, v))

        return 'TargetInfo(%s)' % ','.join(values)
    
TAG_CARBON_FOLDER_NAME = 0
TAG_CNID_PATH = 1
TAG_CARBON_PATH = 2
TAG_APPLESHARE_ZONE = 3
TAG_APPLESHARE_SERVER_NAME = 4
TAG_APPLESHARE_USERNAME = 5
TAG_DRIVER_NAME = 6
TAG_NETWORK_MOUNT_INFO = 9
TAG_DIALUP_INFO = 10
TAG_UNICODE_FILENAME = 14
TAG_UNICODE_VOLUME_NAME = 15
TAG_HIGH_RES_VOLUME_CREATION_DATE = 16
TAG_HIGH_RES_CREATION_DATE = 17
TAG_POSIX_PATH = 18
TAG_POSIX_PATH_TO_MOUNTPOINT = 19
TAG_RECURSIVE_ALIAS_OF_DISK_IMAGE = 20
TAG_USER_HOME_LENGTH_PREFIX = 21

class Alias (object):
    def __init__(self, appinfo=b'\0\0\0\0', version=2, volume=None,
                       target=None, extra=[]):
        """Construct a new :class:`Alias` object with the specified
        contents."""

        #: Application specific information (four byte byte-string)
        self.appinfo = appinfo

        #: Version (we support only version 2)
        self.version = version

        #: A :class:`VolumeInfo` object describing the target's volume
        self.volume = volume
        
        #: A :class:`TargetInfo` object describing the target
        self.target = target

        #: A list of extra `(tag, value)` pairs
        self.extra = list(extra)

    @classmethod
    def _from_fd(cls, b):
        appinfo, recsize, version = struct.unpack(b'>4shh', b.read(8))

        if recsize < 150:
            raise ValueError('Incorrect alias length')
                        
        if version != 2:
            raise ValueError('Unsupported alias version %u' % version)

        kind, volname, voldate, fstype, disktype, \
        folder_cnid, filename, cnid, crdate, creator_code, type_code, \
        levels_from, levels_to, volattrs, volfsid, reserved = \
              struct.unpack(b'>h28pI2shI64pII4s4shhI2s10s', b.read(142))

        voldate = mac_epoch + datetime.timedelta(seconds=voldate)
        crdate = mac_epoch + datetime.timedelta(seconds=crdate)

        alias = Alias()
        alias.appinfo = appinfo
            
        alias.volume = VolumeInfo (volname.replace('/',':'),
                                   voldate, fstype, disktype,
                                   volattrs, volfsid)
        alias.target = TargetInfo (kind, filename.replace('/',':'),
                                   folder_cnid, cnid,
                                   crdate, creator_code, type_code)
        alias.target.levels_from = levels_from
        alias.target.levels_to = levels_to
        
        tag = struct.unpack(b'>h', b.read(2))[0]

        while tag != -1:
            length = struct.unpack(b'>h', b.read(2))[0]
            value = b.read(length)
            if length & 1:
                b.read(1)

            if tag == TAG_CARBON_FOLDER_NAME:
                alias.target.folder_name = value.replace('/',':')
            elif tag == TAG_CNID_PATH:
                alias.target.cnid_path = struct.unpack(b'>%uI' % (length // 4),
                                                           value)
            elif tag == TAG_CARBON_PATH:
                alias.target.carbon_path = value
            elif tag == TAG_APPLESHARE_ZONE:
                if alias.volume.appleshare_info is None:
                    alias.volume.appleshare_info = AppleShareInfo()
                alias.volume.appleshare_info.zone = value
            elif tag == TAG_APPLESHARE_SERVER_NAME:
                if alias.volume.appleshare_info is None:
                    alias.volume.appleshare_info = AppleShareInfo()
                alias.volume.appleshare_info.server = value
            elif tag == TAG_APPLESHARE_USERNAME:
                if alias.volume.appleshare_info is None:
                    alias.volume.appleshare_info = AppleShareInfo()
                alias.volume.appleshare_info.user = value
            elif tag == TAG_DRIVER_NAME:
                alias.volume.driver_name = value
            elif tag == TAG_NETWORK_MOUNT_INFO:
                alias.volume.network_mount_info = value
            elif tag == TAG_DIALUP_INFO:
                alias.volume.dialup_info = value
            elif tag == TAG_UNICODE_FILENAME:
                alias.target.filename = value[2:].decode('utf-16be')
            elif tag == TAG_UNICODE_VOLUME_NAME:
                alias.volume.name = value[2:].decode('utf-16be')
            elif tag == TAG_HIGH_RES_VOLUME_CREATION_DATE:
                seconds = struct.unpack(b'>Q', value)[0] / 65536.0
                alias.volume.creation_date \
                    = mac_epoch + datetime.timedelta(seconds=seconds)
            elif tag == TAG_HIGH_RES_CREATION_DATE:
                seconds = struct.unpack(b'>Q', value)[0] / 65536.0
                alias.target.creation_date \
                    = mac_epoch + datetime.timedelta(seconds=seconds)
            elif tag == TAG_POSIX_PATH:
                alias.target.posix_path = value
            elif tag == TAG_POSIX_PATH_TO_MOUNTPOINT:
                alias.volume.posix_path = value
            elif tag == TAG_RECURSIVE_ALIAS_OF_DISK_IMAGE:
                alias.volume.disk_image_alias = Alias.from_bytes(value)
            elif tag == TAG_USER_HOME_LENGTH_PREFIX:
                alias.target.user_home_prefix_len = struct.unpack(b'>h', value)[0]
            else:
                alias.extra.append((tag, value))

            tag = struct.unpack(b'>h', b.read(2))[0]
            
        return alias
         
    @classmethod
    def from_bytes(cls, bytes):
        """Construct an :class:`Alias` object given binary Alias data."""
        with io.BytesIO(bytes) as b:
            return cls._from_fd(b)

    @classmethod
    def for_file(cls, path):
        """Create an :class:`Alias` that points at the specified file."""
        if sys.platform != 'darwin':
            raise Exception('Not implemented (requires special support)')
        
        a = Alias()

        # Find the filesystem
        st = osx.statfs(path)
        vol_path = st.f_mntonname
        
        # Grab its attributes
        attrs = [osx.ATTR_CMN_CRTIME,
                 osx.ATTR_VOL_NAME,
                 0, 0, 0]
        volinfo = osx.getattrlist(vol_path, attrs, 0)

        vol_crtime = volinfo[0]
        vol_name = volinfo[1]
        
        # Also grab various attributes of the file
        attrs = [(osx.ATTR_CMN_OBJTYPE
                  | osx.ATTR_CMN_CRTIME
                  | osx.ATTR_CMN_FNDRINFO
                  | osx.ATTR_CMN_FILEID
                  | osx.ATTR_CMN_PARENTID), 0, 0, 0, 0]
        info = osx.getattrlist(path, attrs, osx.FSOPT_NOFOLLOW)

        if info[0] == osx.VDIR:
            kind = ALIAS_KIND_FOLDER
        else:
            kind = ALIAS_KIND_FILE

        cnid = info[3]
        folder_cnid = info[4]
        
        dirname, filename = os.path.split(path)

        if dirname == '' or dirname == '.':
            dirname = os.getcwd()

        foldername = os.path.basename(dirname)
        
        creation_date = info[1]

        if kind == ALIAS_KIND_FILE:
            creator_code = struct.pack(b'I', info[2].fileInfo.fileCreator)
            type_code = struct.pack(b'I', info[2].fileInfo.fileType)
        else:
            creator_code = b'\0\0\0\0'
            type_code = b'\0\0\0\0'

        a.target = TargetInfo(kind, filename, folder_cnid, cnid, creation_date,
                              creator_code, type_code)
        a.volume = VolumeInfo(vol_name, vol_crtime, b'H+',
                              ALIAS_FIXED_DISK, 0, b'\0\0')

        a.target.folder_name = foldername
        a.volume.posix_path = vol_path

        rel_path = os.path.relpath(path, vol_path)

        # Leave off the initial '/' if vol_path is '/' (no idea why)
        if vol_path == '/':
            a.target.posix_path = rel_path
        else:
            a.target.posix_path = '/' + rel_path

        # Construct the Carbon and CNID paths
        carbon_path = []
        cnid_path = []
        head, tail = os.path.split(rel_path)
        if not tail:
            head, tail = os.path.split(head)
        while head or tail:
            if head:
                attrs = [osx.ATTR_CMN_FILEID, 0, 0, 0, 0]
                info = osx.getattrlist(os.path.join(vol_path, head), attrs, 0)
                cnid_path.append(info[0])
            carbon_tail = tail.replace(':','/')
            carbon_path.insert(0, carbon_tail)
            head, tail = os.path.split(head)
        carbon_path = vol_name + ':' + ':\0'.join(carbon_path)

        a.target.carbon_path = carbon_path
        a.target.cnid_path = cnid_path

        return a
    
    def _to_fd(self, b):
        # We'll come back and fix the length when we're done
        pos = b.tell()
        b.write(struct.pack(b'>4shh', self.appinfo, 0, self.version))

        carbon_volname = self.volume.name.replace(':','/').encode('utf-8')
        carbon_filename = self.target.filename.replace(':','/').encode('utf-8')
        voldate = (self.volume.creation_date - mac_epoch).total_seconds()
        crdate = (self.target.creation_date - mac_epoch).total_seconds()

        # NOTE: crdate should be in local time, but that's system dependent
        #       (so doing so is ridiculous, and nothing could rely on it).
        b.write(struct.pack(b'>h28pI2shI64pII4s4shhI2s10s',
                            self.target.kind,
                            carbon_volname, voldate,
                            self.volume.fs_type,
                            self.volume.disk_type,
                            self.target.folder_cnid,
                            carbon_filename,
                            self.target.cnid,
                            crdate,
                            self.target.creator_code,
                            self.target.type_code,
                            self.target.levels_from,
                            self.target.levels_to,
                            self.volume.attribute_flags,
                            self.volume.fs_id,
                            b'\0'*10))

        # Excuse the odd order; we're copying Finder
        if self.target.folder_name:
            carbon_foldername = self.target.folder_name.replace(':','/')\
              .encode('utf-8')
            b.write(struct.pack(b'>hh', TAG_CARBON_FOLDER_NAME,
                                len(carbon_foldername)))
            b.write(carbon_foldername)
            if len(carbon_foldername) & 1:
                b.write(b'\0')

        b.write(struct.pack(b'>hhQhhQ',
                TAG_HIGH_RES_VOLUME_CREATION_DATE,
                8, long(voldate * 65536),
                TAG_HIGH_RES_CREATION_DATE,
                8, long(crdate * 65536)))

        if self.target.cnid_path:
            cnid_path = struct.pack(b'>%uI' % len(self.target.cnid_path),
                                    *self.target.cnid_path)
            b.write(struct.pack(b'>hh', TAG_CNID_PATH,
                                 len(cnid_path)))
            b.write(cnid_path)

        if self.target.carbon_path:
            carbon_path=self.target.carbon_path.encode('utf-8')
            b.write(struct.pack(b'>hh', TAG_CARBON_PATH,
                                 len(carbon_path)))
            b.write(carbon_path)
            if len(carbon_path) & 1:
                b.write(b'\0')

        if self.volume.appleshare_info:
            ai = self.volume.appleshare_info
            if ai.zone:
                b.write(struct.pack(b'>hh', TAG_APPLESHARE_ZONE,
                                     len(ai.zone)))
                b.write(ai.zone)
                if len(ai.zone) & 1:
                    b.write(b'\0')
            if ai.server:
                b.write(struct.pack(b'>hh', TAG_APPLESHARE_SERVER_NAME,
                                     len(ai.server)))
                b.write(ai.server)
                if len(ai.server) & 1:
                    b.write(b'\0')
            if ai.username:
                b.write(struct.pack(b'>hh', TAG_APPLESHARE_USERNAME,
                                     len(ai.username)))
                b.write(ai.username)
                if len(ai.username) & 1:
                    b.write(b'\0')

        if self.volume.driver_name:
            driver_name = self.volume.driver_name.encode('utf-8')
            b.write(struct.pack(b'>hh', TAG_DRIVER_NAME,
                                len(driver_name)))
            b.write(driver_name)
            if len(driver_name) & 1:
                b.write(b'\0')

        if self.volume.network_mount_info:
            b.write(struct.pack(b'>hh', TAG_NETWORK_MOUNT_INFO,
                                len(self.volume.network_mount_info)))
            b.write(self.volume.network_mount_info)
            if len(self.volume.network_mount_info) & 1:
                b.write(b'\0')

        if self.volume.dialup_info:
            b.write(struct.pack(b'>hh', TAG_DIALUP_INFO,
                                len(self.volume.network_mount_info)))
            b.write(self.volume.network_mount_info)
            if len(self.volume.network_mount_info) & 1:
                b.write(b'\0')

        utf16 = self.target.filename.replace(':','/').encode('utf-16-be')
        b.write(struct.pack(b'>hhh', TAG_UNICODE_FILENAME,
                            len(utf16) + 2,
                            len(utf16) // 2))
        b.write(utf16)

        utf16 = self.volume.name.replace(':','/').encode('utf-16-be')
        b.write(struct.pack(b'>hhh', TAG_UNICODE_VOLUME_NAME,
                            len(utf16) + 2,
                            len(utf16) // 2))
        b.write(utf16)

        if self.target.posix_path:
            posix_path = self.target.posix_path.encode('utf-8')
            b.write(struct.pack(b'>hh', TAG_POSIX_PATH,
                                len(posix_path)))
            b.write(posix_path)
            if len(posix_path) & 1:
                b.write(b'\0')

        if self.volume.posix_path:
            posix_path = self.volume.posix_path.encode('utf-8')
            b.write(struct.pack(b'>hh', TAG_POSIX_PATH_TO_MOUNTPOINT,
                                len(posix_path)))
            b.write(posix_path)
            if len(posix_path) & 1:
                b.write(b'\0')

        if self.volume.disk_image_alias:
            d = self.volume.disk_image_alias.to_bytes()
            b.write(struct.pack(b'>hh', TAG_RECURSIVE_ALIAS_OF_DISK_IMAGE,
                                len(d)))
            b.write(d)
            if len(d) & 1:
                b.write(b'\0')

        if self.target.user_home_prefix_len is not None:
            b.write(struct.pack(b'>hhh', TAG_USER_HOME_LENGTH_PREFIX,
                                2, self.target.user_home_prefix_len))

        for t,v in self.extra:
            b.write(struct.pack(b'>hh', t, len(v)))
            b.write(v)
            if len(v) & 1:
                b.write(b'\0')

        b.write(struct.pack(b'>hh', -1, 0))
        
        blen = b.tell() - pos
        b.seek(pos + 4, os.SEEK_SET)
        b.write(struct.pack(b'>h', blen))

    def to_bytes(self):
        """Returns the binary representation for this :class:`Alias`."""
        with io.BytesIO() as b:
            self._to_fd(b)
            return b.getvalue()

    def __str__(self):
        return '<Alias target=%s>' % self.target.filename

    def __repr__(self):
        values = []
        if self.appinfo != b'\0\0\0\0':
            values.append('appinfo=%r' % self.appinfo)
        if self.version != 2:
            values.append('version=%r' % self.version)
        if self.volume is not None:
            values.append('volume=%r' % self.volume)
        if self.target is not None:
            values.append('target=%r' % self.target)
        if self.extra:
            values.append('extra=%r' % self.extra)
        return 'Alias(%s)' % ','.join(values)
