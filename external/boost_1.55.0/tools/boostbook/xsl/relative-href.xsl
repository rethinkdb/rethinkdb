<?xml version="1.0"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/lib/lib.xsl"/>

<!-- ==================================================================== -->

<!-- Check if path is absolute

    Not attempting to fully parse or validate absolute vs. relative URI.
    Assumes an absolute url when $target matches the regex '^[a-zA-Z.+-]*:'

    According to RFC1808, however, the colon (':') may also appear in a relative
    URL. To workaround this limitation for relative links containing colons one
    may use the alternatives below, instead.

    For the relative URI this:that use ./this:that or this%3Athat, instead.
-->
<xsl:template name="is.absolute.uri">
    <xsl:param name="uri"/>

    <xsl:variable name="scheme1" select="substring-before($uri, ':')"/>
    <xsl:variable name="scheme2" select="translate($scheme1, 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-.', '')"/>

    <xsl:choose>
        <xsl:when test="$scheme1 and not($scheme2)">1</xsl:when>
        <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template name="href.target.relative">
    <xsl:param name="target"/>
    <xsl:param name="context" select="."/>

    <xsl:variable name="isabsoluteuri">
        <xsl:call-template name="is.absolute.uri">
            <xsl:with-param name="uri" select="$target"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
        <xsl:when test="$isabsoluteuri='1'">
            <xsl:value-of select="$target"/>
        </xsl:when>

        <xsl:otherwise>
            <xsl:variable name="href.to.uri" select="$target"/>
            <xsl:variable name="href.from.uri">
                <xsl:call-template name="href.target.uri">
                    <xsl:with-param name="object" select="$context"/>
                </xsl:call-template>
            </xsl:variable>

            <xsl:variable name="href.to">
                <xsl:call-template name="trim.common.uri.paths">
                    <xsl:with-param name="uriA" select="$href.to.uri"/>
                    <xsl:with-param name="uriB" select="$href.from.uri"/>
                    <xsl:with-param name="return" select="'A'"/>
                </xsl:call-template>
            </xsl:variable>

            <xsl:variable name="href.from">
                <xsl:call-template name="trim.common.uri.paths">
                    <xsl:with-param name="uriA" select="$href.to.uri"/>
                    <xsl:with-param name="uriB" select="$href.from.uri"/>
                    <xsl:with-param name="return" select="'B'"/>
                </xsl:call-template>
            </xsl:variable>

            <xsl:variable name="depth">
                <xsl:call-template name="count.uri.path.depth">
                    <xsl:with-param name="filename" select="$href.from"/>
                </xsl:call-template>
            </xsl:variable>

            <xsl:variable name="href">
                <xsl:call-template name="copy-string">
                    <xsl:with-param name="string" select="'../'"/>
                    <xsl:with-param name="count" select="$depth"/>
                </xsl:call-template>
                <xsl:value-of select="$href.to"/>
            </xsl:variable>

            <xsl:value-of select="$href"/>
        </xsl:otherwise>
    </xsl:choose>

</xsl:template>

</xsl:stylesheet>
