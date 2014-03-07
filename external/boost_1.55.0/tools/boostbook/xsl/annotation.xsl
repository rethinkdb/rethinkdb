<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:variable name="uppercase-letters" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>
  <xsl:variable name="lowercase-letters" select="'abcdefghijklmnopqrstuvwxyz'"/>

  <xsl:key name="classes" match="class|struct|union|typedef" use="@name"/>
  <xsl:key name="methods" match="method|overloaded-method" use="@name"/>
  <xsl:key name="functions" match="function|overloaded-function" use="@name"/>
  <xsl:key name="enums" match="enum" use="@name"/>
  <xsl:key name="concepts" match="concept" use="@name"/>
  <xsl:key name="libraries" match="library" use="@name"/>
  <xsl:key name="macros" match="macro" use="@name"/>
  <xsl:key name="headers" match="header" use="@name"/>
  <xsl:key name="globals" match="namespace/data-member|header/data-member" use="@name"/>
  <xsl:key name="named-entities"
    match="class|struct|union|concept|function|overloaded-function|macro|library|namespace/data-member|header/data-member|*[attribute::id]"
    use="translate(@name|@id, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')"/>

  <xsl:template match="function|overloaded-function" mode="generate.id">
    <xsl:call-template name="fully-qualified-id">
      <xsl:with-param name="node" select="."/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="classname" mode="annotation">
    <!-- Determine the (possibly qualified) class name we are looking for -->
    <xsl:variable name="fullname">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Strip off any instantiation -->
    <xsl:variable name="name">
      <xsl:choose>
        <xsl:when test="contains($fullname, '&lt;')">
          <xsl:value-of select="substring-before($fullname, '&lt;')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$fullname"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Determine the unqualified name -->
    <xsl:variable name="unqualified-name">
      <xsl:call-template name="strip-qualifiers">
        <xsl:with-param name="name" select="$name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:call-template name="cxx-link-name">
      <xsl:with-param name="lookup" select="."/>
      <xsl:with-param name="type" select="'class'"/>
      <xsl:with-param name="name" select="$name"/>
      <xsl:with-param name="display-name" select="string(.)"/>
      <xsl:with-param name="unqualified-name" select="$unqualified-name"/>
      <xsl:with-param name="nodes" select="key('classes', $unqualified-name)"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="globalname" mode="annotation">
    <!-- Determine the (possibly qualified) global name we are looking for -->
    <xsl:variable name="name">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Determine the unqualified name -->
    <xsl:variable name="unqualified-name">
      <xsl:call-template name="strip-qualifiers">
        <xsl:with-param name="name" select="$name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:call-template name="cxx-link-name">
      <xsl:with-param name="lookup" select="."/>
      <xsl:with-param name="type" select="'data-member'"/>
      <xsl:with-param name="name" select="$name"/>
      <xsl:with-param name="display-name" select="string(.)"/>
      <xsl:with-param name="unqualified-name" select="$unqualified-name"/>
      <xsl:with-param name="nodes" select="key('globals', $unqualified-name)"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="methodname" mode="annotation">
    <!-- Determine the (possibly qualified) method name we are looking for -->
    <xsl:variable name="fullname">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Strip off any call -->
    <xsl:variable name="name">
      <xsl:choose>
        <xsl:when test="contains($fullname, 'operator()')">
          <xsl:value-of select="substring-before($fullname, 'operator()')"/>
          <xsl:value-of select="'operator()'"/>
        </xsl:when>
        <xsl:when test="contains($fullname, '(')">
          <xsl:value-of select="substring-before($fullname, '(')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$fullname"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Determine the unqualified name -->
    <xsl:variable name="unqualified-name">
      <xsl:call-template name="strip-qualifiers">
        <xsl:with-param name="name" select="$name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:call-template name="cxx-link-name">
      <xsl:with-param name="lookup" select="."/>
      <xsl:with-param name="type" select="'method'"/>
      <xsl:with-param name="name" select="$name"/>
      <xsl:with-param name="display-name" select="string(.)"/>
      <xsl:with-param name="unqualified-name" select="$unqualified-name"/>
      <xsl:with-param name="nodes" select="key('methods', $unqualified-name)"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="functionname" mode="annotation">
    <!-- Determine the (possibly qualified) function name we are
         looking for -->
    <xsl:variable name="fullname">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Strip off any call -->
    <xsl:variable name="name">
      <xsl:choose>
        <xsl:when test="contains($fullname, '(')">
          <xsl:value-of select="substring-before($fullname, '(')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$fullname"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Determine the unqualified name -->
    <xsl:variable name="unqualified-name">
      <xsl:call-template name="strip-qualifiers">
        <xsl:with-param name="name" select="$name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:call-template name="cxx-link-name">
      <xsl:with-param name="lookup" select="."/>
      <xsl:with-param name="type" select="'function'"/>
      <xsl:with-param name="name" select="$name"/>
      <xsl:with-param name="display-name" select="string(.)"/>
      <xsl:with-param name="unqualified-name" select="$unqualified-name"/>
      <xsl:with-param name="nodes"
        select="key('functions', $unqualified-name)"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="enumname" mode="annotation">
    <!-- Determine the (possibly qualified) enum name we are
         looking for -->
    <xsl:variable name="fullname">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Strip off any call -->
    <xsl:variable name="name">
      <xsl:choose>
        <xsl:when test="contains($fullname, '(')">
          <xsl:value-of select="substring-before($fullname, '(')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$fullname"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Determine the unqualified name -->
    <xsl:variable name="unqualified-name">
      <xsl:call-template name="strip-qualifiers">
        <xsl:with-param name="name" select="$name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:call-template name="cxx-link-name">
      <xsl:with-param name="lookup" select="."/>
      <xsl:with-param name="type" select="'enum'"/>
      <xsl:with-param name="name" select="$name"/>
      <xsl:with-param name="display-name" select="string(.)"/>
      <xsl:with-param name="unqualified-name" select="$unqualified-name"/>
      <xsl:with-param name="nodes"
        select="key('enums', $unqualified-name)"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="libraryname" mode="annotation">
    <xsl:variable name="name">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="text()"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="node" select="key('libraries', $name)"/>

    <xsl:choose>
      <xsl:when test="count($node)=0">
        <xsl:message>
          <xsl:text>warning: Cannot find library '</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>'</xsl:text>
        </xsl:message>
        <xsl:value-of select="$name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="library.link">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="name" select="text()"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="conceptname" mode="annotation">
    <xsl:param name="name" select="text()"/>

    <xsl:call-template name="concept.link">
      <xsl:with-param name="name" select="$name"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="macroname" mode="annotation">
    <xsl:param name="name">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:param>

    <xsl:variable name="node" select="key('macros', $name)"/>
    <xsl:choose>
      <xsl:when test="count($node) = 0">
        <xsl:message>
          <xsl:text>warning: cannot find macro `</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>'</xsl:text>
        </xsl:message>
        <xsl:value-of select="$name"/>
      </xsl:when>

      <xsl:when test="count($node) = 1">
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="$node"/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="string(.)"/>
        </xsl:call-template>
      </xsl:when>

      <xsl:otherwise>
        <xsl:message>
          <xsl:text>error: macro `</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>' is multiply defined.</xsl:text>
        </xsl:message>
        <xsl:value-of select="$node"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="headername" mode="annotation">
    <xsl:variable name="name">
      <xsl:choose>
        <xsl:when test="@alt">
          <xsl:value-of select="@alt"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="node" select="key('headers', $name)"/>
    <xsl:choose>
      <xsl:when test="count($node) = 0">
        <xsl:message>
          <xsl:text>warning: cannot find header `</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>'</xsl:text>
        </xsl:message>
        <xsl:value-of select="$name"/>
      </xsl:when>

      <xsl:when test="count($node) = 1">
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="$node"/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="string(.)"/>
        </xsl:call-template>
      </xsl:when>

      <xsl:otherwise>
        <xsl:message>
          <xsl:text>error: header `</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>' is multiply defined.</xsl:text>
        </xsl:message>
        <xsl:value-of select="$node"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="text()" mode="annotation">
    <xsl:param name="highlight" select="false()"/>
    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="source-highlight">
          <xsl:with-param name="text" select="."/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="."/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="programlisting" mode="annotation">
    <programlisting>
      <xsl:apply-templates mode="annotation">
        <xsl:with-param name="highlight" select="true()"/>
      </xsl:apply-templates>
    </programlisting>
  </xsl:template>
  
  <xsl:template match="code" mode="annotation">
    <computeroutput>
      <xsl:apply-templates mode="annotation"/>
    </computeroutput>
  </xsl:template>

  <xsl:template match="code[@language='jam']" mode="annotation">
    <computeroutput>
      <xsl:apply-templates mode="annotation"/>
    </computeroutput>
  </xsl:template>

  <xsl:template match="bold" mode="annotation">
    <emphasis role="bold">
      <xsl:apply-templates mode="annotation"/>
    </emphasis>
  </xsl:template>

  <xsl:template match="description" mode="annotation">
    <xsl:apply-templates mode="annotation"/>
  </xsl:template>

  <xsl:template match="type" mode="annotation">
    <xsl:apply-templates mode="annotation"/>
  </xsl:template>

  <xsl:template match="comment()" mode="annotation">
    <xsl:copy/>
  </xsl:template>

  <xsl:template match="node()" mode="annotation">
    <xsl:param name="highlight" select="false()"/>

    <xsl:element name="{name(.)}">
      <xsl:copy-of select="./@*"/>
      <xsl:apply-templates select="./*|./text()" mode="annotation">
        <xsl:with-param name="highlight" select="$highlight"/>
      </xsl:apply-templates>
    </xsl:element>
  </xsl:template>

  <!-- The "purpose" mode strips simpara/para elements so that we can
       place the resulting text into a comment in the synopsis. -->
  <xsl:template match="para|simpara" mode="purpose">
    <xsl:apply-templates mode="annotation"/>
  </xsl:template>

  <xsl:template match="*" mode="purpose">
    <xsl:apply-templates select="." mode="annotation"/>
  </xsl:template>

  <xsl:template match="text()" mode="purpose">
    <xsl:apply-templates select="." mode="annotation"/>
  </xsl:template>
</xsl:stylesheet>
