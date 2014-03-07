<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                version="1.0">

  <!-- Maximum length of directory and file names is 31 characters.
       '.html' uses 5 characters.
       31 - 5 = 26 -->
  <xsl:param name="boost.max.id.part.length">26</xsl:param>

  <!-- Generate an ID for the entity referenced -->
  <xsl:template name="generate.id">
    <xsl:param name="node" select="."/>
    <xsl:apply-templates select="$node" mode="generate.id"/>
  </xsl:template>

  <xsl:template match="*" mode="generate.id">
    <xsl:value-of select="generate-id(.)"/>
    <xsl:text>-bb</xsl:text>
  </xsl:template>

  <xsl:template name="strip-qualifiers-non-template">
    <xsl:param name="name"/>
    <xsl:choose>
      <xsl:when test="contains($name, '&gt;')">
        <xsl:call-template name="strip-qualifiers-non-template">
          <xsl:with-param name="name" select="substring-after($name, '&gt;')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="contains($name, '::')">
        <xsl:call-template name="strip-qualifiers-non-template">
          <xsl:with-param name="name" select="substring-after($name, '::')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="strip-balanced">
    <xsl:param name="name"/>
    <xsl:param name="open" select="'&lt;'"/>
    <xsl:param name="close" select="'&gt;'"/>
    <xsl:param name="depth" select="0"/>
    <xsl:choose>
      <xsl:when test="contains($name, $open)
                and not(contains(substring-before($name, $open), $close))">
        <xsl:call-template name="strip-balanced">
          <xsl:with-param name="name" select="substring-after($name, $open)"/>
          <xsl:with-param name="open" select="$open"/>
          <xsl:with-param name="close" select="$close"/>
          <xsl:with-param name="depth" select="$depth + 1"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="contains($name, $close) and ($depth &gt; 1)">
        <xsl:call-template name="strip-balanced">
          <xsl:with-param name="name" select="substring-after($name, $close)"/>
          <xsl:with-param name="open" select="$open"/>
          <xsl:with-param name="close" select="$close"/>
          <xsl:with-param name="depth" select="$depth - 1"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-after($name, $close)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="strip-qualifiers-template">
    <xsl:param name="name"/>
    <xsl:choose>
      <xsl:when test="contains($name, '::')
                and not(contains(substring-before($name, '::'), '&lt;'))">
        <xsl:call-template name="strip-qualifiers-template">
          <xsl:with-param name="name" select="substring-after($name, '::')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="rest">
          <xsl:call-template name="strip-balanced">
            <xsl:with-param name="name" select="$name"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$rest != ''">
            <xsl:call-template name="strip-qualifiers-template">
              <xsl:with-param name="name" select="$rest"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$name"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Strip the qualifiers off a qualified name and return the unqualified
       name. For instance, "boost::python::function" would become just
       "function".
       Must handle ns::foo                    -> foo
       Must handle ns::foo<bar::baz>          -> foo<bar::baz>
       Must handle ns::foo<bar::baz>::nested  -> nested
       Must handle ns::foo<x>::bar<y>         -> bar<y> -->
  <xsl:template name="strip-qualifiers">
    <xsl:param name="name"/>
    <xsl:choose>
      <xsl:when test="substring($name, string-length($name)) = '&gt;'">
        <xsl:call-template name="strip-qualifiers-template">
          <xsl:with-param name="name" select="$name"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="strip-qualifiers-non-template">
          <xsl:with-param name="name" select="$name"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Build the fully-qualified id of the given node -->
  <xsl:template name="fully-qualified-id">
    <xsl:param name="node"/>

    <xsl:apply-templates select="$node" mode="fully-qualified-name">
      <xsl:with-param name="is.id" select="true()"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Build the fully-qualified name of the given node -->
  <xsl:template name="fully-qualified-name">
    <xsl:param name="node"/>
    <xsl:apply-templates select="$node" mode="fully-qualified-name"/>
  </xsl:template>

  <!-- Hack to make the node we are building the current node so that the
       ancestor:: syntax will work -->
  <xsl:template match="*" mode="fully-qualified-name">
    <xsl:param name="is.id" select="false()" />
    <xsl:call-template name="build-fully-qualified-name">
      <xsl:with-param name="is.id" select="$is.id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- The real routine that builds a fully-qualified name for the current
       node. -->
  <xsl:template name="build-fully-qualified-name">
    <xsl:param name="is.id" select="false()" />

    <!-- Determine the set of ancestor namespaces -->
    <xsl:variable name="ancestors"
      select="ancestor::namespace|
                  ancestor::class|ancestor::struct|ancestor::union|
                  ancestor::class-specialization|ancestor::struct-specialization|ancestor::union-specialization"/>

    <xsl:for-each select="$ancestors">
      <xsl:apply-templates select="." mode="fast-print-id-part">
        <xsl:with-param name="is.id" select="$is.id"/>
      </xsl:apply-templates>
      <xsl:choose>
        <xsl:when test="$is.id"><xsl:text>.</xsl:text></xsl:when>
        <xsl:otherwise><xsl:text>::</xsl:text></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:apply-templates select="." mode="fast-print-id-part">
      <xsl:with-param name="is.id" select="$is.id"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:variable name="elements-with-ids">
    <xsl:apply-templates select="namespace|class|struct|union|class-specialization|struct-specialization|union-specialization"
                         mode="preprocess-ids"/>
  </xsl:variable>
  
  <xsl:variable name="fast-elements" select="exsl:node-set($elements-with-ids)"/>
  
  <xsl:template match="*" mode="preprocess-ids">
    <element>
      <xsl:attribute name="id">
        <xsl:value-of select="generate-id()"/>
      </xsl:attribute>
      <xsl:attribute name="part-id">
        <xsl:call-template name="print-id-part"/>
      </xsl:attribute>
    </element>
  </xsl:template>
  
  <xsl:template name="print-id-part">
    <xsl:apply-templates select="." mode="print-id-part"/>
  </xsl:template>
  
  <xsl:template match="*" mode="fast-print-id-part">
    <xsl:param name="is.id"/>
    <xsl:choose>
      <xsl:when test="not($is.id)">
        <xsl:apply-templates select="." mode="print-name"/>
      </xsl:when>
      <xsl:when test="$fast-elements[@id=generate-id()]">
        <xsl:value-of select="$fast-elements[@id=generate-id()]/@part-id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="." mode="print-id-part">
          <xsl:with-param name="is.id" select="$is.id"/>
        </xsl:apply-templates>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <!-- Print the part of a fully qualified name for a single element -->
  <xsl:template match="*" mode="print-id-part">
    <xsl:param name="is.id"/>

    <xsl:variable name="part">
      <xsl:apply-templates select="." mode="print-name"/>
    </xsl:variable>

    <xsl:variable name="unique-name">
      <xsl:apply-templates select="." mode="unique.name"/>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test=
        "$is.id and (
          string-length($part) &gt; $boost.max.id.part.length or
          $unique-name = 0 or
          translate($part, '.&lt;&gt;;\:*?&quot;| ', '') != $part
        )">
        <xsl:variable name="normalized" select="translate(normalize-space(translate($part, '.&lt;&gt;;\:*?&quot;|_', '            ')), ' ', '_')"/>
        <xsl:value-of select =
          "concat(
            substring($normalized, 1, $boost.max.id.part.length - string-length(generate-id(.)) - 1),
            concat('_', generate-id(.)))"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$part"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Override this if an id might not be unique -->
  <xsl:template match="*" mode="unique.name">
    <xsl:value-of select="1"/>
  </xsl:template>

  <xsl:template match="function|overloaded-function" mode="unique.name">
    <xsl:value-of select="number(count(key('named-entities',
        translate(@name, $uppercase-letters, $lowercase-letters))) = 1)"/>
  </xsl:template>

  <!-- Print the name of the current node -->
  <xsl:template match="*" mode="print-name">
    <xsl:value-of select="@name"/>
  </xsl:template>

  <xsl:template match="template-arg" mode="print-name">
    <xsl:if test="position() &gt; 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:value-of select="text()"/>
    <xsl:if test="@pack=1">
      <xsl:text>...</xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template
      match="struct-specialization|class-specialization|union-specialization"
      mode="print-name">
    <xsl:value-of select="@name"/>
    <xsl:text>&lt;</xsl:text>
    <xsl:apply-templates select="specialization/template-arg" mode="print-name"/>
    <xsl:text>&gt;</xsl:text>
  </xsl:template>
  
  <xsl:template name="concat-directives">
    <xsl:param name="directives"/>
    <xsl:for-each select="$directives">
      <xsl:apply-templates select="." mode="print-name"/>
      <xsl:text>::</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="find-nodes-matching-name">
    <!-- The name we are looking for -->
    <xsl:param name="name"/>

    <!-- The context in which this name occurs -->
    <xsl:param name="context"/>

    <!-- The node that we are checking against -->
    <xsl:param name="nodes"/>

    <!-- The set of using directives for this context node -->
    <xsl:variable name="directives"
      select="$context/ancestor::*/using-namespace |
                  $context/ancestor::namespace |
                  $context/ancestor::*/using-class |
                  $context/ancestor::class |
                  $context/ancestor::struct"/>

    <xsl:variable name="directives-str">
      <xsl:call-template name="concat-directives">
        <xsl:with-param name="directives" select="$directives"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:apply-templates select="$nodes" mode="generate-cxx-links">
      <xsl:with-param name="name" select="$name"/>
      <xsl:with-param name="directives-str" select="$directives-str"/>
    </xsl:apply-templates>
  </xsl:template>
  
  <xsl:template match="*" mode="generate-cxx-links">
    <xsl:param name="name"/>
    <xsl:param name="directives-str"/>

    <xsl:variable name="node-name">
      <xsl:call-template name="fully-qualified-name">
        <xsl:with-param name="node" select="."/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="leading-chars"
                  select="string-length($node-name) - string-length($name)"/>

    <!-- Check if this node matches any visible namespace -->
    <xsl:if test="string-length($node-name) &gt;= string-length($name) and
                  substring($node-name, $leading-chars + 1,
                                        string-length($name)) = $name">
      <xsl:variable name="qualifiers"
                    select="substring($node-name, 1, $leading-chars)"/>
      <xsl:if test="contains($directives-str, $qualifiers)">
        <xsl:variable name="myid">
          <xsl:call-template name="generate.id">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:variable>
        <cxx-link-helper>
          <xsl:attribute name="id">
            <xsl:value-of select="$myid"/>
          </xsl:attribute>
          <xsl:attribute name="namespace">
            <xsl:value-of select="$qualifiers"/>
          </xsl:attribute>
          <xsl:text>random text</xsl:text>
        </cxx-link-helper>
      </xsl:if>
    </xsl:if>
  </xsl:template>

  <xsl:template name="cxx-link-name">
    <!-- The actual lookup node -->
    <xsl:param name="lookup"/>

    <!-- The type of name to lookup (e.g., class) -->
    <xsl:param name="type"/>

    <!-- The name we are looking for -->
    <xsl:param name="name"/>

    <!-- The name we will display  -->
    <xsl:param name="display-name"/>

    <!-- The name we are looking for (unqualified)-->
    <xsl:param name="unqualified-name"/>

    <!-- The list of nodes that match the lookup node in both name and type -->
    <xsl:param name="nodes"/>
    
    <!-- Filter the nodes to leave only the ones that are in scope. -->
    <xsl:variable name="matches1">
      <xsl:call-template name="find-nodes-matching-name">
        <xsl:with-param name="name" select="$name"/>
        <xsl:with-param name="nodes" select="$nodes"/>
        <xsl:with-param name="context" select="$lookup"/>
      </xsl:call-template>
    </xsl:variable>
    
    <xsl:variable name="matches" select="exsl:node-set($matches1)//cxx-link-helper"/>

    <xsl:choose>
      <xsl:when test="count($matches) = 0">
        <xsl:message>
          <xsl:text>Cannot find </xsl:text>
          <xsl:value-of select="$type"/>
          <xsl:text> named '</xsl:text>
          <xsl:value-of select="$name"/>
          <xsl:text>'</xsl:text>
        </xsl:message>
        <xsl:value-of select="$display-name"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- If we found more than one, print a message and take the first -->
        <xsl:if test="count($matches) &gt; 1">
          <xsl:message>
            <xsl:text>Reference to </xsl:text>
            <xsl:value-of select="$type"/>
            <xsl:text> '</xsl:text>
            <xsl:value-of select="$name"/>
            <xsl:text>' is ambiguous. Found:</xsl:text>
            <xsl:for-each select="$matches">
              <xsl:text>
              Match in namespace ::</xsl:text>
              <xsl:value-of select="@namespace"/>
            </xsl:for-each>
          </xsl:message>
        </xsl:if>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:value-of select="$matches[position() = 1]/@id"/>
          </xsl:with-param>
          <xsl:with-param name="text" select="$display-name"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
