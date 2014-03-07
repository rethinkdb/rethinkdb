.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. Version 1.1 of this ReStructuredText document corresponds to
   n1530_, the paper accepted by the LWG.

.. Copyright David Abrahams, Jeremy Siek, and Thomas Witt 2003. 

Each specialization of the ``iterator_adaptor`` class template is derived from
a specialization of ``iterator_facade``. The core interface functions
expected by ``iterator_facade`` are implemented in terms of the
``iterator_adaptor``\ 's ``Base`` template parameter. A class derived
from ``iterator_adaptor`` typically redefines some of the core
interface functions to adapt the behavior of the ``Base`` type.
Whether the derived class models any of the standard iterator concepts
depends on the operations supported by the ``Base`` type and which
core interface functions of ``iterator_facade`` are redefined in the
``Derived`` class.
