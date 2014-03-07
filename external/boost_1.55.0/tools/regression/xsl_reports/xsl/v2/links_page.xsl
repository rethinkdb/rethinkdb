<?xml version="1.0" encoding="utf-8"?>
<!--

Copyright MetaCommunications, Inc. 2003-2006.

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)

-->

<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns:str="http://exslt.org/strings"
    xmlns:set="http://exslt.org/sets"
    xmlns:exsl="http://exslt.org/common"
    xmlns:func="http://exslt.org/functions"
    xmlns:meta="http://www.meta-comm.com"
    extension-element-prefixes="func exsl str set"
    exclude-result-prefixes="meta"
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
    <xsl:variable name="runner_id" select="test-run/@runner"/>
    <xsl:variable name="revision" select="test-run/@revision"/>
    <xsl:variable name="timestamp" select="test-run/@timestamp"/>

    <!-- runs / toolsets -->
    <xsl:variable name="run_toolsets" select="meta:test_structure( /, 'no' )"/>

    <!-- libraries -->
    <xsl:variable name="test_case_logs" select="//test-log[ meta:is_test_log_a_test_case(.) ]"/>
    <xsl:variable name="libraries" select="set:distinct( $test_case_logs/@library )"/>

    <xsl:variable name="unusables_f">
            <xsl:for-each select="set:distinct( $run_toolsets//toolset/@name )">
                <xsl:variable name="toolset" select="."/>
                <xsl:for-each select="$libraries">
                    <xsl:variable name="library" select="."/>
                    <xsl:if test="meta:is_unusable_( $explicit_markup, $library, $toolset )">
                        <unusable library-name="{$library}" toolset-name="{$toolset}"/>                            
                    </xsl:if>
                </xsl:for-each>
            </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="unusables" select="exsl:node-set( $unusables_f )"/>

        
    <xsl:key 
        name="library-name_toolset-name_key" 
        match="unusable" 
        use="concat( @library-name, '&gt;@&lt;', @toolset-name )"/>


    <!-- 
         Build a  tree with the following structure:

         lib -> test -> toolsets -> test-log
         -->

    <xsl:template match="/">
        <xsl:variable name="test_logs_to_show" select="//test-log"/>
        <xsl:variable name="libs_test_test_log_tree" select="meta:restructure_logs( $test_logs_to_show )"/>
        
        <exsl:document href="debug.xml"
            method="xml" 
            encoding="utf-8"
            indent="yes">
            <debug>
                <xsl:copy-of select="$libs_test_test_log_tree"/>
            </debug>
        </exsl:document>

        <xsl:for-each select="$libs_test_test_log_tree//toolset">
            <xsl:variable name="toolset" select="."/>
            <xsl:variable name="library_name" select="$toolset/../../@name"/>
            <xsl:variable name="test_name" select="$toolset/../@name"/>
            <xsl:variable name="toolset_name" select="$toolset/@name"/>
            <xsl:message>Processing test "<xsl:value-of select="$runner_id"/>/<xsl:value-of select="$library_name"/>/<xsl:value-of select="$test_name"/>/<xsl:value-of select="$toolset_name"/>"</xsl:message>

            <xsl:if test="count( $toolset/* ) &gt; 1">
                <xsl:message>  Processing variants</xsl:message>

                <xsl:variable name="variants_file_path" select="meta:output_file_path( concat( $runner_id, '-', $library_name, '-', $toolset_name, '-', $test_name, '-variants' ) )"/>

                <xsl:call-template name="write_variants_file">
                    <xsl:with-param name="path" select="$variants_file_path"/>
                    <xsl:with-param name="test_logs" select="$toolset/*"/>
                    <xsl:with-param name="runner_id" select="$runner_id"/>
                    <xsl:with-param name="revision" select="$revision"/>
                    <xsl:with-param name="timestamp" select="$timestamp"/>
                </xsl:call-template>

                <xsl:for-each select="str:tokenize( string( ' |_release' ), '|')">
                    <xsl:variable name="release_postfix" select="translate(.,' ','')"/>
                    <xsl:for-each select="str:tokenize( string( 'developer|user' ), '|')">
                        <xsl:variable name="directory" select="."/>
                        <xsl:variable name="variants__file_path" select="concat( $directory, '/', meta:encode_path( concat( $runner_id, '-', $library_name, '-', $toolset_name, '-', $test_name, '-variants_', $release_postfix ) ), '.html' )"/>

                        <xsl:call-template name="write_variants_reference_file">
                            <xsl:with-param name="path" select="$variants__file_path"/>
                            <xsl:with-param name="variants_file_path" select="concat( '../', $variants_file_path )"/>
                            <xsl:with-param name="release_postfix" select="$release_postfix"/>
                        </xsl:call-template>
                    </xsl:for-each>
                </xsl:for-each>
            </xsl:if>

            <xsl:for-each select="./test-log">
                <xsl:message>  Processing test-log</xsl:message>
                <xsl:variable name="test_log" select="."/>

                <xsl:if test="meta:show_output( $explicit_markup, $test_log )">
                    <xsl:variable name="log_file_path" select="meta:log_file_path( ., $runner_id )"/>
                    
                    <xsl:call-template name="write_test_result_file">
                        <xsl:with-param name="path" select="$log_file_path"/>
                        <xsl:with-param name="test_log" select="$test_log"/>
                        <xsl:with-param name="runner_id" select="$runner_id"/>
                        <xsl:with-param name="revision" select="$revision"/>
                        <xsl:with-param name="timestamp" select="$timestamp"/>
                    </xsl:call-template>
                    
                    <xsl:for-each select="str:tokenize( string( ' |_release' ), '|')">
                        <xsl:variable name="release_postfix" select="translate(.,' ','')"/>
                        <xsl:for-each select="str:tokenize( string( 'developer|user' ), '|')">
                            <xsl:variable name="directory" select="."/>

                            <xsl:variable name="reference_file_path" select="concat( $directory, '/', meta:log_file_path( $test_log, $runner_id, $release_postfix ) )"/>
                            <xsl:call-template name="write_test_results_reference_file">
                                <xsl:with-param name="path" select="$reference_file_path"/>
                                <xsl:with-param name="log_file_path" select="$log_file_path"/>
                            </xsl:call-template>
                        </xsl:for-each>                
                    </xsl:for-each>
                </xsl:if>

            </xsl:for-each>
        </xsl:for-each>
    </xsl:template>

    <func:function name="meta:restructure_logs">
        <xsl:param name="test_logs"/>
        <xsl:variable name="libs" select="set:distinct( $test_logs/@library )"/>
        <xsl:variable name="fragment">
            <runner runner_id="{$test_logs[1]/../@runner}" revision="{$test_logs[1]/../@revision}" timestamp="{$test_logs[1]/../@timestamp}">
                <xsl:for-each select="$libs">
                    <xsl:variable name="library_name" select="."/>
                    <xsl:variable name="library_test_logs" select="$test_logs[@library=$library_name]"/>
                    <library name="{$library_name}">
                        <xsl:variable name="tests" select="set:distinct( $library_test_logs/@test-name )"/>
                        <xsl:for-each select="$tests">
                            <xsl:variable name="test_name" select="."/>
                            <xsl:variable name="test_test_logs" select="$library_test_logs[@test-name=$test_name]"/>
                            <test name="{$test_name}"  >
                                <xsl:variable name="toolsets" select="set:distinct( $test_test_logs/@toolset )"/>
                                <xsl:for-each select="$toolsets">
                                    <xsl:variable name="toolset" select="."/>
                                    <xsl:variable name="toolset_test_logs" select="$test_test_logs[@toolset=$toolset]"/>
                                    <toolset name="{$toolset}">
                                        <xsl:copy-of select="$toolset_test_logs"/>
                                    </toolset>
                                </xsl:for-each>
                            </test>
                        </xsl:for-each>
                    </library>
                </xsl:for-each>
            </runner>
        </xsl:variable>
        <func:result select="exsl:node-set( $fragment )"/>
    </func:function>

    <xsl:template name="write_variants_reference_file">
        <xsl:param name="path"/>
        <xsl:param name="variants_file_path"/>
        <xsl:param name="release_postfix"/>

        <xsl:message>    Writing variants reference file <xsl:value-of select="$path"/></xsl:message>
        <exsl:document href="{$path}"
            method="html" 
            doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
            encoding="utf-8"
            indent="yes">
            <html>
                <head>
                    <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
                    <!--                    <title>Boost regression: <xsl:value-of select="$library_name"/>/<xsl:value-of select="$source"/></title>-->
                </head>
                <frameset cols="190px,*" frameborder="0" framespacing="0" border="0">
                    <frame name="tocframe" src="toc{$release_postfix}.html" scrolling="auto"/>
                    <frame name="docframe" src="{$variants_file_path}" scrolling="auto"/>
                </frameset>
            </html>
        </exsl:document>  
        
    </xsl:template>

    <func:function name="meta:output_page_header">
        <xsl:param name="test_log"/>
        <xsl:param name="runner_id"/>
        <xsl:choose>
            <xsl:when test="$test_log/@test-name != ''">
                <func:result select="concat( $runner_id, ' - ', $test_log/@library, ' - ', $test_log/@test-name, ' / ', $test_log/@toolset )"/>
            </xsl:when>
            <xsl:otherwise>
                <func:result select="$test_log/@target-dir"/>
            </xsl:otherwise>
        </xsl:choose>
    </func:function>


    <xsl:template name="write_variants_file">
        <xsl:param name="path"/>
        <xsl:param name="test_logs"/>
        <xsl:param name="runner_id"/>
        <xsl:param name="revision"/>
        <xsl:param name="timestamp"/>
        <xsl:message>    Writing variants file <xsl:value-of select="$path"/></xsl:message>
        <exsl:document href="{$path}"
            method="html" 
            doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
            encoding="utf-8"
            indent="yes">
            
            <html>
                <xsl:variable name="component" select="meta:output_page_header( $test_logs[1], $runner_id )"/>
                <xsl:variable name="age" select="meta:timestamp_difference( $timestamp, $run_date )"/>

                <head>
                    <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
                    <title>Test output: <xsl:value-of select="$component"/></title>
                </head>

                <body>
                    <div class="log-test-header">
                        <div class="log-test-title">
                            Test output: <xsl:value-of select="$component"/>
                        </div>
                        <div><span class="timestamp-{$age}">
                            Rev <xsl:value-of select="$revision"/> /
                            <xsl:value-of select="meta:format_timestamp( $timestamp )"/>
                        </span></div>
                    </div>

                    <div>
                        <b>Report Time: </b> <xsl:value-of select="meta:format_timestamp( $run_date )"/>
                    </div>

                    <p>Output by test variants:</p>
                    <table>
                        <xsl:for-each select="$test_logs">
                            <tr>
                                <td>
                                     <xsl:choose>
                                         <xsl:when test="meta:log_file_path(.,$runner_id) != ''">
                                             <a href="../{meta:log_file_path(.,$runner_id)}">
                                                 <xsl:value-of select="@target-directory"/>
                                             </a>
                                         </xsl:when>
                                         <xsl:otherwise>
                                             <xsl:value-of select="@target-directory"/>
                                         </xsl:otherwise>
                                     </xsl:choose>
                                 </td>
                             </tr>
                         </xsl:for-each>
                     </table>
                 </body>
             </html>
         </exsl:document>           
    </xsl:template>

    <xsl:template name="write_test_result_file">
        <xsl:param name="path"/>
        <xsl:param name="test_log"/>
        <xsl:param name="runner_id"/>
        <xsl:param name="revision"/>
        <xsl:param name="timestamp"/>
        <xsl:message>    Writing log file document <xsl:value-of select="$path"/></xsl:message>

        <exsl:document href="{$path}" 
            method="html" 
            doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
            encoding="utf-8"
            indent="yes">
                        
            <html>
                <xsl:variable name="component" select="meta:output_page_header( $test_log, $runner_id )"/>
                <xsl:variable name="age" select="meta:timestamp_difference( $timestamp, $run_date )"/>

                <head>
                    <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
                    <title>Test output: <xsl:value-of select="$component"/></title>
                </head>
                
                <body>
                    <div class="log-test-header">
                        <div class="log-test-title">
                            Test output: <xsl:value-of select="$component"/>
                        </div>
                        <div><span class="timestamp-{$age}">
                            Rev <xsl:value-of select="$revision"/> /
                            <xsl:value-of select="meta:format_timestamp( $timestamp )"/>
                        </span></div>
                    </div>

                    <div>
                        <b>Report Time: </b> <xsl:value-of select="meta:format_timestamp( $run_date )"/>
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
                            <div class="log-compiler-output-title">Compile [<xsl:value-of select="compile/@timestamp"/>]: <span class="output-{compile/@result}"><xsl:value-of select="compile/@result"/></span></div>
                            <pre><xsl:copy-of select="compile/node()"/></pre>
                        </p>
                    </xsl:if>
                
                    <xsl:if test="link">
                        <p>
                            <div class="log-linker-output-title">Link [<xsl:value-of select="link/@timestamp"/>]: <span class="output-{link/@result}"><xsl:value-of select="link/@result"/></span></div>
                            <pre><xsl:copy-of select="link/node()"/></pre>
                        </p>
                    </xsl:if>
                            
                    <xsl:if test="lib">
                        <p>
                            <div class="log-linker-output-title">Lib [<xsl:value-of select="lib/@timestamp"/>]: <span class="output-{lib/@result}"><xsl:value-of select="lib/@result"/></span></div>
                            <p>
                                See <a href="{meta:encode_path( concat( $runner_id, '-',  lib/node() )  ) }.html">
                                <xsl:copy-of select="lib/node()"/>
                                </a>
                            </p>
                        </p>
                    </xsl:if>
                        
                    <xsl:if test="run">
                        <p>
                            <div class="log-run-output-title">Run [<xsl:value-of select="run/@timestamp"/>]: <span class="output-{run/@result}"><xsl:value-of select="run/@result"/></span></div>
                            <pre>
                                <xsl:copy-of select="run/node()"/>
                            </pre>
                        </p>
                    </xsl:if>
                        
                    <xsl:copy-of select="document( 'html/make_tinyurl.html' )"/>
                </body>
                
            </html>
        </exsl:document>         
    </xsl:template>


    <xsl:template name="write_test_results_reference_file">
        <xsl:param name="path"/>
        <xsl:param name="log_file_path"/>
        <xsl:message>    Writing log frame document <xsl:value-of select="$path"/></xsl:message>
        <exsl:document href="{$path}"
            method="html" 
            doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
            encoding="utf-8"
            indent="yes">
            
            <html>
                <head>
                    <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
                </head>
                <frameset cols="190px,*" frameborder="0" framespacing="0" border="0">
                    <frame name="tocframe" src="../toc.html" scrolling="auto"/>
                    <frame name="docframe" src="../../{$log_file_path}" scrolling="auto"/>
                </frameset>
            </html>
        </exsl:document>
    </xsl:template>

</xsl:stylesheet>
