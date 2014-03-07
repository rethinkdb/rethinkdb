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
    version="1.0">

    <xsl:variable name="output_directory" select="'output'"/>

    <xsl:template name="get_toolsets">
    <xsl:param name="toolsets"/>
    <xsl:param name="required-toolsets"/>

    <xsl:variable name="toolset_output">
        <xsl:for-each select="$toolsets">
        <xsl:variable name="toolset" select="."/>
        <xsl:element name="toolset">
            <xsl:attribute name="toolset"><xsl:value-of select="$toolset"/></xsl:attribute>
            <xsl:choose>
            <xsl:when test="$required_toolsets[ $toolset = @name ]">
                <xsl:attribute name="required">yes</xsl:attribute>
                <xsl:attribute name="sort">a</xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
                <xsl:attribute name="required">no</xsl:attribute>
                <xsl:attribute name="sort">z</xsl:attribute>
            </xsl:otherwise>
            </xsl:choose>
        </xsl:element>
        </xsl:for-each>
    </xsl:variable>

    <xsl:for-each select="exsl:node-set( $toolset_output )/toolset">
        <xsl:sort select="concat( @sort, ' ', @toolset)" order="ascending"/>
        <xsl:copy-of select="."/>
    </xsl:for-each>

    </xsl:template>

  <func:function name="meta:show_output">
      <xsl:param name="explicit_markup"/>     
      <xsl:param name="test_log"/>     
      <func:result select="$test_log/@result != 'success' and not( meta:is_unusable( $explicit_markup, $test_log/@library, $test_log/@toolset )) or $test_log/@show-run-output = 'true'"/>
  </func:function>

    <func:function name="meta:is_test_log_a_test_case">
        <xsl:param name="test_log"/>      
        <func:result select="$test_log/@test-type='compile' or $test_log/@test-type='compile_fail' or $test_log/@test-type='run' or $test_log/@test-type='run_pyd'"/>
    </func:function>

    <func:function name="meta:is_unusable">
        <xsl:param name="explicit_markup"/>
        <xsl:param name="library"/>
        <xsl:param name="toolset"/>
        
        <func:result select="$explicit_markup//library[ @name = $library ]/mark-unusable[ toolset/@name = $toolset or toolset/@name='*' ]"/>
    </func:function>

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
                <xsl:variable name="pattern_head" select="substring( $pattern, 1, string-length($pattern) - 2 )"/>
                <func:result select="substring( $text, 1, string-length($pattern_head) ) = $pattern_head "/>
            </xsl:when>
        </xsl:choose>
    </func:function>

    <func:function name="meta:encode_path">
        <xsl:param name="path"/>
        <func:result select="translate( translate( $path, '/', '-' ), './', '-' )"/>
    </func:function>

    <func:function name="meta:toolset_name">
        <xsl:param name="name"/>
        <func:result select="$name"/>
    </func:function>

    <func:function name="meta:output_file_path">
        <xsl:param name="path"/>
        <func:result select="concat( $output_directory, '/', meta:encode_path( $path ), '.html' )"/>
    </func:function>

    <xsl:template name="show_notes">
        <xsl:param name="explicit_markup"/>
        <xsl:param name="notes"/>
        <div class="notes">
            <xsl:for-each select="$notes">
            <div>
                <xsl:variable name="refid" select="@refid"/>
                <xsl:call-template name="show_note">
                    <xsl:with-param name="note" select="."/>
                    <xsl:with-param name="reference" select="$explicit_markup//note[ $refid = @id ]"/>
                </xsl:call-template>
            </div>
            </xsl:for-each>
        </div>
    </xsl:template>

    <xsl:template name="show_note">
        <xsl:param name="note"/>
        <xsl:param name="reference"/>
        <div class="note">
            <xsl:variable name="author">
                <xsl:choose>
                    <xsl:when test="$note/@author">
                        <xsl:value-of select="$note/@author"/>
                    </xsl:when>
                    <xsl:when test="$reference">
                        <xsl:value-of select="$reference/@author"/>                               
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text/>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:variable>

            <xsl:variable name="date">
                <xsl:choose>
                    <xsl:when test="$note/@date">
                        <xsl:value-of select="$note/@date"/>
                    </xsl:when>
                    <xsl:when test="$reference">
                        <xsl:value-of select="$reference/@date"/>                      
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text/>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:variable>

        <span class="note-header">
            <xsl:choose>
                <xsl:when test="$author != '' and $date != ''">
                    [&#160;<xsl:value-of select="$author"/>&#160;<xsl:value-of select="$date"/>&#160;]
                </xsl:when>
                <xsl:when test="$author != ''">
                    [&#160;<xsl:value-of select="$author"/>&#160;]                        
                </xsl:when>
                <xsl:when test="$date != ''">
                    [&#160;<xsl:value-of select="$date"/>&#160;]                        
                </xsl:when>
            </xsl:choose>
        </span>

        <xsl:if test="$reference">
            <xsl:copy-of select="$reference/node()"/>                
        </xsl:if>
        <xsl:copy-of select="$note/node()"/>      

        </div>
    </xsl:template>

</xsl:stylesheet>
