<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:output method="xml" indent="yes" standalone="yes"/>

  <xsl:strip-space elements="sequence-group or-group element element-name"/>

  <xsl:key name="elements" match="element" use="@name"/>
  <xsl:key name="attributes" match="attlist" use="@name"/>
  <xsl:key name="attribute-purposes" match="attpurpose" use="@name"/>

  <xsl:template match="dtd">
    <section id="reference">
      <title>Reference</title>
      <para>
        <xsl:text>Elements:</xsl:text>
        <itemizedlist spacing="compact">
          <xsl:apply-templates select="element" mode="synopsis">
            <xsl:sort select="@name"/>
          </xsl:apply-templates>
        </itemizedlist>
      </para>
      <xsl:apply-templates select="element"/>
    </section>
  </xsl:template>

  <!-- Element synopsis -->
  <xsl:template match="element" mode="synopsis">
    <listitem>
      <simpara>
        <link>
          <xsl:attribute name="linkend">
            <xsl:value-of select="concat('boostbook.dtd.',@name)"/>
          </xsl:attribute>
          <xsl:text>Element </xsl:text>
          <sgmltag><xsl:value-of select="@name"/></sgmltag>
          <xsl:text> - </xsl:text>
          <xsl:apply-templates select="purpose"/>
        </link>
      </simpara>
    </listitem>
  </xsl:template>

  <!-- Elements are transformed into DocBook refentry elements -->
  <xsl:template match="element">
    <refentry>
      <xsl:attribute name="id">
        <xsl:value-of select="concat('boostbook.dtd.',@name)"/>
      </xsl:attribute>

      <refmeta>
        <refentrytitle>
          BoostBook element <sgmltag><xsl:value-of select="@name"/></sgmltag>
        </refentrytitle>
        <manvolnum>9</manvolnum>
      </refmeta>
      <refnamediv>
        <refname><xsl:value-of select="@name"/></refname>
        <refpurpose><xsl:apply-templates select="purpose"/></refpurpose>
      </refnamediv>
      <refsynopsisdiv>
        <xsl:value-of select="@name"/><xsl:text> ::= </xsl:text>
        <xsl:apply-templates select="content-model-expanded"/>
      </refsynopsisdiv>
      <xsl:apply-templates select="description"/>
      <xsl:apply-templates select="key('attributes', @name)"/>
    </refentry>
  </xsl:template>

  <xsl:template match="content-model-expanded">
    <xsl:apply-templates/>
  </xsl:template>

  <!-- Sequences -->
  <xsl:template match="sequence-group">
    <xsl:param name="separator" select="''"/>

    <xsl:if test="preceding-sibling::*">
      <xsl:value-of select="$separator"/>
    </xsl:if>
    <xsl:text>(</xsl:text>
    <xsl:apply-templates>
      <xsl:with-param name="separator" select="', '"/>
    </xsl:apply-templates>
    <xsl:text>)</xsl:text>
    <xsl:value-of select="@occurrence"/>
  </xsl:template>

  <!-- Alternatives -->
  <xsl:template match="or-group">
    <xsl:param name="separator" select="''"/>

    <xsl:if test="preceding-sibling::*">
      <xsl:value-of select="$separator"/>
    </xsl:if>
    <xsl:text>(</xsl:text>
    <xsl:apply-templates>
      <xsl:with-param name="separator" select="'| '"/>
    </xsl:apply-templates>
    <xsl:text>)</xsl:text>
    <xsl:value-of select="@occurrence"/>
  </xsl:template>

  <!-- Element references -->
  <xsl:template match="element-name">
    <xsl:param name="separator" select="''"/>

    <xsl:if test="preceding-sibling::*">
      <xsl:value-of select="$separator"/>
    </xsl:if>

    <xsl:variable name="element-node" select="key('elements', @name)"/>

    <xsl:choose>
      <xsl:when test="$element-node">
        <link>
          <xsl:attribute name="linkend">
            <xsl:value-of select="concat('boostbook.dtd.',@name)"/>
          </xsl:attribute>
          <xsl:value-of select="@name"/>
        </link>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="@name"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="@occurrence"/>
  </xsl:template>

  <!-- #PCDATA -->
  <xsl:template match="pcdata">
    <xsl:param name="separator" select="''"/>

    <xsl:if test="preceding-sibling::*">
      <xsl:value-of select="$separator"/>
    </xsl:if>

    <xsl:text>#PCDATA</xsl:text>
  </xsl:template>

  <!-- ANY -->
  <xsl:template match="any">
    <xsl:text>ANY</xsl:text>
  </xsl:template>

  <!-- EMPTY -->
  <xsl:template match="empty">
    <xsl:text>EMPTY</xsl:text>
  </xsl:template>

  <!-- Just copy anything in a purpose element -->
  <xsl:template match="purpose">
    <xsl:copy-of select="text()|*"/>
  </xsl:template>

  <!-- Just copy anything in a description element, but place it in a
       section. -->
  <xsl:template match="description">
    <refsection>
      <title>Description</title>
      <xsl:copy-of select="text()|*"/>
    </refsection>
  </xsl:template>

  <!-- Attributes -->
  <xsl:template match="attlist">
    <refsection>
      <title>Attributes</title>
      
      <informaltable>
        <tgroup cols="4">
          <thead>
            <row>
              <entry>Name</entry>
              <entry>Type</entry>
              <entry>Value</entry>
              <entry>Purpose</entry>
            </row>
          </thead>
          <tbody>
            <xsl:apply-templates/>
          </tbody>
        </tgroup>
      </informaltable>
    </refsection>
  </xsl:template>

  <!-- Attribute -->
  <xsl:template match="attribute">
    <row>
      <entry><xsl:value-of select="@name"/></entry>
      <entry><xsl:value-of select="@type"/></entry>
      <entry><xsl:value-of select="@value"/></entry>
      <entry>
        <xsl:choose>
          <xsl:when test="purpose">
            <xsl:apply-templates select="purpose"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="key('attribute-purposes', @name)"/>
          </xsl:otherwise>
        </xsl:choose>
      </entry>
    </row>
  </xsl:template>

  <!-- Eat attribute declarations -->
  <xsl:template match="attdecl"/>
</xsl:stylesheet>
