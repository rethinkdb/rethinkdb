<?xml version="1.0"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!-- Import the HTML chunking stylesheet -->
  
<!-- Watch out that we don't override chunk.xsl -->
<!--
<xsl:import
    href="http://docbook.sourceforge.net/release/xsl/current/html/chunk-common.xsl"/> -->

<!-- Already included in the main stylesheet -->
<!-- <xsl:import href="relative-href.xsl"/> -->

<!-- ==================================================================== -->

<xsl:template match="*" mode="recursive-chunk-filename">
    <xsl:param name="recursive" select="false()"/>

    <xsl:variable name="their">
        <xsl:apply-imports mode="recursive-chunk-filename" select="."/>
    </xsl:variable>

    <xsl:variable name="basename" select="substring-before( $their, $html.ext )"/>
    <xsl:choose>
        <xsl:when test="not($recursive)">
            <!-- translate dots into directory separators, and replace illegal file path characters with underscores -->
            <xsl:value-of select="translate($basename, '.&lt;&gt;\:*?&quot;|,()!+=&amp;', '/_______________' )"/>
            <xsl:value-of select="$html.ext"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:value-of select="$basename"/>
            <xsl:value-of select="'.'"/>
        </xsl:otherwise>
    </xsl:choose>

</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="navig.content">
    <xsl:param name="direction" select="next"/>
    <xsl:variable name="navtext">
        <xsl:choose>
	    <xsl:when test="$direction = 'prev'">
    		<xsl:call-template name="gentext.nav.prev"/>
	    </xsl:when>
	    <xsl:when test="$direction = 'next'">
    		<xsl:call-template name="gentext.nav.next"/>
	    </xsl:when>
	    <xsl:when test="$direction = 'up'">
    		<xsl:call-template name="gentext.nav.up"/>
	    </xsl:when>
	    <xsl:when test="$direction = 'home'">
    		<xsl:call-template name="gentext.nav.home"/>
	    </xsl:when>
	    <xsl:otherwise>
    		<xsl:text>xxx</xsl:text>
	    </xsl:otherwise>
	</xsl:choose>
    </xsl:variable>

    <xsl:choose>
    <xsl:when test="$navtext = 'xxx'">
        <xsl:value-of select="$direction"/>
    </xsl:when>
	<xsl:when test="$navig.graphics != 0">
	    <img>
		<xsl:attribute name="src">
            <xsl:call-template name="href.target.relative">
                <xsl:with-param name="target" select="$navig.graphics.path"/>
            </xsl:call-template>
		    <xsl:value-of select="$direction"/>
		    <xsl:value-of select="$navig.graphics.extension"/>
		</xsl:attribute>
		<xsl:attribute name="alt">
		    <xsl:value-of select="$navtext"/>
		</xsl:attribute>
	    </img>
	</xsl:when>
	<xsl:otherwise>
	    <xsl:value-of select="$navtext"/>
	</xsl:otherwise>
    </xsl:choose>
</xsl:template>


<!-- ====================================================================== -->

<xsl:template match="@fileref">
    <xsl:choose>
        <xsl:when test="contains(., ':')">
            <xsl:value-of select="."/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:call-template name="href.target.relative">
                <xsl:with-param name="target" select="."/>
                <xsl:with-param name="context" select=".."/>
            </xsl:call-template>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>


<xsl:template match="@url">
    <xsl:choose>
        <xsl:when test="contains(., ':')">
            <xsl:value-of select="."/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>XXX</xsl:text>
            <xsl:value-of select="."/>
            <xsl:text>XXX</xsl:text>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>


</xsl:stylesheet>
