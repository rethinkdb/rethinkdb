<?xml version="1.0" ?>

<!--
Copyright (c) 2002-2003 The Trustees of Indiana University.
                        All rights reserved.
Copyright (c) 2000-2001 University of Notre Dame. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="* xsl:*"/>

  <xsl:include href="unparser.xsl"/>

  <xsl:key name="concepts" match="concept" use="@name"/>

  <!-- The layout type to use for concept descriptions. Can be one of:
       sgi: simulate the SGI STL documentation
       austern: simulate the documentation in Generic Programming and the STL,
         by Matthew H. Austern
       caramel: simulate the formatting from Caramel
       -->
  <xsl:param name="boost.concept.layout" select="'austern'"/>

  <xsl:template match="concept">
    <refentry>
      <xsl:attribute name="id">
        <xsl:call-template name="generate.id"/>
      </xsl:attribute>

      <refmeta>
        <refentrytitle>Concept <xsl:value-of select="@name"/></refentrytitle>
        <manvolnum>7</manvolnum>
      </refmeta>

      <refnamediv>
        <refname><xsl:value-of select="@name"/></refname>
        <xsl:if test="purpose">
          <refpurpose>
            <xsl:apply-templates select="purpose/*|purpose/text()"/>
          </refpurpose>
        </xsl:if>
      </refnamediv>

      <!--
      <refentryinfo>
        <xsl:for-each select="copyright | copyright-include | legalnotice">
          <xsl:choose>
            <xsl:when test="name(.)='copyright'">
              <copyright><xsl:copy-of select="./node()"/></copyright>
            </xsl:when>
            <xsl:when test="name(.)='legalnotice'">
              <legalnotice><xsl:copy-of select="./node()"/></legalnotice>
            </xsl:when>
            <xsl:when test="name(.)='copyright-include'">
              <copyright><xsl:copy-of select="document(concat('../concepts/', @file))/copyright/node()"/></copyright>
            </xsl:when>
          </xsl:choose>
        </xsl:for-each>
      </refentryinfo>
-->

    <xsl:if test="description">
      <xsl:if test="description">
        <refsect1>
          <title>Description</title>
          <xsl:for-each select="description">
            <xsl:apply-templates/>
          </xsl:for-each>
        </refsect1>
      </xsl:if>
    </xsl:if>

    <xsl:if test="refines | refines-when-mutable">
      <refsect1>
        <title>Refinement of</title>
        <itemizedlist>
          <xsl:if test="refines">
            <xsl:for-each select="refines">
              <listitem>
                <para>
                  <xsl:call-template name="concept.link">
                    <xsl:with-param name="name" select="@concept"/>
                  </xsl:call-template>
                </para>
              </listitem>
            </xsl:for-each>
          </xsl:if>
          <xsl:if test="refines-when-mutable">
            <xsl:for-each select="refines-when-mutable">
              <listitem>
                <para>
                  <xsl:text>When mutable: </xsl:text>
                  <xsl:call-template name="concept.link">
                    <xsl:with-param name="name" select="@concept"/>
                  </xsl:call-template>
                </para>
              </listitem>
            </xsl:for-each>
          </xsl:if>
        </itemizedlist>
      </refsect1>
    </xsl:if>

    <!-- This part must be run even if there are no associated types to print out, so the hidden type definitions can be found -->
    <xsl:variable name="definition_list">
      <xsl:call-template name="make-definition-list">
        <xsl:with-param name="typedefs" select="define-type | associated-type"/>
        <xsl:with-param name="definition_list">
          <xsl:for-each select="param/@name">
            @(@<xsl:value-of select="."/>=<xsl:value-of select="."/>@)@
          </xsl:for-each>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:variable>

    <!-- <xsl:message>Definition list: <xsl:value-of select="$definition_list"/></xsl:message> -->

    <xsl:call-template name="print-associated-types">
      <xsl:with-param name="typedefs" select="associated-type"/>
      <xsl:with-param name="definition_list" select="$definition_list"/>
    </xsl:call-template>

    <xsl:call-template name="concept.notation">
      <xsl:with-param name="definition_list" select="$definition_list"/>
    </xsl:call-template>

    <xsl:variable name="notations">
      <xsl:for-each select="notation">
        @@(@@<xsl:call-template name="unparse-cpp">
          <xsl:with-param name="typeref" select="*[1]"/>
          <xsl:with-param name="definition_list" select="$definition_list"/>
          <xsl:with-param name="ignore-cv" select="true()"/>
          <xsl:with-param name="ignore-references" select="true()"/>
        </xsl:call-template>@@=@@<xsl:value-of select="normalize-space(@variables)"/>@@)@@
      </xsl:for-each>
    </xsl:variable>

    <!-- <xsl:message>Notations: <xsl:value-of select="normalize-space($notations)"/> End notations</xsl:message> -->

    <xsl:if test="definition">
      <refsect1>
      <title>Definitions</title>
        <xsl:for-each select="definition">
          <p><xsl:apply-templates/></p>
        </xsl:for-each>
      </refsect1>
    </xsl:if>

    <xsl:if test="valid-type-expression | models | models-when-mutable">
      <refsect1>
        <title>Type expressions</title>
        <variablelist>
          <xsl:for-each select="models">
            <varlistentry>
              <term/>
              <listitem>
                <para>
                  <xsl:call-template name="unparse-operator-definition">
                    <xsl:with-param name="typeref" select="."/>
                    <xsl:with-param name="operator_nodeset" select="key('concepts', @concept)/models-sentence/node()"/>
                    <xsl:with-param name="definition_list" select="$definition_list"/>
                    <xsl:with-param name="notations" select="$notations"/>
                    <xsl:with-param name="ignore-cv" select="false()"/>
                    <xsl:with-param name="self" select="@concept"/>
                    <xsl:with-param name="use-code-block" select="true()"/>
                  </xsl:call-template>
                </para>
              </listitem>
            </varlistentry>
          </xsl:for-each>
          <xsl:for-each select="models-when-mutable">
            <varlistentry>
              <term>Only when mutable</term>
              <listitem>
                <para>
                  <xsl:call-template name="unparse-operator-definition">
                    <xsl:with-param name="typeref" select="."/>
                    <xsl:with-param name="operator_nodeset" select="key('concepts', @concept)/models-sentence/node()"/>
                    <xsl:with-param name="definition_list" select="$definition_list"/>
                    <xsl:with-param name="notations" select="$notations"/>
                    <xsl:with-param name="ignore-cv" select="false()"/>
                    <xsl:with-param name="self" select="@concept"/>
                    <xsl:with-param name="use-code-block" select="true()"/>
                  </xsl:call-template>
                </para>
              </listitem>
            </varlistentry>
          </xsl:for-each>
          <xsl:for-each select="valid-type-expression">
            <varlistentry>
              <term><xsl:value-of select="@name"/></term>
              <listitem>
                <para>
                  <type>
                    <xsl:call-template name="unparse-cpp">
                      <xsl:with-param name="typeref" select="*[2]"/>
                      <xsl:with-param name="definition_list" select="$definition_list"/>
                      <xsl:with-param name="notations" select="normalize-space($notations)"/>
                    </xsl:call-template>
                  </type>

                  <xsl:comment/> must be
                  <xsl:for-each select="return-type/*">
                    <xsl:if test="position()!=1 and last()!=2">, </xsl:if>
                    <xsl:if test="position()=last() and last()!=1"> and </xsl:if>
                    <xsl:call-template name="unparse-constraint">
                      <xsl:with-param name="constraint" select="."/>
                      <xsl:with-param name="definition_list" select="$definition_list"/>
                      <xsl:with-param name="type-expr-mode" select="true()"/>
                    </xsl:call-template>
                  </xsl:for-each><xsl:comment/>.
                </para>

                <xsl:if test="description">
                  <xsl:for-each select="description">
                    <xsl:apply-templates/>
                  </xsl:for-each>
                </xsl:if>
              </listitem>
            </varlistentry>
          </xsl:for-each>
        </variablelist>
      </refsect1>
    </xsl:if>

    <xsl:if test="valid-expression">
      <refsect1>
      <title>Valid expressions</title>

      <xsl:variable name="columns">
        <xsl:if test="valid-expression/return-type">
          <xsl:text>T</xsl:text>
        </xsl:if>
        <xsl:if test="valid-expression/precondition">
          <xsl:text>P</xsl:text>
        </xsl:if>
        <xsl:if test="valid-expression/semantics">
          <xsl:text>S</xsl:text>
        </xsl:if>
        <xsl:if test="valid-expression/postcondition">
          <xsl:text>O</xsl:text>
        </xsl:if>
      </xsl:variable>

      <informaltable>
        <tgroup>
          <xsl:attribute name="cols">
            <xsl:value-of select="string-length($columns) + 2"/>
          </xsl:attribute>
          <thead>
            <row>
              <entry>Name</entry>
              <entry>Expression</entry>
              <xsl:if test="contains($columns, 'T')">
                <entry>Type</entry>
              </xsl:if>
              <xsl:if test="contains($columns, 'P')">
                <entry>Precondition</entry>
              </xsl:if>
              <xsl:if test="contains($columns, 'S')">
                <entry>Semantics</entry>
              </xsl:if>
              <xsl:if test="contains($columns, 'O')">
                <entry>Postcondition</entry>
              </xsl:if>
            </row>
          </thead>
          <tbody>
            <xsl:apply-templates select="valid-expression">
              <xsl:with-param name="definition_list"
                select="$definition_list"/>
              <xsl:with-param name="notations"
                select="normalize-space($notations)"/>
              <xsl:with-param name="columns" select="$columns"/>
            </xsl:apply-templates>
          </tbody>
        </tgroup>
      </informaltable>
      <!-- Doug prefers the table
      <variablelist>
        <xsl:for-each select="valid-expression">
          <xsl:variable name="as-cxx-value">
            <xsl:call-template name="unparse-cpp">
              <xsl:with-param name="typeref" select="*[1]"/>
              <xsl:with-param name="definition_list" select="$definition_list"/>
              <xsl:with-param name="notations" select="normalize-space($notations)"/>
            </xsl:call-template>
          </xsl:variable>
          <varlistentry>
            <term><xsl:value-of select="@name"/>: <literal><xsl:value-of select="$as-cxx-value"/></literal></term>
            <listitem><variablelist>
              <xsl:if test="return-type/*">
                <varlistentry><term>Return value</term><listitem><para>
                  <xsl:for-each select="return-type/*">
                    <xsl:if test="position()!=1 and last()!=2">, </xsl:if>
                    <xsl:if test="position()=last() and last()!=1"> and </xsl:if>
                    <xsl:call-template name="unparse-constraint">
                      <xsl:with-param name="constraint" select="."/>
                      <xsl:with-param name="definition_list" select="$definition_list"/>
                      <xsl:with-param name="capitalize" select="position()=1"/>
                    </xsl:call-template>
                  </xsl:for-each>
                </para></listitem></varlistentry>
              </xsl:if>

              <xsl:for-each select="precondition">
                <varlistentry><term>Precondition</term><listitem><para>
                  <xsl:apply-templates/>
                </para></listitem></varlistentry>
              </xsl:for-each>

              <xsl:for-each select="semantics">
                <varlistentry><term>Semantics</term><listitem><para>
                  <xsl:apply-templates/>
                </para></listitem></varlistentry>
              </xsl:for-each>

              <xsl:for-each select="postcondition">
                <varlistentry><term>Postcondition</term><listitem><para>
                  <xsl:apply-templates/>
                </para></listitem></varlistentry>
              </xsl:for-each>

            </variablelist></listitem>
          </varlistentry>

        </xsl:for-each>
      </variablelist>
-->
      </refsect1>
    </xsl:if>

    <xsl:if test="complexity">
      <refsect1>
      <title>Complexity</title>
        <xsl:for-each select="complexity">
          <para><xsl:apply-templates/></para>
        </xsl:for-each>
      </refsect1>
    </xsl:if>

    <xsl:if test="invariant">
      <refsect1>
      <title>Invariants</title>
      <variablelist>
        <xsl:for-each select="invariant">
          <varlistentry>
            <term><xsl:value-of select="@name"/></term>
            <listitem>
              <para><xsl:apply-templates/></para>
            </listitem>
          </varlistentry>
        </xsl:for-each>
      </variablelist>
      </refsect1>
    </xsl:if>

    <xsl:if test="example-model">
      <refsect1>
      <title>Models</title>
        <itemizedlist>
        <xsl:for-each select="example-model">
          <listitem>
            <simplelist type="inline">
            <xsl:for-each select="*">
              <xsl:variable name="example-value">
                <xsl:call-template name="unparse-cpp">
                  <xsl:with-param name="typeref" select="."/>
                  <xsl:with-param name="definition_list" select="$definition_list"/>
                </xsl:call-template>
              </xsl:variable>
              <member><type><xsl:value-of select="$example-value"/></type></member>
            </xsl:for-each>
            </simplelist>
          </listitem>
        </xsl:for-each>
        </itemizedlist>
      </refsect1>
    </xsl:if>

    <xsl:variable name="see-also-list-0" select="concept-ref | see-also | refines | refines-when-mutable | models-as-first-arg | models | models-when-mutable"/>
    <xsl:variable name="see-also-list-1" select="$see-also-list-0[string(@name | @concept) != string(../@name)]"/>
    <xsl:variable name="see-also-list" select="$see-also-list-1[not(string(@name|@concept) = (preceding::*/@name | preceding::*/@concept | ancestor::*/@name | ancestor::*/@concept))]"/>
    <xsl:if test="$see-also-list">
      <refsect1>
        <title>See also</title>
        <itemizedlist>
          <xsl:for-each select="$see-also-list">
            <xsl:sort select="string(@name|@concept)" data-type="text"/>
            <listitem>
              <para>
                <xsl:call-template name="concept.link">
                  <xsl:with-param name="name" select="@name|@concept"/>
                </xsl:call-template>
              </para>
            </listitem>
          </xsl:for-each>
        </itemizedlist>
      </refsect1>
    </xsl:if>

  </refentry>
  </xsl:template>

  <xsl:template name="unparse-constraint">
    <xsl:param name="constraint"/>
    <xsl:param name="definition_list"/>
    <xsl:param name="type-expr-mode" select="false()"/>
    <xsl:param name="capitalize" select="true()"/>

    <xsl:choose>

      <xsl:when test="name($constraint)='require-same-type'">
        <xsl:if test="$type-expr-mode">identical to </xsl:if>
        <type>
          <xsl:call-template name="unparse-cpp">
            <xsl:with-param name="typeref" select="$constraint/*[1]"/>
            <xsl:with-param name="definition_list" select="definition_list"/>
          </xsl:call-template>
        </type>
      </xsl:when>

      <xsl:when test="name($constraint)='convertible-to'">
        <xsl:choose>
          <xsl:when test="$type-expr-mode">convertible to </xsl:when>
          <xsl:when test="not($type-expr-mode) and $capitalize">Convertible to </xsl:when>
          <xsl:when test="not($type-expr-mode) and not($capitalize)">convertible to </xsl:when>
        </xsl:choose>
        <type>
          <xsl:call-template name="unparse-cpp">
            <xsl:with-param name="typeref" select="$constraint/*[1]"/>
            <xsl:with-param name="definition_list" select="definition_list"/>
          </xsl:call-template>
        </type>
      </xsl:when>

      <xsl:when test="name($constraint)='derived-from'">
        <xsl:choose>
          <xsl:when test="$type-expr-mode">derived from </xsl:when>
          <xsl:when test="not($type-expr-mode) and $capitalize">Derived from </xsl:when>
          <xsl:when test="not($type-expr-mode) and not($capitalize)">derived from </xsl:when>
        </xsl:choose>
        <type>
          <xsl:call-template name="unparse-cpp">
            <xsl:with-param name="typeref" select="$constraint/*[1]"/>
            <xsl:with-param name="definition_list" select="definition_list"/>
          </xsl:call-template>
        </type>
      </xsl:when>

      <xsl:when test="name($constraint)='assignable-to'">
        <xsl:choose>
          <xsl:when test="$type-expr-mode">assignable to </xsl:when>
          <xsl:when test="not($type-expr-mode) and $capitalize">Assignable to </xsl:when>
          <xsl:when test="not($type-expr-mode) and not($capitalize)">assignable to </xsl:when>
        </xsl:choose>
        <type>
          <xsl:call-template name="unparse-cpp">
            <xsl:with-param name="typeref" select="$constraint/*[1]"/>
            <xsl:with-param name="definition_list" select="definition_list"/>
          </xsl:call-template>
        </type>
      </xsl:when>

      <xsl:when test="name($constraint)='models-as-first-arg'">
        <xsl:choose>
          <xsl:when test="$type-expr-mode"> a model </xsl:when>
          <xsl:when test="not($type-expr-mode) and $capitalize"> Models </xsl:when>
          <xsl:when test="not($type-expr-mode) and not($capitalize)"> models </xsl:when>
        </xsl:choose>
        <xsl:if test="$constraint/*"><xsl:comment/>
          (along with <xsl:for-each select="$constraint/*"><type>
              <xsl:call-template name="unparse-cpp">
                <xsl:with-param name="typeref" select="."/>
                <xsl:with-param name="definition_list" select="definition_list"/>
              </xsl:call-template>
            </type>
            <xsl:choose>
              <xsl:when test="position()=last()"/>
              <xsl:when test="position()=last()-1 and last()=2"> and </xsl:when>
              <xsl:when test="position()=last()-1 and last()!=2">, and </xsl:when>
              <xsl:otherwise>, </xsl:otherwise>
            </xsl:choose><xsl:comment/>
          </xsl:for-each><xsl:comment/>) <xsl:comment/>
        </xsl:if><xsl:comment/>
        <xsl:if test="$type-expr-mode"> of </xsl:if>
        <xsl:call-template name="concept.link">
          <xsl:with-param name="name" select="$constraint/@concept"/>
        </xsl:call-template>
      </xsl:when>

    </xsl:choose>
  </xsl:template>

  <xsl:template name="make-definition-list">
    <xsl:param name="typedefs"/>
    <xsl:param name="definition_list"/>

    <xsl:choose>
      <xsl:when test="$typedefs">
        <xsl:variable name="type_definition">
          <xsl:if test="name($typedefs[1]/*[1])!='description'">
            <xsl:call-template name="unparse-cpp">
              <xsl:with-param name="typeref" select="$typedefs[1]/*[1]"/>
              <xsl:with-param name="definition_list" select="$definition_list"/>
            </xsl:call-template>
          </xsl:if>
        </xsl:variable>

        <xsl:variable name="new_type_definition">
          <xsl:choose>
            <xsl:when test="name($typedefs[1])='associated-type'">
              <xsl:value-of select="$typedefs[1]/@name"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$type_definition"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:call-template name="make-definition-list">
          <xsl:with-param name="typedefs" select="$typedefs[position()!=1]"/>
          <xsl:with-param name="definition_list" select="concat($definition_list, ' @(@', $typedefs[1]/@name, '=', $new_type_definition, '@)@')"/>
        </xsl:call-template>

      </xsl:when>

      <xsl:otherwise> <!-- End of expression list, emit the results that have accumulated -->
        <xsl:value-of select="$definition_list"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="print-associated-types">
    <xsl:param name="typedefs"/>
    <xsl:param name="definition_list"/>

    <xsl:if test="$typedefs">
      <refsect1>
        <title>Associated types</title>

        <xsl:choose>
          <xsl:when test="$boost.concept.layout='sgi'">
            <informaltable>
              <tgroup cols="2">
                <tbody>
                  <xsl:apply-templates select="associated-type" mode="sgi">
                    <xsl:with-param name="definition_list"
                      select="$definition_list"/>
                  </xsl:apply-templates>
                </tbody>
              </tgroup>
            </informaltable>
          </xsl:when>
          <xsl:when test="$boost.concept.layout='austern'">
            <itemizedlist>
              <xsl:apply-templates select="associated-type" mode="austern">
                <xsl:with-param name="definition_list"
                  select="$definition_list"/>
              </xsl:apply-templates>
            </itemizedlist>
          </xsl:when>
          <xsl:when test="$boost.concept.layout='caramel'">
            <segmentedlist>
              <segtitle>Name</segtitle>
              <segtitle>Code</segtitle>
              <segtitle>Description</segtitle>
              <xsl:for-each select="$typedefs">
                <xsl:variable name="type_definition">
                  <xsl:call-template name="unparse-cpp">
                    <xsl:with-param name="typeref" select="*[1]"/>
                    <xsl:with-param name="definition_list" select="$definition_list"/>
                  </xsl:call-template>
                </xsl:variable>
                <seglistitem>
                  <seg><xsl:value-of select="@name"/></seg>
                  <seg><xsl:value-of select="$type_definition"/></seg>
                  <seg>
                    <xsl:for-each select="description">
                      <xsl:call-template name="description"/>
                    </xsl:for-each>
                  </seg>
                </seglistitem>
              </xsl:for-each>
            </segmentedlist>
          </xsl:when>
        </xsl:choose>
      </refsect1>
    </xsl:if>
  </xsl:template>

  <xsl:template name="comma-list">
    <xsl:param name="list"/>

    <xsl:if test="$list!=''">
      <term><varname>
        <xsl:if test="substring-before($list,' ')=''"><xsl:value-of select="$list"/></xsl:if>
        <xsl:value-of select="substring-before($list,' ')"/>
      </varname></term>
      <xsl:call-template name="comma-list">
  <xsl:with-param name="list" select="substring-after($list,' ')"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="associated-type" mode="sgi">
    <row>
      <entry><simpara><xsl:value-of select="@name"/></simpara></entry>

      <entry>
        <para>
          <xsl:for-each select="description">
            <xsl:apply-templates/>
          </xsl:for-each>
        </para>
      </entry>
    </row>
  </xsl:template>

  <xsl:template match="associated-type" mode="austern">
    <xsl:param name="definition_list" select="''"/>

    <listitem>
      <para>
        <emphasis role="bold"><xsl:value-of select="@name"/></emphasis>

        <xsl:call-template name="preformatted">
          <xsl:with-param name="text">
            <xsl:call-template name="unparse-cpp">
              <xsl:with-param name="typeref" select="*[1]"/>
              <xsl:with-param name="definition_list" select="$definition_list"/>
            </xsl:call-template>
          </xsl:with-param>
        </xsl:call-template>

        <xsl:for-each select="description">
          <xsl:apply-templates/>
        </xsl:for-each>
      </para>
    </listitem>
  </xsl:template>

  <xsl:template match="valid-expression">
    <xsl:param name="definition_list"/>
    <xsl:param name="notations"/>
    <xsl:param name="columns"/>

    <row>
      <entry><simpara><xsl:value-of select="@name"/></simpara></entry>

      <entry>
        <simpara>
          <xsl:call-template name="unparse-cpp">
            <xsl:with-param name="typeref" select="*[1]"/>
            <xsl:with-param name="definition_list" select="$definition_list"/>
            <xsl:with-param name="notations" select="$notations"/>
          </xsl:call-template>
        </simpara>
      </entry>

      <xsl:if test="contains($columns, 'T')">
        <entry>
          <simpara>
            <xsl:for-each select="return-type/*">
              <xsl:if test="position()!=1 and last()!=2">, </xsl:if>
              <xsl:if test="position()=last() and last()!=1"> and </xsl:if>
              <xsl:call-template name="unparse-constraint">
                <xsl:with-param name="constraint" select="."/>
                <xsl:with-param name="definition_list"
                  select="$definition_list"/>
                <xsl:with-param name="capitalize" select="position()=1"/>
              </xsl:call-template>
            </xsl:for-each>
          </simpara>
        </entry>
      </xsl:if>

      <xsl:if test="contains($columns, 'P')">
        <entry>
          <xsl:for-each select="precondition">
            <simpara><xsl:apply-templates/></simpara>
          </xsl:for-each>
        </entry>
      </xsl:if>

      <xsl:if test="contains($columns, 'S')">
        <entry>
          <xsl:for-each select="semantics">
            <simpara><xsl:apply-templates/></simpara>
          </xsl:for-each>
        </entry>
      </xsl:if>

      <xsl:if test="contains($columns, 'O')">
        <entry>
          <xsl:for-each select="postcondition">
            <simpara><xsl:apply-templates/></simpara>
          </xsl:for-each>
        </entry>
      </xsl:if>
    </row>
  </xsl:template>

  <xsl:template name="concept.notation">
    <xsl:param name="definition_list"/>

    <refsect1>
      <title>Notation</title>
      <variablelist>
        <xsl:for-each select="param">
          <varlistentry>
            <term><xsl:value-of select="@name"/></term>
            <listitem>
              <simpara>
                <xsl:text>A type playing the role of </xsl:text>
                <xsl:value-of select="@role"/>
                <xsl:text> in the </xsl:text>
                <xsl:call-template name="concept.link">
                  <xsl:with-param name="name" select="../@name"/>
                </xsl:call-template>
                <xsl:text> concept.</xsl:text>
              </simpara>
            </listitem>
          </varlistentry>
        </xsl:for-each>
        <xsl:for-each select="notation">
          <xsl:variable name="notation_name">
            <xsl:call-template name="comma-list">
              <xsl:with-param name="list"
                select="normalize-space(@variables)"/>
            </xsl:call-template>
          </xsl:variable>

          <varlistentry>
            <xsl:copy-of select="$notation_name"/>
            <listitem>
              <simpara>
                <xsl:variable name="output-plural" select="substring-before(normalize-space(@variables),' ')!=''"/>
                <xsl:if test="name(*[1])='sample-value'">Object<xsl:if test="$output-plural">s</xsl:if> of type </xsl:if>
                <xsl:variable name="typeref-to-print" select="*[name()!='sample-value'] | sample-value/*[name()!='sample-value']"/>
                <xsl:call-template name="unparse-cpp">
                  <xsl:with-param name="typeref" select="$typeref-to-print"/>
                  <xsl:with-param name="definition_list" select="$definition_list"/>
                  <xsl:with-param name="ignore-cv" select="true()"/>
                  <xsl:with-param name="ignore-references" select="true()"/>
                </xsl:call-template>
              </simpara>
            </listitem>
          </varlistentry>
        </xsl:for-each>
      </variablelist>
    </refsect1>
  </xsl:template>

  <xsl:template name="concept.link">
    <xsl:param name="name" select="text()"/>
    <xsl:param name="warn" select="true()"/>
    <xsl:param name="text" select="$name"/>
    <xsl:variable name="node" select="key('concepts', $name)"/>

    <xsl:choose>
      <xsl:when test="count($node)=0">
        <xsl:if test="$warn">
          <xsl:message>
            <xsl:text>warning: cannot find concept '</xsl:text>
            <xsl:value-of select="$name"/>
            <xsl:text>'</xsl:text>
          </xsl:message>
        </xsl:if>
        <xsl:value-of select="$text"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="$node"/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="remove-whitespace">
    <xsl:param name="text" select="text()"/>

    <xsl:variable name="normalized" select="normalize-space($text)"/>
    <xsl:choose>
      <xsl:when test="contains($normalized, ' ')">
        <xsl:value-of select="substring-before($normalized, ' ')"/>
        <xsl:call-template name="remove-whitespace">
          <xsl:with-param name="text"
            select="substring-after($normalized, ' ')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$normalized"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="concept" mode="generate.id">
    <xsl:call-template name="remove-whitespace">
      <xsl:with-param name="text" select="@name"/>
    </xsl:call-template>
  </xsl:template>
</xsl:stylesheet>
