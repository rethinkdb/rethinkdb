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
    xmlns:set="http://exslt.org/sets"
    xmlns:meta="http://www.meta-comm.com"
    extension-element-prefixes="func exsl"
    exclude-result-prefixes="set str meta"
    version="1.0">

    <xsl:import href="common.xsl"/>

    <xsl:output method="html" 
        doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" 
        encoding="utf-8" 
        indent="yes"
        />

    <xsl:param name="source"/>
    <xsl:param name="run_date"/>
    <xsl:param name="comment_file"/>
    <xsl:param name="explicit_markup_file"/>

    <xsl:variable name="explicit_markup" select="document( $explicit_markup_file )"/>

    <xsl:template match="test-log[ meta:show_output( $explicit_markup, . ) ]">
        <xsl:variable name="document_path" select="meta:output_file_path( @target-directory )"/>

        <xsl:message>Writing log file document <xsl:value-of select="$document_path"/></xsl:message>

        <exsl:document href="{$document_path}" 
        method="html" 
        doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
        encoding="utf-8"
        indent="yes">

        <html>
            <xsl:variable name="component">
            <xsl:choose>
                <xsl:when test="@test-name != ''">
                <div class="log-test-title">
                    <xsl:value-of select="concat( @library, ' - ', @test-name, ' / ', @toolset )"/>
                </div>
                </xsl:when>
                <xsl:otherwise>
                <xsl:value-of select="@target-dir"/>
                </xsl:otherwise>
            </xsl:choose>
            </xsl:variable>
            
            <head>
            <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
            <title>Boost regression - test run output: <xsl:value-of select="$component"/></title>
            </head>

            <body>
            <div>
                <div class="log-test-title">
                Boost regression - test run output: <xsl:value-of select="$component"/>
            </div>
            

            <xsl:if test="notes/note">
                <p>
                <div class="notes-title">Notes</div>
                <xsl:call-template name="show_notes">
                    <xsl:with-param name="notes" select="notes/note"/>
                    <xsl:with-param name="explicit_markup" select="$explicit_markup"/>
                </xsl:call-template>
                </p>
            </xsl:if>
              
            <xsl:if test="compile">
                <p>
                <div class="log-compiler-output-title">Compiler output:</div>
                <pre>
                    <xsl:copy-of select="compile/node()"/>
                </pre>
                </p>
            </xsl:if>
              
            <xsl:if test="link">
                <p>
                <div class="log-linker-output-title">Linker output:</div>
                <pre>
                    <xsl:copy-of select="link/node()"/>
                </pre>
                </p>
            </xsl:if>

            <xsl:if test="lib">
                <p>
                <div class="log-linker-output-title">Lib output:</div>
                <p>
                    See <a href="{meta:encode_path( lib/node() )}.html">
                    <xsl:copy-of select="lib/node()"/>
                    </a>
                </p>
                </p>
            </xsl:if>
              
            <xsl:if test="run">
                <p>
                <div class="log-run-output-title">Run output:</div>
                <pre>
                    <xsl:copy-of select="run/node()"/>
                </pre>
                </p>
            </xsl:if>
              
            </div>

            <xsl:copy-of select="document( 'html/make_tinyurl.html' )"/>

            </body>
        
        </html>
        </exsl:document>  

    </xsl:template>

</xsl:stylesheet>
