<?php

function echo_unordered_docs(
    $map,
    $equivalent_keys)
{
    $name = 'unordered_'.
        ($equivalent_keys ? 'multi' : '').
        ($map ? 'map' : 'set');

    if ($map)
    {
        $template_value = <<<EOL
            <template-type-parameter name="Key">
            </template-type-parameter>
            <template-type-parameter name="Mapped">
            </template-type-parameter>

EOL;

        $key_type = 'Key';
        $key_name = 'key';
        $value_type = 'std::pair&lt;Key const, Mapped&gt;';
        $full_type = $name.'&lt;Key, Mapped, Hash, Pred, Alloc&gt;';
    }
    else
    {
        $template_value = <<<EOL
            <template-type-parameter name="Value">
            </template-type-parameter>

EOL;

        $key_type = 'Value';
        $key_name = 'value';
        $value_type = 'Value';
        $full_type = $name.'&lt;Value, Hash, Pred, Alloc&gt;';
    }
?>
        <class name="<?php echo $name ?>">
          <template>
<?php echo $template_value; ?>
            <template-type-parameter name="Hash">
              <default><type>boost::hash&lt;<?php echo $key_type; ?>&gt;</type></default>
            </template-type-parameter>
            <template-type-parameter name="Pred">
              <default><type>std::equal_to&lt;<?php echo $key_type; ?>&gt;</type></default>
            </template-type-parameter>
            <template-type-parameter name="Alloc">
              <default><type>std::allocator&lt;<?php echo $value_type; ?>&gt;</type></default>
            </template-type-parameter>
          </template>
          <purpose><simpara>
            An unordered associative container that <?php
            echo $map ? 'associates ' : 'stores ';
            echo $equivalent_keys ? '' : 'unique ';
            echo $map ? 'keys with another value.' : 'values.';
            echo $equivalent_keys ? ' The same key can be stored multiple times.' : '';
          ?>

          </simpara></purpose>
          <description>
            <para><emphasis role="bold">Template Parameters</emphasis>
              <informaltable>
                <tgroup cols="2">
                  <tbody>
<?php if ($map): ?>
                    <row>
                      <entry><emphasis>Key</emphasis></entry>
                      <entry><code>Key</code> must be <code>Erasable</code> from the container
                        (i.e. <code>allocator_traits</code> can <code>destroy</code> it).
                      </entry></row>
                    <row>
                      <entry><emphasis>Mapped</emphasis></entry>
                      <entry><code>Mapped</code> must be <code>Erasable</code> from the container
                        (i.e. <code>allocator_traits</code> can <code>destroy</code> it).
                      </entry></row>
<?php else: ?>
                    <row>
                      <entry><emphasis>Value</emphasis></entry>
                      <entry><code>Value</code> must be <code>Erasable</code> from the container
                        (i.e. <code>allocator_traits</code> can <code>destroy</code> it).
                      </entry></row>
<?php endif ?>
                    <row>
                      <entry><emphasis>Hash</emphasis></entry>
                      <entry>A unary function object type that acts a hash function for a <code><?php echo $key_type; ?></code>. It takes a single argument of type <code><?php echo $key_type; ?></code> and returns a value of type std::size_t.</entry></row>
                    <row>
                      <entry><emphasis>Pred</emphasis></entry>
                      <entry>A binary function object that implements an equivalence relation on values of type <code><?php echo $key_type; ?></code>.
                        A binary function object that induces an equivalence relation on values of type <code><?php echo $key_type; ?></code>.
                        It takes two arguments of type <code><?php echo $key_type; ?></code> and returns a value of type bool.</entry></row>
                    <row>
                      <entry><emphasis>Alloc</emphasis></entry>
                      <entry>An allocator whose value type is the same as the container's value type.</entry></row></tbody></tgroup></informaltable></para>
            <para>The elements are organized into buckets. <?php
            echo $equivalent_keys ?
                'Keys with the same hash code are stored in the same bucket and elements with equivalent keys are stored next to each other.' :
                'Keys with the same hash code are stored in the same bucket.';
            ?></para>
            <para>The number of buckets can be automatically increased by a call to insert, or as the result of calling rehash.</para>
          </description>
          <typedef name="key_type">
            <type><?php echo $key_type; ?></type>
          </typedef>
          <typedef name="value_type">
            <type><?php echo $value_type; ?></type>
          </typedef>
<?php if ($map): ?>
          <typedef name="mapped_type">
            <type>Mapped</type>
          </typedef>
<?php endif; ?>
          <typedef name="hasher">
            <type>Hash</type>
          </typedef>
          <typedef name="key_equal">
            <type>Pred</type>
          </typedef>
          <typedef name="allocator_type">
            <type>Alloc</type>
          </typedef>
          <typedef name="pointer">
            <type>typename allocator_type::pointer</type>
            <description>
              <para>
                <code>value_type*</code> if
                <code>allocator_type::pointer</code> is not defined.
              </para>
            </description>
          </typedef>
          <typedef name="const_pointer">
            <type>typename allocator_type::const_pointer</type>
            <description>
              <para>
                <code>boost::pointer_to_other&lt;pointer, value_type&gt;::type</code>
                if <code>allocator_type::const_pointer</code> is not defined.
              </para>
            </description>
          </typedef>
          <typedef name="reference">
            <type>value_type&amp;</type>
            <purpose><simpara>lvalue of <type>value_type</type>.</simpara></purpose>
          </typedef>
          <typedef name="const_reference">
            <type>value_type const&amp;</type>
            <purpose><simpara>const lvalue of <type>value_type</type>.</simpara></purpose>
          </typedef>
          <typedef name="size_type">
            <type><emphasis>implementation-defined</emphasis></type>
            <description>
              <para>An unsigned integral type.</para>
              <para><type>size_type</type> can represent any non-negative value of <type>difference_type</type>.</para>
            </description>
          </typedef>
          <typedef name="difference_type">
            <type><emphasis>implementation-defined</emphasis></type>
            <description>
              <para>A signed integral type.</para>
              <para>Is identical to the difference type of <type>iterator</type> and <type>const_iterator</type>.</para>
            </description>
          </typedef>
          <typedef name="iterator">
            <type><emphasis>implementation-defined</emphasis></type>
            <description>
              <para><?php echo $map ? 'An' : 'A constant' ?> iterator whose value type is <type>value_type</type>. </para>
              <para>The iterator category is at least a forward iterator.</para>
              <para>Convertible to <type>const_iterator</type>.</para>
            </description>
          </typedef>
          <typedef name="const_iterator">
            <type><emphasis>implementation-defined</emphasis></type>
            <description>
              <para>A constant iterator whose value type is <type>value_type</type>. </para>
              <para>The iterator category is at least a forward iterator.</para>
            </description>
          </typedef>
          <typedef name="local_iterator">
            <type><emphasis>implementation-defined</emphasis></type>
            <description>
              <para>An iterator with the same value type, difference type and pointer and reference type as <type>iterator</type>.</para>
              <para>A local_iterator object can be used to iterate through a single bucket.</para>
            </description>
          </typedef>
          <typedef name="const_local_iterator">
            <type><emphasis>implementation-defined</emphasis></type>
            <description>
              <para>A constant iterator with the same value type, difference type and pointer and reference type as <type>const_iterator</type>.</para>
              <para>A const_local_iterator object can be used to iterate through a single bucket.</para>
            </description>
          </typedef>
          <constructor specifiers="explicit">
            <parameter name="n">
              <paramtype>size_type</paramtype>
              <default><emphasis>implementation-defined</emphasis></default>
            </parameter>
            <parameter name="hf">
              <paramtype>hasher const&amp;</paramtype>
              <default>hasher()</default>
            </parameter>
            <parameter name="eq">
              <paramtype>key_equal const&amp;</paramtype>
              <default>key_equal()</default>
            </parameter>
            <parameter name="a">
              <paramtype>allocator_type const&amp;</paramtype>
              <default>allocator_type()</default>
            </parameter>
            <postconditions>
              <code><methodname>size</methodname>() == 0</code>
            </postconditions>
            <description>
              <para>Constructs an empty container with at least n buckets, using hf as the hash function, eq as the key equality predicate, a as the allocator and a maximum load factor of 1.0.</para>
            </description>
            <requires>
              <para>If the defaults are used, <code>hasher</code>, <code>key_equal</code> and
                <code>allocator_type</code> need to be <code>DefaultConstructible</code>.
              </para>
            </requires>
          </constructor>
          <constructor>
            <template>
              <template-type-parameter name="InputIterator">
              </template-type-parameter>
            </template>
            <parameter name="f">
              <paramtype>InputIterator</paramtype>
            </parameter>
            <parameter name="l">
              <paramtype>InputIterator</paramtype>
            </parameter>
            <parameter name="n">
              <paramtype>size_type</paramtype>
              <default><emphasis>implementation-defined</emphasis></default>
            </parameter>
            <parameter name="hf">
              <paramtype>hasher const&amp;</paramtype>
              <default>hasher()</default>
            </parameter>
            <parameter name="eq">
              <paramtype>key_equal const&amp;</paramtype>
              <default>key_equal()</default>
            </parameter>
            <parameter name="a">
              <paramtype>allocator_type const&amp;</paramtype>
              <default>allocator_type()</default>
            </parameter>
            <description>
              <para>Constructs an empty container with at least n buckets, using hf as the hash function, eq as the key equality predicate, a as the allocator and a maximum load factor of 1.0 and inserts the elements from [f, l) into it.</para>
            </description>
            <requires>
              <para>If the defaults are used, <code>hasher</code>, <code>key_equal</code> and
                <code>allocator_type</code> need to be <code>DefaultConstructible</code>.
              </para>
            </requires>
          </constructor>
          <constructor>
            <parameter>
              <paramtype><?php echo $name; ?> const&amp;</paramtype>
            </parameter>
            <description>
              <para>The copy constructor. Copies the contained elements, hash function, predicate, maximum load factor and allocator.</para>
              <para>If <code>Allocator::select_on_container_copy_construction</code>
              exists and has the right signature, the allocator will be
              constructed from its result.</para>
            </description>
            <requires>
              <para><code>value_type</code> is copy constructible</para>
            </requires>
          </constructor>
          <constructor>
            <parameter>
              <paramtype><?php echo $name; ?> &amp;&amp;</paramtype>
            </parameter>
            <description>
              <para>The move constructor.</para>
            </description>
            <notes>
              <para>This is implemented using Boost.Move.</para>
            </notes>
            <requires>
              <para>
                <code>value_type</code> is move constructible.
              </para>
              <para>
                On compilers without rvalue reference support the
                emulation does not support moving without calling
                <code>boost::move</code> if <code>value_type</code> is
                not copyable. So, for example, you can't return the
                container from a function.
              </para>
            </requires>
          </constructor>
          <constructor specifiers="explicit">
            <parameter name="a">
              <paramtype>Allocator const&amp;</paramtype>
            </parameter>
            <description>
                <para>Constructs an empty container, using allocator <code>a</code>.</para>
            </description>
          </constructor>
          <constructor>
            <parameter name="x">
              <paramtype><?php echo $name; ?> const&amp;</paramtype>
            </parameter>
            <parameter name="a">
              <paramtype>Allocator const&amp;</paramtype>
            </parameter>
            <description>
                <para>Constructs an container, copying <code>x</code>'s contained elements, hash function, predicate, maximum load factor, but using allocator <code>a</code>.</para>
            </description>
          </constructor>
          <destructor>
            <notes>
              <para>The destructor is applied to every element, and all memory is deallocated</para>
            </notes>
          </destructor>
          <method name="operator=">
            <parameter>
              <paramtype><?php echo $name; ?> const&amp;</paramtype>
            </parameter>
            <type><?php echo $name; ?>&amp;</type>
            <description>
              <para>The assignment operator. Copies the contained elements, hash function, predicate and maximum load factor but not the allocator.</para>
              <para>If <code>Alloc::propagate_on_container_copy_assignment</code>
              exists and <code>Alloc::propagate_on_container_copy_assignment::value
              </code> is true, the allocator is overwritten, if not the
              copied elements are created using the existing
              allocator.</para>
            </description>
            <requires>
              <para><code>value_type</code> is copy constructible</para>
            </requires>
          </method>
          <method name="operator=">
            <parameter>
              <paramtype><?php echo $name; ?> &amp;&amp;</paramtype>
            </parameter>
            <type><?php echo $name; ?>&amp;</type>
            <description>
              <para>The move assignment operator.</para>
              <para>If <code>Alloc::propagate_on_container_move_assignment</code>
              exists and <code>Alloc::propagate_on_container_move_assignment::value
              </code> is true, the allocator is overwritten, if not the
              moved elements are created using the existing
              allocator.</para>
            </description>
            <notes>
              <para>
                On compilers without rvalue references, this is emulated using
                Boost.Move. Note that on some compilers the copy assignment
                operator may be used in some circumstances.
              </para>
            </notes>
            <requires>
              <para>
                <code>value_type</code> is move constructible.
              </para>
            </requires>
          </method>
          <method name="get_allocator" cv="const">
            <type>allocator_type</type>
          </method>
          <method-group name="size and capacity">
            <method name="empty" cv="const">
              <type>bool</type>
              <returns>
                <code><methodname>size</methodname>() == 0</code>
              </returns>
            </method>
            <method name="size" cv="const">
              <type>size_type</type>
              <returns>
                <code>std::distance(<methodname>begin</methodname>(), <methodname>end</methodname>())</code>
              </returns>
            </method>
            <method name="max_size" cv="const">
              <type>size_type</type>
              <returns><code><methodname>size</methodname>()</code> of the largest possible container.
              </returns>
            </method>
          </method-group>
          <method-group name="iterators">
            <overloaded-method name="begin">
              <signature><type>iterator</type></signature>
              <signature cv="const"><type>const_iterator</type></signature>
              <returns>An iterator referring to the first element of the container, or if the container is empty the past-the-end value for the container.
              </returns>
            </overloaded-method>
            <overloaded-method name="end">
              <signature>
                <type>iterator</type>
              </signature>
              <signature cv="const">
                <type>const_iterator</type>
              </signature>
              <returns>An iterator which refers to the past-the-end value for the container.
              </returns>
            </overloaded-method>
            <method name="cbegin" cv="const">
              <type>const_iterator</type>
              <returns>A constant iterator referring to the first element of the container, or if the container is empty the past-the-end value for the container.
              </returns>
            </method>
            <method name="cend" cv="const">
              <type>const_iterator</type>
              <returns>A constant iterator which refers to the past-the-end value for the container.
              </returns>
            </method>
          </method-group>
          <method-group name="modifiers">
            <method name="emplace">
              <template>
                <template-type-parameter name="Args" pack="1">
                </template-type-parameter>
              </template>
              <parameter name="args" pack="1">
                <paramtype>Args&amp;&amp;</paramtype>
              </parameter>
              <type><?php echo $equivalent_keys ? 'iterator' : 'std::pair&lt;iterator, bool&gt;' ?></type>
              <description>
                <para>Inserts an object, constructed with the arguments <code>args</code>, in the container<?php
                echo $equivalent_keys ? '.' :
                    ' if and only if there is no element in the container with an equivalent '.$key_name. '.';
                ?></para>
              </description>
              <requires>
                <para><code>value_type</code> is <code>EmplaceConstructible</code> into
                  <code>X</code> from <code>args</code>.
                </para>
              </requires>
              <returns>
<?php if ($equivalent_keys): ?>
                <para>An iterator pointing to the inserted element.</para>
<?php else: ?>
                <para>The bool component of the return type is true if an insert took place.</para>
                <para>If an insert took place, then the iterator points to the newly inserted element. Otherwise, it points to the element with equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
              </returns>
              <throws>
                <para>If an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
                <para>If the compiler doesn't support variadic template arguments or rvalue
                      references, this is emulated for up to 10 arguments, with no support
                      for rvalue references or move semantics.</para>
                <para>Since existing <code>std::pair</code> implementations don't support
                      <code>std::piecewise_construct</code> this emulates it,
                      but using <code>boost::unordered::piecewise_construct</code>.</para>
              </notes>
            </method>
            <method name="emplace_hint">
              <template>
                <template-type-parameter name="Args" pack="1">
                </template-type-parameter>
              </template>
              <parameter name="hint">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <parameter name="args" pack="1">
                <paramtype>Args&amp;&amp;</paramtype>
              </parameter>
              <type>iterator</type>
              <description>
                <para>Inserts an object, constructed with the arguments <code>args</code>, in the container<?php
                echo $equivalent_keys ? '.' :
                    ' if and only if there is no element in the container with an equivalent '.$key_name. '.';
                ?></para>
                <para><code>hint</code> is a suggestion to where the element should be inserted.</para>
              </description>
              <requires>
                <para><code>value_type</code> is <code>EmplaceConstructible</code> into
                  <code>X</code> from <code>args</code>.
                </para>
              </requires>
              <returns>
<?php if ($equivalent_keys): ?>
                <para>An iterator pointing to the inserted element.</para>
<?php else: ?>
                <para>If an insert took place, then the iterator points to the newly inserted element. Otherwise, it points to the element with equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
              </returns>
              <throws>
                <para>If an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>The standard is fairly vague on the meaning of the hint. But the only practical way to use it, and the only way that Boost.Unordered supports is to point to an existing element with the same <?php echo $key_name; ?>. </para>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
                <para>If the compiler doesn't support variadic template arguments or rvalue
                      references, this is emulated for up to 10 arguments, with no support
                      for rvalue references or move semantics.</para>
                <para>Since existing <code>std::pair</code> implementations don't support
                      <code>std::piecewise_construct</code> this emulates it,
                      but using <code>boost::unordered::piecewise_construct</code>.</para>
              </notes>
            </method>
            <method name="insert">
              <parameter name="obj">
                <paramtype>value_type const&amp;</paramtype>
              </parameter>
              <type><?php echo $equivalent_keys ? 'iterator' : 'std::pair&lt;iterator, bool&gt;' ?></type>
              <description>
                <para>Inserts <code>obj</code> in the container<?php
                echo $equivalent_keys ? '.' :
                    ' if and only if there is no element in the container with an equivalent '.$key_name. '.';
                ?></para>
              </description>
              <requires>
                <para><code>value_type</code> is <code>CopyInsertable</code>.</para>
              </requires>
              <returns>
<?php if ($equivalent_keys): ?>
                <para>An iterator pointing to the inserted element.</para>
<?php else: ?>
                <para>The bool component of the return type is true if an insert took place.</para>
                <para>If an insert took place, then the iterator points to the newly inserted element. Otherwise, it points to the element with equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
              </returns>
              <throws>
                <para>If an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
              </notes>
            </method>
            <method name="insert">
              <parameter name="obj">
                <paramtype>value_type&amp;&amp;</paramtype>
              </parameter>
              <type><?php echo $equivalent_keys ? 'iterator' : 'std::pair&lt;iterator, bool&gt;' ?></type>
              <description>
                <para>Inserts <code>obj</code> in the container<?php
                echo $equivalent_keys ? '.' :
                    ' if and only if there is no element in the container with an equivalent '.$key_name. '.';
                ?></para>
              </description>
              <requires>
                <para><code>value_type</code> is <code>MoveInsertable</code>.</para>
              </requires>
              <returns>
<?php if ($equivalent_keys): ?>
                <para>An iterator pointing to the inserted element.</para>
<?php else: ?>
                <para>The bool component of the return type is true if an insert took place.</para>
                <para>If an insert took place, then the iterator points to the newly inserted element. Otherwise, it points to the element with equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
              </returns>
              <throws>
                <para>If an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
              </notes>
            </method>
            <method name="insert">
              <parameter name="hint">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <parameter name="obj">
                <paramtype>value_type const&amp;</paramtype>
              </parameter>
              <type>iterator</type>
              <description>
<?php if ($equivalent_keys): ?>
                <para>Inserts <code>obj</code> in the container.</para>
<?php else: ?>
                <para>Inserts <code>obj</code> in the container if and only if there is no element in the container with an equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
                <para>hint is a suggestion to where the element should be inserted.</para>
              </description>
              <requires>
                <para><code>value_type</code> is <code>CopyInsertable</code>.</para>
              </requires>
              <returns>
<?php if ($equivalent_keys): ?>
                <para>An iterator pointing to the inserted element.</para>
<?php else: ?>
                <para>If an insert took place, then the iterator points to the newly inserted element. Otherwise, it points to the element with equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
              </returns>
              <throws>
                <para>If an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>The standard is fairly vague on the meaning of the hint. But the only practical way to use it, and the only way that Boost.Unordered supports is to point to an existing element with the same <?php echo $key_name; ?>. </para>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
              </notes>
            </method>
            <method name="insert">
              <parameter name="hint">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <parameter name="obj">
                <paramtype>value_type&amp;&amp;</paramtype>
              </parameter>
              <type>iterator</type>
              <description>
<?php if ($equivalent_keys): ?>
                <para>Inserts <code>obj</code> in the container.</para>
<?php else: ?>
                <para>Inserts <code>obj</code> in the container if and only if there is no element in the container with an equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
                <para>hint is a suggestion to where the element should be inserted.</para>
              </description>
              <requires>
                <para><code>value_type</code> is <code>MoveInsertable</code>.</para>
              </requires>
              <returns>
<?php if ($equivalent_keys): ?>
                <para>An iterator pointing to the inserted element.</para>
<?php else: ?>
                <para>If an insert took place, then the iterator points to the newly inserted element. Otherwise, it points to the element with equivalent <?php echo $key_name; ?>.</para>
<?php endif; ?>
              </returns>
              <throws>
                <para>If an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>The standard is fairly vague on the meaning of the hint. But the only practical way to use it, and the only way that Boost.Unordered supports is to point to an existing element with the same <?php echo $key_name; ?>. </para>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
              </notes>
            </method>
            <method name="insert">
              <template>
                <template-type-parameter name="InputIterator">
                </template-type-parameter>
              </template>
              <parameter name="first">
                <paramtype>InputIterator</paramtype>
              </parameter>
              <parameter name="last">
                <paramtype>InputIterator</paramtype>
              </parameter>
              <type>void</type>
              <description>
                <para>Inserts a range of elements into the container. Elements are inserted if and only if there is no element in the container with an equivalent <?php echo $key_name; ?>.</para>
              </description>
              <requires>
                <para><code>value_type</code> is <code>EmplaceConstructible</code> into
                  <code>X</code> from <code>*first</code>.</para>
              </requires>
              <throws>
                <para>When inserting a single element, if an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
              </notes>
            </method>
            <method name="erase">
              <parameter name="position">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <type>iterator</type>
              <description>
                <para>Erase the element pointed to by <code>position</code>.</para>
              </description>
              <returns>
                <para>The iterator following <code>position</code> before the erasure.</para>
              </returns>
              <throws>
                <para>Only throws an exception if it is thrown by <code>hasher</code> or <code>key_equal</code>.</para>
              </throws>
              <notes>
                <para>
                  In older versions this could be inefficient because it had to search
                  through several buckets to find the position of the returned iterator.
                  The data structure has been changed so that this is no longer the case,
                  and the alternative erase methods have been deprecated.
                </para>
              </notes>
            </method>
            <method name="erase">
              <parameter name="k">
                <paramtype>key_type const&amp;</paramtype>
              </parameter>
              <type>size_type</type>
              <description>
                <para>Erase all elements with key equivalent to <code>k</code>.</para>
              </description>
              <returns>
                <para>The number of elements erased.</para>
              </returns>
              <throws>
                <para>Only throws an exception if it is thrown by <code>hasher</code> or <code>key_equal</code>.</para>
              </throws>
            </method>
            <method name="erase">
              <parameter name="first">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <parameter name="last">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <type>iterator</type>
              <description>
                <para>Erases the elements in the range from <code>first</code> to <code>last</code>.</para>
              </description>
              <returns>
                <para>The iterator following the erased elements - i.e. <code>last</code>.</para>
              </returns>
              <throws>
                <para>Only throws an exception if it is thrown by <code>hasher</code> or <code>key_equal</code>.</para>
                <para>In this implementation, this overload doesn't call either function object's methods so it is no throw, but this might not be true in other implementations.</para>
              </throws>
            </method>
            <method name="quick_erase">
              <parameter name="position">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <type>void</type>
              <description>
                <para>Erase the element pointed to by <code>position</code>.</para>
              </description>
              <throws>
                <para>Only throws an exception if it is thrown by <code>hasher</code> or <code>key_equal</code>.</para>
                <para>In this implementation, this overload doesn't call either function object's methods so it is no throw, but this might not be true in other implementations.</para>
              </throws>
              <notes>
                <para>
                  This method was implemented because returning an iterator to
                  the next element from <code>erase</code> was expensive, but
                  the container has been redesigned so that is no longer the
                  case. So this method is now deprecated.
                </para>
              </notes>
            </method>
            <method name="erase_return_void">
              <parameter name="position">
                <paramtype>const_iterator</paramtype>
              </parameter>
              <type>void</type>
              <description>
                <para>Erase the element pointed to by <code>position</code>.</para>
              </description>
              <throws>
                <para>Only throws an exception if it is thrown by <code>hasher</code> or <code>key_equal</code>.</para>
                <para>In this implementation, this overload doesn't call either function object's methods so it is no throw, but this might not be true in other implementations.</para>
              </throws>
              <notes>
                <para>
                  This method was implemented because returning an iterator to
                  the next element from <code>erase</code> was expensive, but
                  the container has been redesigned so that is no longer the
                  case. So this method is now deprecated.
                </para>
              </notes>
            </method>
            <method name="clear">
              <type>void</type>
              <description>
                <para>Erases all elements in the container.</para>
              </description>
              <postconditions>
                <para><code><methodname>size</methodname>() == 0</code></para>
              </postconditions>
              <throws>
                <para>Never throws an exception.</para>
              </throws>
            </method>
            <method name="swap">
              <parameter>
                <paramtype><?php echo $name; ?>&amp;</paramtype>
              </parameter>
              <type>void</type>
              <description>
                <para>Swaps the contents of the container with the parameter.</para>
                <para>If <code>Allocator::propagate_on_container_swap</code> is declared and
                  <code>Allocator::propagate_on_container_swap::value</code> is true then the
                  containers' allocators are swapped. Otherwise, swapping with unequal allocators
                  results in undefined behavior.</para>
              </description>
              <throws>
                <para>Doesn't throw an exception unless it is thrown by the copy constructor or copy assignment operator of <code>key_equal</code> or <code>hasher</code>.</para>
              </throws>
              <notes>
                <para>The exception specifications aren't quite the same as the C++11 standard, as
                  the equality predieate and hash function are swapped using their copy constructors.</para>
              </notes>
            </method>
          </method-group>
          <method-group name="observers">
            <method name="hash_function" cv="const">
              <type>hasher</type>
              <returns>The container's hash function.
              </returns>
            </method>
            <method name="key_eq" cv="const">
              <type>key_equal</type>
              <returns>The container's key equality predicate.
              </returns>
            </method>
          </method-group>
          <method-group name="lookup">
            <overloaded-method name="find">
              <signature>
                <parameter name="k">
                  <paramtype>key_type const&amp;</paramtype>
                </parameter>
                <type>iterator</type>
              </signature>
              <signature cv="const">
                <parameter name="k">
                  <paramtype>key_type const&amp;</paramtype>
                </parameter>
                <type>const_iterator</type>
              </signature>
              <signature>
                <template>
                  <template-type-parameter name="CompatibleKey"/>
                  <template-type-parameter name="CompatibleHash"/>
                  <template-type-parameter name="CompatiblePredicate"/>
                </template>
                <parameter name="k">
                  <paramtype>CompatibleKey const&amp;</paramtype>
                </parameter>
                <parameter name="hash">
                  <paramtype>CompatibleHash const&amp;</paramtype>
                </parameter>
                <parameter name="eq">
                  <paramtype>CompatiblePredicate const&amp;</paramtype>
                </parameter>
                <type>iterator</type>
              </signature>
              <signature cv="const">
                <template>
                  <template-type-parameter name="CompatibleKey"/>
                  <template-type-parameter name="CompatibleHash"/>
                  <template-type-parameter name="CompatiblePredicate"/>
                </template>
                <parameter name="k">
                  <paramtype>CompatibleKey const&amp;</paramtype>
                </parameter>
                <parameter name="hash">
                  <paramtype>CompatibleHash const&amp;</paramtype>
                </parameter>
                <parameter name="eq">
                  <paramtype>CompatiblePredicate const&amp;</paramtype>
                </parameter>
                <type>const_iterator</type>
              </signature>
              <returns>
                <para>An iterator pointing to an element with key equivalent to <code>k</code>, or <code>b.end()</code> if no such element exists.</para>
              </returns>
              <notes><para>
                The templated overloads are a non-standard extensions which
                allows you to use a compatible hash function and equality
                predicate for a key of a different type in order to avoid
                an expensive type cast. In general, its use is not encouraged.
              </para></notes>
            </overloaded-method>
            <method name="count" cv="const">
              <parameter name="k">
                <paramtype>key_type const&amp;</paramtype>
              </parameter>
              <type>size_type</type>
              <returns>
                <para>The number of elements with key equivalent to <code>k</code>.</para>
              </returns>
            </method>
            <overloaded-method name="equal_range">
              <signature>
                <parameter name="k">
                  <paramtype>key_type const&amp;</paramtype>
                </parameter>
                <type>std::pair&lt;iterator, iterator&gt;</type>
              </signature>
              <signature cv="const">
                <parameter name="k">
                  <paramtype>key_type const&amp;</paramtype>
                </parameter>
                <type>std::pair&lt;const_iterator, const_iterator&gt;</type>
              </signature>
              <returns>
                <para>A range containing all elements with key equivalent to <code>k</code>.
                  If the container doesn't container any such elements, returns
                  <code><functionname>std::make_pair</functionname>(<methodname>b.end</methodname>(),<methodname>b.end</methodname>())</code>.
                  </para>
              </returns>
            </overloaded-method>
<?php if ($map && !$equivalent_keys): ?>
            <method name="operator[]">
              <parameter name="k">
                <paramtype>key_type const&amp;</paramtype>
              </parameter>
              <type>mapped_type&amp;</type>
              <effects>
                <para>If the container does not already contain an elements with a key equivalent to <code>k</code>, inserts the value <code>std::pair&lt;key_type const, mapped_type&gt;(k, mapped_type())</code></para>
              </effects>
              <returns>
                <para>A reference to <code>x.second</code> where x is the element already in the container, or the newly inserted element with a key equivalent to <code>k</code></para>
              </returns>
              <throws>
                <para>If an exception is thrown by an operation other than a call to <code>hasher</code> the function has no effect.</para>
              </throws>
              <notes>
                <para>Can invalidate iterators, but only if the insert causes the load factor to be greater to or equal to the maximum load factor.</para>
                <para>Pointers and references to elements are never invalidated.</para>
              </notes>
            </method>
            <overloaded-method name="at">
              <signature><type>Mapped&amp;</type>
                <parameter name="k"><paramtype>key_type const&amp;</paramtype></parameter></signature>
              <signature cv="const"><type>Mapped const&amp;</type>
                <parameter name="k"><paramtype>key_type const&amp;</paramtype></parameter></signature>
              <returns>
                <para>A reference to <code>x.second</code> where <code>x</code> is the (unique) element whose key is equivalent to <code>k</code>.</para>
              </returns>
              <throws>
                <para>An exception object of type <code>std::out_of_range</code> if no such element is present.</para>
              </throws>
            </overloaded-method>
<?php endif; ?>
          </method-group>
          <method-group name="bucket interface">
            <method name="bucket_count" cv="const">
              <type>size_type</type>
              <returns>
                <para>The number of buckets.</para>
              </returns>
            </method>
            <method name="max_bucket_count" cv="const">
              <type>size_type</type>
              <returns>
                <para>An upper bound on the number of buckets.</para>
              </returns>
            </method>
            <method name="bucket_size" cv="const">
              <parameter name="n">
                <paramtype>size_type</paramtype>
              </parameter>
              <type>size_type</type>
              <requires>
                <para><code>n &lt; <methodname>bucket_count</methodname>()</code></para>
              </requires>
              <returns>
                <para>The number of elements in bucket <code>n</code>.</para>
              </returns>
            </method>
            <method name="bucket" cv="const">
              <parameter name="k">
                <paramtype>key_type const&amp;</paramtype>
              </parameter>
              <type>size_type</type>
              <returns>
                <para>The index of the bucket which would contain an element with key <code>k</code>.</para>
              </returns>
              <postconditions>
                <para>The return value is less than <code>bucket_count()</code></para>
              </postconditions>
            </method>
            <overloaded-method name="begin">
              <signature>
                <parameter name="n">
                  <paramtype>size_type</paramtype>
                </parameter>
                <type>local_iterator</type>
              </signature>
              <signature cv="const">
                <parameter name="n">
                  <paramtype>size_type</paramtype>
                </parameter>
                <type>const_local_iterator</type>
              </signature>
              <requires>
                <para><code>n</code> shall be in the range <code>[0, bucket_count())</code>.</para>
              </requires>
              <returns>
                <para>A local iterator pointing the first element in the bucket with index <code>n</code>.</para>
              </returns>
            </overloaded-method>
            <overloaded-method name="end">
              <signature>
                <parameter name="n">
                  <paramtype>size_type</paramtype>
                </parameter>
                <type>local_iterator</type>
              </signature>
              <signature cv="const">
                <parameter name="n">
                  <paramtype>size_type</paramtype>
                </parameter>
                <type>const_local_iterator</type>
              </signature>
              <requires>
                <para><code>n</code> shall be in the range <code>[0, bucket_count())</code>.</para>
              </requires>
              <returns>
                <para>A local iterator pointing the 'one past the end' element in the bucket with index <code>n</code>.</para>
              </returns>
            </overloaded-method>
            <method name="cbegin" cv="const">
              <parameter name="n">
                <paramtype>size_type</paramtype>
              </parameter>
              <type>const_local_iterator</type>
              <requires>
                <para><code>n</code> shall be in the range <code>[0, bucket_count())</code>.</para>
              </requires>
              <returns>
                <para>A constant local iterator pointing the first element in the bucket with index <code>n</code>.</para>
              </returns>
            </method>
            <method name="cend">
              <parameter name="n">
                <paramtype>size_type</paramtype>
              </parameter>
              <type>const_local_iterator</type>
              <requires>
                <para><code>n</code> shall be in the range <code>[0, bucket_count())</code>.</para>
              </requires>
              <returns>
                <para>A constant local iterator pointing the 'one past the end' element in the bucket with index <code>n</code>.</para>
              </returns>
            </method>
          </method-group>
          <method-group name="hash policy">
            <method name="load_factor" cv="const">
              <type>float</type>
              <returns>
                <para>The average number of elements per bucket.</para>
              </returns>
            </method>
            <method name="max_load_factor" cv="const">
              <type>float</type>
              <returns>
                <para>Returns the current maximum load factor.</para>
              </returns>
            </method>
            <method name="max_load_factor">
              <parameter name="z">
                <paramtype>float</paramtype>
              </parameter>
              <type>void</type>
              <effects>
                <para>Changes the container's maximum load factor, using <code>z</code> as a hint.</para>
              </effects>
            </method>
            <method name="rehash">
              <parameter name="n">
                <paramtype>size_type</paramtype>
              </parameter>
              <type>void</type>
              <description>
                <para>Changes the number of buckets so that there at least <code>n</code> buckets, and so that the load factor is less than the maximum load factor.</para>
                <para>Invalidates iterators, and changes the order of elements. Pointers and references to elements are not invalidated.</para>
              </description>
              <throws>
                <para>The function has no effect if an exception is thrown, unless it is thrown by the container's hash function or comparison function.</para>
              </throws>
            </method>
            <method name="reserve">
              <parameter name="n">
                <paramtype>size_type</paramtype>
              </parameter>
              <type>void</type>
              <description>
                <para>Invalidates iterators, and changes the order of elements. Pointers and references to elements are not invalidated.</para>
              </description>
              <throws>
                <para>The function has no effect if an exception is thrown, unless it is thrown by the container's hash function or comparison function.</para>
              </throws>
            </method>
          </method-group>
          <free-function-group name="Equality Comparisons">
            <function name="operator==">
              <template>
<?php echo $template_value; ?>
                <template-type-parameter name="Hash">
                </template-type-parameter>
                <template-type-parameter name="Pred">
                </template-type-parameter>
                <template-type-parameter name="Alloc">
                </template-type-parameter>
              </template>
              <parameter name="x">
                <paramtype><?php echo $full_type; ?> const&amp;</paramtype>
              </parameter>
              <parameter name="y">
                <paramtype><?php echo $full_type; ?> const&amp;</paramtype>
              </parameter>
              <type>bool</type>
              <description>
<?php if($equivalent_keys): ?>
                <para>Return <code>true</code> if <code>x.size() ==
                y.size</code> and for every equivalent key group in
                <code>x</code>, there is a group in <code>y</code>
                for the same key, which is a permutation (using
                <code>operator==</code> to compare the value types).
                </para>
<?php else: ?>
                <para>Return <code>true</code> if <code>x.size() ==
                y.size</code> and for every element in <code>x</code>,
                there is an element in <code>y</code> with the same
                for the same key, with an equal value (using
                <code>operator==</code> to compare the value types).
                </para>
<?php endif; ?>
              </description>
              <notes>
                <para>The behavior of this function was changed to match
                  the C++11 standard in Boost 1.48.</para>
                <para>Behavior is undefined if the two containers don't have
                    equivalent equality predicates.</para>
              </notes>
            </function>
            <function name="operator!=">
              <template>
<?php echo $template_value; ?>
                <template-type-parameter name="Hash">
                </template-type-parameter>
                <template-type-parameter name="Pred">
                </template-type-parameter>
                <template-type-parameter name="Alloc">
                </template-type-parameter>
              </template>
              <parameter name="x">
                <paramtype><?php echo $full_type; ?> const&amp;</paramtype>
              </parameter>
              <parameter name="y">
                <paramtype><?php echo $full_type; ?> const&amp;</paramtype>
              </parameter>
              <type>bool</type>
              <description>
<?php if($equivalent_keys): ?>
                <para>Return <code>false</code> if <code>x.size() ==
                y.size</code> and for every equivalent key group in
                <code>x</code>, there is a group in <code>y</code>
                for the same key, which is a permutation (using
                <code>operator==</code> to compare the value types).
                </para>
<?php else: ?>
                <para>Return <code>false</code> if <code>x.size() ==
                y.size</code> and for every element in <code>x</code>,
                there is an element in <code>y</code> with the same
                for the same key, with an equal value (using
                <code>operator==</code> to compare the value types).
                </para>
<?php endif; ?>
              </description>
              <notes>
                <para>The behavior of this function was changed to match
                  the C++11 standard in Boost 1.48.</para>
                <para>Behavior is undefined if the two containers don't have
                    equivalent equality predicates.</para>
              </notes>
            </function>
          </free-function-group>
          <free-function-group name="swap">
            <function name="swap">
              <template>
<?php echo $template_value; ?>
                <template-type-parameter name="Hash">
                </template-type-parameter>
                <template-type-parameter name="Pred">
                </template-type-parameter>
                <template-type-parameter name="Alloc">
                </template-type-parameter>
              </template>
              <parameter name="x">
                <paramtype><?php echo $full_type; ?>&amp;</paramtype>
              </parameter>
              <parameter name="y">
                <paramtype><?php echo $full_type; ?>&amp;</paramtype>
              </parameter>
              <type>void</type>
              <effects>
                <para><code>x.swap(y)</code></para>
              </effects>
              <description>
                <para>Swaps the contents of <code>x</code> and <code>y</code>.</para>
                <para>If <code>Allocator::propagate_on_container_swap</code> is declared and
                  <code>Allocator::propagate_on_container_swap::value</code> is true then the
                  containers' allocators are swapped. Otherwise, swapping with unequal allocators
                  results in undefined behavior.</para>
              </description>
              <throws>
                <para>Doesn't throw an exception unless it is thrown by the copy constructor or copy assignment operator of <code>key_equal</code> or <code>hasher</code>.</para>
              </throws>
              <notes>
                <para>The exception specifications aren't quite the same as the C++11 standard, as
                  the equality predieate and hash function are swapped using their copy constructors.</para>
              </notes>
            </function>
          </free-function-group>
        </class>
<?php
}

?>
<!--
Copyright Daniel James 2006-2009
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
--><library-reference>
    <header name="boost/unordered_set.hpp">
      <namespace name="boost">
<?php
echo_unordered_docs(false, false);
echo_unordered_docs(false, true);
?>
      </namespace>
    </header>
    <header name="boost/unordered_map.hpp">
      <namespace name="boost">
<?php
echo_unordered_docs(true, false);
echo_unordered_docs(true, true);
?>
      </namespace>
    </header>
  </library-reference>
