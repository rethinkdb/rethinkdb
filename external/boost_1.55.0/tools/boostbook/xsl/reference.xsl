<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:include href="annotation.xsl"/>
  <xsl:include href="template.xsl"/>
  <xsl:include href="function.xsl"/>
  <xsl:include href="type.xsl"/>
  <xsl:include href="source-highlight.xsl"/>
  <xsl:include href="utility.xsl"/>
  <xsl:include href="lookup.xsl"/>
  <xsl:include href="library.xsl"/>
  <xsl:include href="index.xsl"/>
  <xsl:include href="error.xsl"/>
  <xsl:include href="macro.xsl"/>
  <xsl:include href="testing/testsuite.xsl"/>
  <xsl:include href="caramel/concept2docbook.xsl"/>

  <xsl:template name="namespace-synopsis">
    <xsl:param name="indentation" select="0"/>
    <!-- Open namespace-->
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text" select="concat('namespace ',@name, ' {')"/>
    </xsl:call-template>

    <!-- Emit namespace types -->
    <xsl:apply-templates select="class|class-specialization|
                                 struct|struct-specialization|
                                 union|union-specialization|
                                 typedef|enum|data-member" mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>

    <!-- Emit namespace functions -->
    <xsl:apply-templates
      select="free-function-group|function|overloaded-function"
      mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>

    <!-- Emit namespaces -->
    <xsl:apply-templates select="namespace" mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>

    <!-- Close namespace -->
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="highlight-text">
      <xsl:with-param name="text" select="'}'"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Emit namespace synopsis -->
  <xsl:template match="namespace" mode="synopsis">
    <xsl:param name="indentation" select="0"/>

    <xsl:choose>
      <xsl:when test="count(ancestor::namespace)=0">
        <xsl:call-template name="namespace-synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="namespace-synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Emit namespace reference -->
  <xsl:template match="namespace" mode="reference">
    <xsl:apply-templates select="namespace|free-function-group"
      mode="reference">
      <xsl:with-param name="indentation" select="0"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="class|class-specialization|
                                 struct|struct-specialization|
                                 union|union-specialization|enum|function|
                                 overloaded-function|data-member|typedef"
      mode="namespace-reference"/>
  </xsl:template>

  <!-- Eat extra documentation when in the synopsis or reference sections -->
  <xsl:template match="para|section" mode="synopsis"/>
  <xsl:template match="para|section" mode="reference"/>

  <xsl:template name="find-wrap-point">
    <xsl:param name="text"/>
    <xsl:param name="prefix"/>
    <xsl:param name="result" select="0"/>
    <xsl:param name="default" select="$max-columns - string-length($prefix)"/>
    <xsl:variable name="s" select="substring($text, 2)"/>
    <xsl:variable name="candidate">
      <xsl:choose>
        <xsl:when test="contains($s, ' ')">
          <xsl:value-of select="string-length(substring-before($s, ' ')) + 1"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string-length($text)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="string-length($prefix) + $result + $candidate &lt;= $max-columns">
        <xsl:call-template name="find-wrap-point">
          <xsl:with-param name="text" select="substring($text, $candidate + 1)"/>
          <xsl:with-param name="prefix" select="$prefix"/>
          <xsl:with-param name="result" select="$result + $candidate"/>
          <xsl:with-param name="default" select="$result + $candidate"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$default"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <xsl:template name="wrap-comment">
    <xsl:param name="prefix"/>
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="string-length($prefix) + string-length($text) &lt;= $max-columns">
        <xsl:value-of select="$text"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="size">
          <xsl:call-template name="find-wrap-point">
            <xsl:with-param name="prefix" select="$prefix"/>
            <xsl:with-param name="text" select="$text"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="substring($text, 1, $size)"/>
        <xsl:text>&#10;</xsl:text>
        <xsl:value-of select="$prefix"/>
        <xsl:call-template name="wrap-comment">
          <xsl:with-param name="prefix" select="$prefix"/>
          <xsl:with-param name="text" select="normalize-space(substring($text, $size + 1))"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Comment mode tries to wipe out any extra spacing in the output -->
  <xsl:template match="purpose" mode="comment">
    <xsl:param name="wrap" select="false()"/>
    <xsl:param name="prefix"/>
    <xsl:apply-templates mode="comment">
      <xsl:with-param name="wrap"
                      select="$wrap and count(text()|*) = 1"/>
      <xsl:with-param name="prefix" select="$prefix"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="simpara|para" mode="comment">
    <xsl:param name="wrap" select="false()"/>
    <xsl:param name="prefix"/>
    <xsl:apply-templates select="text()|*" mode="comment">
      <xsl:with-param name="wrap"
                      select="$wrap and count(text()|*) = 1"/>
      <xsl:with-param name="prefix" select="$prefix"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="text()" mode="comment">
    <xsl:param name="wrap" select="false()"/>
    <xsl:param name="prefix"/>
    <xsl:variable name="stripped" select="normalize-space(.)"/>
    <xsl:choose>
      <xsl:when test="$wrap">
        <xsl:call-template name="wrap-comment">
          <xsl:with-param name="prefix" select="$prefix"/>
          <xsl:with-param name="text" select="$stripped"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="."/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="*" mode="comment">
    <xsl:apply-templates select="." mode="annotation"/>
  </xsl:template>
</xsl:stylesheet>
