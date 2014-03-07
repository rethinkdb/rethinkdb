.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

``counting_iterator`` adapts an object by adding an ``operator*`` that
returns the current value of the object. All other iterator operations
are forwarded to the adapted object.

