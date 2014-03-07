<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <!-- Indent the current line-->
  <xsl:template name="indent">
    <xsl:param name="indentation"/>
    <xsl:if test="$indentation > 0">
      <xsl:text> </xsl:text>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation - 1"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!--   get name of first ancestor-or-self which is a class, struct or union -->
  <xsl:template name="object-name">
    <xsl:variable name="ancestors" select="ancestor-or-self::class |
      ancestor-or-self::class-specialization |
      ancestor-or-self::struct |
      ancestor-or-self::struct-specialization |
      ancestor-or-self::union |
      ancestor-or-self::union-specialization"/>
    <xsl:value-of select="$ancestors[last()]/@name"/>
  </xsl:template>

  <!--   get name of access specification that we are inside -->
  <xsl:template name="access-name">
    <xsl:variable name="ancestors" select="ancestor-or-self::access |
      ancestor-or-self::class |
      ancestor-or-self::class-specialization |
      ancestor-or-self::struct |
      ancestor-or-self::struct-specialization |
      ancestor-or-self::union |
      ancestor-or-self::union-specialization"/>
    <xsl:choose>
      <xsl:when test="name($ancestors[last()])='access'">
        <xsl:value-of select="$ancestors[last()]/@name"/>
      </xsl:when>
      <xsl:otherwise>
        public
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
