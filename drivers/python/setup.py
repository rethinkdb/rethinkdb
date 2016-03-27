# Copyright 2010-2015 RethinkDB, all rights reserved.
"""
Please see the following links for more information::

- `Python API documentation <http://rethinkdb.com/api/python/>`_
- `GitHub Project <https://github.com/rethinkdb/rethinkdb>`_
- `Python Driver Source <https://github.com/rethinkdb/rethinkdb/tree/next/drivers/python>`_

"""

import os, setuptools, sys

modulePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'rethinkdb')
sys.path.insert(0, modulePath)
from version import version
sys.path.remove(modulePath)

conditionalPackages = []
if 'upload' in sys.argv: # ensure that uplodas always have everything while uploading to pypi
    conditionalPackages = ['rethinkdb.asyncio_net']
else:
    try: # only add asyncio when it is supported per #4702
        import asyncio
        conditionalPackages = ['rethinkdb.asyncio_net']
    except ImportError: pass

setuptools.setup(
    name="rethinkdb",
    zip_safe=True,
    version=version,
    description="Python driver library for the RethinkDB database server.",
    long_description=__doc__,
    url="http://rethinkdb.com",
    maintainer="RethinkDB Inc.",
    maintainer_email="bugs@rethinkdb.com",
    packages=[
        'rethinkdb',
        'rethinkdb.tornado_net',
        'rethinkdb.twisted_net',
        'rethinkdb.gevent_net',
        'rethinkdb.backports',
        'rethinkdb.backports.ssl_match_hostname'
    ] + conditionalPackages,
    package_dir={'rethinkdb':'rethinkdb'},
    package_data={ 'rethinkdb':['backports/ssl_match_hostname/*.txt'] },
    entry_points={
        'console_scripts':[
            'rethinkdb-import = rethinkdb._import:main',
            'rethinkdb-dump = rethinkdb._dump:main',
            'rethinkdb-export = rethinkdb._export:main',
            'rethinkdb-restore = rethinkdb._restore:main',
            'rethinkdb-index-rebuild = rethinkdb._index_rebuild:main'
        ]
    }
)
