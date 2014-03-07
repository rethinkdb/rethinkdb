<?xml version="1.0" encoding="utf-8"?>
<!--

Copyright MetaCommunications, Inc. 2003-2004.

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)

-->

<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    xmlns:func="http://exslt.org/functions"
    xmlns:str="http://exslt.org/strings"
    xmlns:meta="http://www.meta-comm.com"
    extension-element-prefixes="func"
    exclude-result-prefixes="str meta exsl"
    version="1.0">

    <func:function name="meta:re_match">
        <xsl:param name="pattern"/>
        <xsl:param name="text"/>
        
        <xsl:choose>
            <xsl:when test="not( contains( $pattern, '*' ) )">
                <func:result select="$text = $pattern"/>
            </xsl:when>
            <xsl:when test="$pattern = '*'">
                <func:result select="1 = 1"/>
            </xsl:when>
            <xsl:when test="substring( $pattern, 1, 1 ) = '*' and substring( $pattern, string-length($pattern), 1 ) = '*' ">
                <func:result select="contains( $text, substring( $pattern, 2, string-length($pattern) - 2 ) ) "/>
            </xsl:when>
            <xsl:when test="substring( $pattern, 1, 1 ) = '*'">
                <xsl:variable name="pattern_tail" select="substring( $pattern, 2, string-length($pattern) - 1 )"/>
                <func:result select="substring( $text, string-length($text) - string-length($pattern_tail) + 1, string-length($pattern_tail) ) = $pattern_tail"/>
            </xsl:when>
            <xsl:when test="substring( $pattern, string-length($pattern), 1 ) = '*' ">
                <xsl:variable name="pattern_head" select="substring( $pattern, 1, string-length($pattern) - 1 )"/>
                <func:result select="starts-with( $text, $pattern_head )"/>
            </xsl:when>
            <xsl:when test="contains( $pattern, '*' ) ">
                <xsl:variable name="pattern_head" select="substring-before( $pattern, '*' )"/>
                <xsl:variable name="pattern_tail" select="substring-after( $pattern, '*' )"/>
                <func:result select="starts-with( $text, $pattern_head ) and substring( $text, string-length($text) - string-length($pattern_tail) + 1, string-length($pattern_tail) ) = $pattern_tail"/>
            </xsl:when>
        </xsl:choose>
    </func:function>

    <xsl:template match='test'>
        <xsl:variable name="result" select="meta:re_match( @pattern, @text )"/>
        <xsl:variable name="expected-result" select="@result = 'true'"/>
        <xsl:if test="$result != $expected-result">
            <failed regex="{@pattern}" text="{@text}" result="{$result}"  expected-result="{$expected-result}"/>
        </xsl:if>
    </xsl:template>

</xsl:stylesheet>
