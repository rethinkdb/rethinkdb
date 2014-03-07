
Building the documenation
*************************

Prerequisites
-------------

* Python_ 2.3 or higher

* Docutils_ 0.4 or higher

* Docutils `HTML/frames writer`_ from Docutils Sandbox. Download it as 
  a part of daily `Sandbox snapshot`_ at or get it from the `Docutils' 
  Subversion repository`_.


Building
--------

1. Install prerequisites.

   - To install Docutils, go into its source directory and run
     ``python setup.py install``.

   - To install HTML/frames writer, go to Sandbox's ``agurtovoy/html_frames``
     directory and run ``python setup.py install``.

2. Make sure your Python ``Scripts`` directory  (e.g. ``C:\Python25\Scripts``) 
   is in your search path.

3. Go to ``$BOOST_ROOT/libs/mpl/doc/src/docutils`` directory and do 
   ``python setup.py install`` to install MPL-specific HTML/refdoc Docutils 
   writer.

4. Do ``python build.py``. It's going to take a couple of minutes to finish.

5. If all goes well, the resulting HTML docs will be placed in
   ``$BOOST_ROOT/libs/mpl/doc/src/_build/`` directory.


.. _Python: http://python.org/
.. _Docutils: http://docutils.sourceforge.net/
.. _HTML/frames writer: http://docutils.sourceforge.net/sandbox/agurtovoy/html_frames/
.. _Sandbox snapshot: http://docutils.sourceforge.net/docutils-sandbox-snapshot.tgz
.. _Docutils' Subversion repository: http://docutils.sourceforge.net/docs/dev/repository.html
