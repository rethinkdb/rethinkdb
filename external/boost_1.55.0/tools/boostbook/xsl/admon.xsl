<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/admon.xsl"/>

<!-- Already included in the main style sheet -->
<!-- <xsl:import href="relative-href.xsl"/> -->

<xsl:template name="admon.graphic">
    <xsl:param name="node" select="."/>

    <xsl:call-template name="href.target.relative">
        <xsl:with-param name="target" select="$admon.graphics.path"/>
    </xsl:call-template>

    <xsl:choose>
        <xsl:when test="local-name($node)='note'">note</xsl:when>
        <xsl:when test="local-name($node)='warning'">warning</xsl:when>
        <xsl:when test="local-name($node)='caution'">caution</xsl:when>
        <xsl:when test="local-name($node)='tip'">tip</xsl:when>
        <xsl:when test="local-name($node)='important'">important</xsl:when>
        <xsl:otherwise>note</xsl:otherwise>
    </xsl:choose>

    <xsl:value-of select="$admon.graphics.extension"/>
</xsl:template>

</xsl:stylesheet>
