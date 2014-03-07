<?xml version="1.0" encoding="utf-8"?>
<!--

Copyright MetaCommunications, Inc. 2003-2004.

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)

-->

<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns:meta="http://www.meta-comm.com"
    exclude-result-prefixes="meta"
    version="1.0">

    <xsl:import href="common.xsl"/>
  
    <xsl:output method="xml" encoding="utf-8"/>
      
    <xsl:param name="expected_results_file"/>
    <xsl:param name="failures_markup_file"/>
    <xsl:variable name="expected_results" select="document( $expected_results_file )" />
    <xsl:variable name="failures_markup" select="document( $failures_markup_file )" />

    <xsl:template match="/">
        <xsl:apply-templates/>
    </xsl:template>
      
    <xsl:template match="test-log">
        <xsl:variable name="library" select="@library"/>
        <xsl:variable name="test-name" select="@test-name"/>
        <xsl:variable name="toolset" select="@toolset"/>

        <xsl:element name="{local-name()}">
        <xsl:apply-templates select="@*"/>

        <xsl:variable name="actual_result">
            <xsl:choose>
            <!-- Hack: needs to be researched (and removed). See M.Wille's incident. -->
            <xsl:when test="run/@result='succeed' and lib/@result='fail'">
                <xsl:text>success</xsl:text>
            </xsl:when>
            <xsl:when test="./*/@result = 'fail'" >
                <xsl:text>fail</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>success</xsl:text>
            </xsl:otherwise>
            </xsl:choose>                     
        </xsl:variable>

        <xsl:variable name="expected_results_test_case" select="$expected_results//*/test-result[ @library=$library and ( @test-name=$test-name or @test-name='*' ) and @toolset = $toolset]"/>
        <xsl:variable name="new_failures_markup" select="$failures_markup//library[@name=$library]/mark-expected-failures[ meta:re_match( test/@name, $test-name ) and meta:re_match( toolset/@name, $toolset ) ]"/>
        <xsl:variable name="failures_markup" select="$failures_markup//library[@name=$library]/test[ meta:re_match( @name, $test-name ) ]/mark-failure[ meta:re_match( toolset/@name, $toolset ) ]"/>
        <xsl:variable name="is_new">
            <xsl:choose>
                <xsl:when test="$expected_results_test_case">
                <xsl:text>no</xsl:text>
                </xsl:when>
                <xsl:otherwise>yes</xsl:otherwise>
            </xsl:choose>
        </xsl:variable>

        <xsl:variable name="expected_result">
            <xsl:choose>
            <xsl:when test='count( $failures_markup ) &gt; 0 or count( $new_failures_markup ) &gt; 0'>
                <xsl:text>fail</xsl:text>
            </xsl:when>
              
            <xsl:otherwise>
                <xsl:choose>
                <xsl:when test="$expected_results_test_case and $expected_results_test_case/@result = 'fail'">
                    <xsl:text>fail</xsl:text>
                </xsl:when>
                  
                <xsl:otherwise>success</xsl:otherwise>
                </xsl:choose>
            </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>

        <xsl:variable name="status">
            <xsl:choose>
            <xsl:when test="count( $failures_markup ) &gt; 0 or count( $new_failures_markup ) &gt; 0">
                <xsl:choose>
                <xsl:when test="$expected_result = $actual_result">expected</xsl:when>
                <xsl:otherwise>unexpected</xsl:otherwise>
                </xsl:choose>
            </xsl:when>

            <xsl:otherwise>
                <xsl:choose>
                <xsl:when test="$expected_result = $actual_result">expected</xsl:when>
                <xsl:otherwise>unexpected</xsl:otherwise>
                </xsl:choose>
            </xsl:otherwise>
              
            </xsl:choose>
        </xsl:variable>

        <xsl:variable name="notes">
            <xsl:choose>

            <xsl:when test='count( $failures_markup ) &gt; 0'>
                <xsl:for-each select="$failures_markup/note">
                <xsl:copy-of select="."/>
                </xsl:for-each>
            </xsl:when>

            <xsl:when test='count( $new_failures_markup ) &gt; 0'>
                <xsl:for-each select="$new_failures_markup/note">
                <xsl:copy-of select="."/>
                </xsl:for-each>
            </xsl:when>
              
            </xsl:choose>
        </xsl:variable>

        <xsl:attribute name="result"><xsl:value-of select="$actual_result"/></xsl:attribute>
        <xsl:attribute name="expected-result"><xsl:value-of select="$expected_result"/></xsl:attribute>
        <xsl:attribute name="status"><xsl:value-of select="$status"/></xsl:attribute>
        <xsl:attribute name="is-new"><xsl:value-of select="$is_new"/></xsl:attribute>
        <!--<a><xsl:value-of select="count( $failures_markup )"/></a>-->
        <xsl:element name="notes"><xsl:copy-of select="$notes"/></xsl:element>


        <xsl:apply-templates select="node()" />
        </xsl:element>
    </xsl:template>

    <xsl:template match="*">
        <xsl:element name="{local-name()}">
        <xsl:apply-templates select="@*"/>
        <xsl:apply-templates select="node()" />
        </xsl:element>
    </xsl:template>

    <xsl:template match="@*">
        <xsl:copy-of select="." />
    </xsl:template>

</xsl:stylesheet>
