#  Copyright (C) 2003 Vladimir Prus
#  Use, modification, and distribution is subject to the Boost Software
#  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
#  at http://www.boost.org/LICENSE_1_0.txt)

class Order:
    """Allows ordering arbitrary objects with regard to arbitrary binary relation.

        The primary use case is the gcc toolset, which is sensitive to
        library order: if library 'a' uses symbols from library 'b',
        then 'a' must be present before 'b' on the linker's command line.
        
        This requirement can be lifted for gcc with GNU ld, but for gcc with
        Solaris LD (and for Solaris toolset as well), the order always matters.
        
        So, we need to store order requirements and then order libraries
        according to them. It it not possible to use dependency graph as
        order requirements. What we need is "use symbols" relationship
        while dependency graph provides "needs to be updated" relationship.
        
        For example::
          lib a : a.cpp b;
          lib b ;
        
        For static linking, the 'a' library need not depend on 'b'. However, it
        still should come before 'b' on the command line.
    """

    def __init__ (self):
        self.constraints_ = []
        
    def add_pair (self, first, second):
        """ Adds the constraint that 'first' should precede 'second'.
        """
        self.constraints_.append ((first, second))

    def order (self, objects):
        """ Given a list of objects, reorder them so that the constains specified
            by 'add_pair' are satisfied.
            
            The algorithm was adopted from an awk script by Nikita Youshchenko
            (yoush at cs dot msu dot su)
        """
        # The algorithm used is the same is standard transitive closure,
        # except that we're not keeping in-degree for all vertices, but
        # rather removing edges.
        result = []

        if not objects: 
            return result

        constraints = self.__eliminate_unused_constraits (objects)
        
        # Find some library that nobody depends upon and add it to
        # the 'result' array.
        obj = None
        while objects:
            new_objects = []
            while objects:
                obj = objects [0]

                if self.__has_no_dependents (obj, constraints):
                    # Emulate break ;
                    new_objects.extend (objects [1:])
                    objects = []

                else:
                    new_objects.append (obj)
                    obj = None
                    objects = objects [1:]
            
            if not obj:
                raise BaseException ("Circular order dependencies")

            # No problem with placing first.
            result.append (obj)

            # Remove all containts where 'obj' comes first,
            # since they are already satisfied.
            constraints = self.__remove_satisfied (constraints, obj)

            # Add the remaining objects for further processing
            # on the next iteration
            objects = new_objects
            
        return result

    def __eliminate_unused_constraits (self, objects):
        """ Eliminate constraints which mention objects not in 'objects'.
            In graph-theory terms, this is finding subgraph induced by
            ordered vertices.
        """
        result = []
        for c in self.constraints_:
            if c [0] in objects and c [1] in objects:
                result.append (c)

        return result
    
    def __has_no_dependents (self, obj, constraints):
        """ Returns true if there's no constraint in 'constraints' where 
            'obj' comes second.
        """
        failed = False
        while constraints and not failed:
            c = constraints [0]

            if c [1] == obj:
                failed = True

            constraints = constraints [1:]

        return not failed
    
    def __remove_satisfied (self, constraints, obj):
        result = []
        for c in constraints:
            if c [0] != obj:
                result.append (c)

        return result
