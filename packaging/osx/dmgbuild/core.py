# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import os
import pkg_resources
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import tokenize

try:
    {}.iteritems
    iteritems = lambda x: x.iteritems()
    iterkeys = lambda x: x.iterkeys()
except AttributeError:
    iteritems = lambda x: x.items()
    iterkeys = lambda x: x.keys()
try:
    unicode
except NameError:
    unicode = str

import biplist
from mac_alias import *
from ds_store import *

from . import colors, badge

_hexcolor_re = re.compile(r'#[0-9a-f]{3}(?:[0-9a-f]{3})?')

class DMGError(Exception):
    pass

def hdiutil(cmd, *args, **kwargs):
    plist = kwargs.get('plist', True)
    all_args = ['/usr/bin/hdiutil', cmd]
    all_args.extend(args)
    if plist:
        all_args.append('-plist')
    p = subprocess.Popen(all_args, stdout=subprocess.PIPE, close_fds=True)
    output, errors = p.communicate()
    if plist:
        results = biplist.readPlistFromString(output)
    else:
        results = output
    retcode = p.wait()
    return retcode, results

# On Python 2 we can just execfile() it, but Python 3 deprecated that
def load_settings(filename, globs, locs):
    if sys.version_info[0] == 2:
        execfile(filename, globs, locs)
    else:
        encoding = 'utf-8'
        with open(filename, 'rb') as fp:
            try:
                encoding = tokenize.detect_encoding(fp.readline)[0]
            except SyntaxError:
                pass
    
        with open(filename, 'r', encoding=encoding) as fp:
            exec(compile(fp.read(), filename, 'exec'), globs, locs)

def build_dmg(filename, volume_name, settings_file=None, defines={}, lookForHiDPI=True):
    settings = {
        # Default settings
        'filename': filename,
        'volume_name': volume_name,
        'format': 'UDBZ',
        'size': '100M',
        'files': [],
        'symlinks': {},
        'icon': None,
        'badge_icon': None,
        'background': 'builtin-arrow',
        'show_status_bar': False,
        'show_tab_view': False,
        'show_toolbar': False,
        'show_pathbar': False,
        'show_sidebar': False,
        'sidebar_width': 180,
        'arrange_by': None,
        'grid_offset': (0, 0),
        'grid_spacing': 100.0,
        'scroll_position': (0.0, 0.0),
        'show_icon_preview': False,
        'show_item_info': False,
        'label_pos': 'bottom',
        'text_size': 16.0,
        'icon_size': 128.0,
        'include_icon_view_settings': 'auto',
        'include_list_view_settings': 'auto',
        'list_icon_size': 16.0,
        'list_text_size': 12.0,
        'list_scroll_position': (0, 0),
        'list_sort_by': 'name',
        'list_use_relative_dates': True,
        'list_calculate_all_sizes': False,
        'list_columns': ('name', 'date-modified', 'size', 'kind', 'date-added'),
        'list_column_widths': {
            'name': 300,
            'date-modified': 181,
            'date-created': 181,
            'date-added': 181,
            'date-last-opened': 181,
            'size': 97,
            'kind': 115,
            'label': 100,
            'version': 75,
            'comments': 300,
            },
        'list_column_sort_directions': {
            'name': 'ascending',
            'date-modified': 'descending',
            'date-created': 'descending',
            'date-added': 'descending',
            'date-last-opened': 'descending',
            'size': 'descending',
            'kind': 'ascending',
            'label': 'ascending',
            'version': 'ascending',
            'comments': 'ascending',
            },
        'window_rect': ((100, 100), (640, 280)),
        'default_view': 'icon-view',
        'icon_locations': {}
        }
    
    # Execute the settings file
    if settings_file:
        load_settings(settings_file, settings, settings)
    
    # Overlay the defines
    settings.update(defines)
    
    # Set up the finder data
    bounds = settings['window_rect']
    
    bwsp = {
        b'ShowStatusBar': settings['show_status_bar'],
        b'WindowBounds': b'{{%s, %s}, {%s, %s}}' % (bounds[0][0],
                                                    bounds[0][1],
                                                    bounds[1][0],
                                                    bounds[1][1]),
        b'ContainerShowSidebar': False,
        b'PreviewPaneVisibility': False,
        b'SidebarWidth': settings['sidebar_width'],
        b'ShowTabView': settings['show_tab_view'],
        b'ShowToolbar': settings['show_toolbar'],
        b'ShowPathbar': settings['show_pathbar'],
        b'ShowSidebar': settings['show_sidebar']
        }

    arrange_options = {
        'name': 'name',
        'date-modified': 'dateModified',
        'date-created': 'dateCreated',
        'date-added': 'dateAdded',
        'date-last-opened': 'dateLastOpened',
        'size': 'size',
        'kind': 'kind',
        'label': 'label',
        }

    icvp = {
        b'viewOptionsVersion': 1,
        b'backgroundType': 0,
        b'backgroundColorRed': 1.0,
        b'backgroundColorGreen': 1.0,
        b'backgroundColorBlue': 1.0,
        b'gridOffsetX': float(settings['grid_offset'][0]),
        b'gridOffsetY': float(settings['grid_offset'][1]),
        b'gridSpacing': float(settings['grid_spacing']),
        b'arrangeBy': str(arrange_options.get(settings['arrange_by'], 'none')),
        b'showIconPreview': settings['show_icon_preview'] == True,
        b'showItemInfo': settings['show_item_info'] == True,
        b'labelOnBottom': settings['label_pos'] == 'bottom',
        b'textSize': float(settings['text_size']),
        b'iconSize': float(settings['icon_size']),
        b'scrollPositionX': float(settings['scroll_position'][0]),
        b'scrollPositionY': float(settings['scroll_position'][1])
        }

    background = settings['background']
    
    columns = {
        'name': 'name',
        'date-modified': 'dateModified',
        'date-created': 'dateCreated',
        'date-added': 'dateAdded',
        'date-last-opened': 'dateLastOpened',
        'size': 'size',
        'kind': 'kind',
        'label': 'label',
        'version': 'version',
        'comments': 'comments'
        }

    default_widths = {
        'name': 300,
        'date-modified': 181,
        'date-created': 181,
        'date-added': 181,
        'date-last-opened': 181,
        'size': 97,
        'kind': 115,
        'label': 100,
        'version': 75,
        'comments': 300,
        }

    default_sort_directions = {
        'name': 'ascending',
        'date-modified': 'descending',
        'date-created': 'descending',
        'date-added': 'descending',
        'date-last-opened': 'descending',
        'size': 'descending',
        'kind': 'ascending',
        'label': 'ascending',
        'version': 'ascending',
        'comments': 'ascending',
        }
        
    lsvp = {
        b'viewOptionsVersion': 1,
        b'sortColumn': columns.get(settings['list_sort_by'], 'name'),
        b'textSize': float(settings['list_text_size']),
        b'iconSize': float(settings['list_icon_size']),
        b'showIconPreview': settings['show_icon_preview'],
        b'scrollPositionX': settings['list_scroll_position'][0],
        b'scrollPositionY': settings['list_scroll_position'][1],
        b'useRelativeDates': settings['list_use_relative_dates'],
        b'calculateAllSizes': settings['list_calculate_all_sizes'],
        }

    lsvp['columns'] = {}
    cndx = {}

    for n, column in enumerate(settings['list_columns']):
        cndx[column] = n
        width = settings['list_column_widths'].get(column,
                                                   default_widths[column])
        asc = 'ascending' == settings['list_column_sort_directions'].get(column,
                    default_sort_directions[column])
        
        lsvp['columns'][columns[column]] = {
            'index': n,
            'width': width,
            'identifier': columns[column],
            'visible': True,
            'ascending': asc
            }

    n = len(settings['list_columns'])
    for k in iterkeys(columns):
        if cndx.get(k, None) is None:
            cndx[k] = n
            width = default_widths[k]
            asc = 'ascending' == default_sort_directions[k]
            
        lsvp['columns'][columns[column]] = {
            'index': n,
            'width': width,
            'identifier': columns[column],
            'visible': False,
            'ascending': asc
            }

        n += 1
    
    default_view = settings['default_view']
    views = {
        'icon-view': b'icnv',
        'column-view': b'clmv',
        'list-view': b'Nlsv',
        'coverflow': b'Flwv'
        }

    icvl = (b'type', views.get(default_view, 'icnv'))

    include_icon_view_settings = default_view == 'icon-view' \
        or settings['include_icon_view_settings'] not in \
        ('auto', 'no', 0, False, None)
    include_list_view_settings = default_view in ('list-view', 'coverflow') \
        or settings['include_list_view_settings'] not in \
        ('auto', 'no', 0, False, None)

    filename = settings['filename']
    volume_name = settings['volume_name']
    
    # Construct a writeable image to start with
    dirname,basename = os.path.split(filename)
    if dirname == '':
        dirname = '.'
    tempname = os.path.join(dirname, 'tmp-' + basename)

    ret, output = hdiutil('create',
                          '-ov',
                          '-volname', volume_name,
                          '-fs', 'HFS+',
                          '-fsargs', '-c c=64,a=16,e=16',
                          '-size', settings['size'],
                          tempname)

    if ret:
        raise DMGError('Unable to create disk image')

    ret, output = hdiutil('attach',
                          '-nobrowse',
                          '-owners', 'off',
                          '-noidme',
                          tempname)

    if ret:
        raise DMGError('Unable to attach disk image')

    try:
        for info in output['system-entities']:
            if info.get('mount-point', None):
                device = info['dev-entry']
                mount_point = info['mount-point']

        icon = settings['icon']
        badge_icon = settings['badge_icon']
        icon_target_path = os.path.join(mount_point, '.VolumeIcon.icns')
        if icon:
            shutil.copyfile(icon, icon_target_path)
        elif badge_icon:
            badge.badge_disk_icon(badge_icon, icon_target_path)

        if icon or badge_icon:
            subprocess.call(['/usr/bin/SetFile', '-a', 'C', mount_point])
        
        if not isinstance(background, (str, unicode)):
            pass
        elif background == 'builtin-arrow':
            tiffdata = pkg_resources.resource_string(
                'dmgbuild', 
                'resources/builtin-arrow.tiff')
            path_in_image = os.path.join(mount_point, '.background.tiff')
            
            with open(path_in_image, 'w') as f:
                f.write(tiffdata)

            alias = Alias.for_file(path_in_image)

            icvp['backgroundType'] = 2
            icvp['backgroundImageAlias'] = biplist.Data(alias.to_bytes())
        elif colors.isAColor(background):
            c = colors.parseColor(background).to_rgb()

            icvp['backgroundType'] = 1
            icvp['backgroundColorRed'] = float(c.r)
            icvp['backgroundColorGreen'] = float(c.g)
            icvp['backgroundColorBlue'] = float(c.b)
        
        elif os.path.isfile(background):
            
            # look to see if there are HiDPI resources available
            
            if lookForHiDPI is True:
                name, extension = os.path.splitext(os.path.basename(background))
                orderedImages = [background]
                imageDirectory = os.path.dirname(background)
                if imageDirectory == '':
                    imageDirectory = '.'
                for candidateName in os.listdir(imageDirectory):
                    hasScale = re.match(
                        '^(?P<name>.+)@(?P<scale>\d+)x(?P<extension>\.\w+)$',
                        candidateName)
                    if hasScale and name == hasScale.group('name') and \
                        extension == hasScale.group('extension'):
                            scale = int(hasScale.group('scale'))
                            if len(orderedImages) < scale:
                                orderedImages += [None] * (scale - len(orderedImages))
                            orderedImages[scale - 1] = os.path.join(imageDirectory, candidateName)
                
                if len(orderedImages) > 1:
                    # compile the grouped tiff
                    backgroundFile = tempfile.NamedTemporaryFile(suffix='.tiff')
                    background = backgroundFile.name
                    output = tempfile.TemporaryFile(mode='w+')
                    try:
                        subprocess.check_call(
                            ['/usr/bin/tiffutil', '-cathidpicheck'] +
                            filter(None, orderedImages) +
                            ['-out', background], stdout=output, stderr=output)
                    except Exception as e:
                        output.seek(0)
                        raise ValueError(
                            'unable to compile combined HiDPI file "%s" got error: %s\noutput: %s'
                            % (background, str(e), output.read()))
            
            _, kind = os.path.splitext(background)
            path_in_image = os.path.join(mount_point, '.background' + kind)
            shutil.copyfile(background, path_in_image)

            alias = Alias.for_file(path_in_image)
        
            icvp['backgroundType'] = 2
            icvp['backgroundImageAlias'] = biplist.Data(alias.to_bytes())
        else:
            raise ValueError('background file "%s" not found' % background)

        for f in settings['files']:
            basename = os.path.basename(f)
            f_in_image = os.path.join(mount_point, basename)
            if stat.S_ISDIR(os.stat(f).st_mode):
                shutil.copytree(f, f_in_image, symlinks=True)
            else:
                shutil.copyfile(f, f_in_image)
                shutil.copymode(f, f_in_image)

        for name,target in iteritems(settings['symlinks']):
            name_in_image = os.path.join(mount_point, name)
            os.symlink(target, name_in_image)
    
        userfn = settings.get('create_hook', None)
        if callable(userfn):
            userfn(mount_point, settings)
        
        image_dsstore = os.path.join(mount_point, '.DS_Store')

        with DSStore.open(image_dsstore, 'w+') as d:
            d['.']['vSrn'] = ('long', 1)
            d['.']['bwsp'] = bwsp
            if include_icon_view_settings:
                d['.']['icvp'] = icvp
            if include_list_view_settings:
                d['.']['lsvp'] = lsvp
            d['.']['icvl'] = icvl

            for k,v in iteritems(settings['icon_locations']):
                d[k]['Iloc'] = v
    except:
        # Always try to detach
        hdiutil('detach', '-force', device, plist=False)
        raise
    
    ret, output = hdiutil('detach', device, plist=False)

    if ret:
        hdiutil('detach', '-force', device, plist=False)
        raise DMGError('Unable to detach device cleanly')
    
    ret, output = hdiutil('convert', tempname,
                          '-format', settings['format'],
                          '-ov',
                          '-o', filename)

    if ret:
        raise DMGError('Unable to convert')
    
    os.remove(tempname)
    
