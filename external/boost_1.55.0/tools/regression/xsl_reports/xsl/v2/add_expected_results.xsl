<?xml version="1.0" encoding="utf-8"?>
<!--

Copyright MetaCommunications, Inc. 2003-2007.

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)

-->

<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns:func="http://exslt.org/functions"
    xmlns:meta="http://www.meta-comm.com"
    extension-element-prefixes="func"
    exclude-result-prefixes="func meta"
    version="1.0">

    <xsl:import href="common.xsl"/>
  
    <xsl:output method="xml" encoding="utf-8"/>
      
    <xsl:param name="expected_results_file"/>
    <xsl:param name="failures_markup_file"/>
    <xsl:param name="source"/>
    <xsl:variable name="expected_results" select="document( $expected_results_file )" />

    <func:function name="meta:is_test_log_complete">
        <xsl:param name="test_log"/>
        <xsl:variable name="type" select="$test_log/@test-type"/>
        <func:result>
            <xsl:choose>
                <xsl:when test="$type='compile' or $type='compile_fail'  or $test_log/compile/@result='fail' ">
                    <xsl:value-of select="count( $test_log/compile ) = 1 and count( $test_log/link) = 0 and count( $test_log/run) = 0"/>
                </xsl:when>
                <xsl:when test="$type='link' or $type='link_fail' or $type='' or $type='lib' or $test_log/link/@result='fail'">
                    <xsl:value-of select="count( $test_log/compile) = 1  and count( $test_log/link) = 1 and count( $test_log/run) = 0"/></xsl:when>
                <xsl:when test="$type='run' or $type='run_fail' or $type='run_pyd' or $type='run_mpi'">
                    <xsl:value-of select="count( $test_log/compile) = 1  and count( $test_log/link)  = 1 and count($test_log/run) = 1 "/>
                </xsl:when>
                <xsl:otherwise> 
                    <xsl:message terminate="yes">
                        Unknown test type "<xsl:value-of select="$type"/>"
                    </xsl:message>
                </xsl:otherwise>
            </xsl:choose>
        </func:result>            
    </func:function>


    <xsl:key name = "trk" match = "test-result" use = "concat( ../../@name, '-', ../@name, '-', @test-name )" />
    <xsl:key name = "tak" match = "toolset-alias" use = "@name" />

    <xsl:variable name="failures_markup" select="document( $failures_markup_file )" />
    <xsl:template match="/">
        <xsl:apply-templates/>
    </xsl:template>
      
    <xsl:template match="test-log">
        <xsl:variable name="test_log" select="."/>
        <xsl:variable name="library" select="@library"/>
        <xsl:variable name="test-name" select="@test-name"/>
        <xsl:variable name="toolset" select="@toolset"/>

        <xsl:variable name="is_complete" select="meta:is_test_log_complete( $test_log )"/>

        <xsl:element name="{local-name()}">
            <xsl:apply-templates select="@*"/>

            <xsl:variable name="has_failures" select="./*/@result = 'fail'"/>
            <xsl:variable name="actual_result">
                <xsl:choose>
                    <xsl:when test="$has_failures or not( $is_complete )" >
                        <xsl:text>fail</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text>success</xsl:text>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:variable>
            
            <!-- 
                 Select expected_results context 
                 See http://clover.slavic.pitt.edu/~repertorium/plectogram/keys/keys.html for a good explanation.
                 
                 Briefly, for-each doesn't iterate through expected_results, it just selects expected result
                 as current context to make "key" function work.
                 -->

            <xsl:for-each select="$expected_results">

                <xsl:variable name="main_toolset" select="key( 'tak', $toolset )/../@name" />
                <xsl:variable name="toolset_name">
                    <xsl:choose>
                        <xsl:when test="$main_toolset"><xsl:value-of select="$main_toolset"/></xsl:when>
                        <xsl:otherwise><xsl:value-of select="$toolset"/></xsl:otherwise>
                    </xsl:choose>
                </xsl:variable>

                <xsl:variable name="expected_results_test_case" select="key( 'trk', concat( $toolset_name, '-', $library, '-', $test-name ) )"/>
                <xsl:variable name="test_case_markup"           select="$failures_markup//library[@name=$library]/test[ meta:re_match( @name, $test-name ) ]"/>
                <xsl:variable name="test_failures_markup"       select="$test_case_markup/mark-failure/toolset[ meta:re_match( @name, $toolset ) ]/.."/>
                <xsl:variable name="test_failures_markup2"      select="$failures_markup//library[@name=$library]/mark-expected-failures/test[ meta:re_match( @name, $test-name ) ]/../toolset[ meta:re_match( @name, $toolset ) ]/.."/>

                <xsl:variable name="category">
                    <xsl:choose>
                        <xsl:when test="$test_case_markup/@category">
                            <xsl:value-of select="$test_case_markup/@category"/>
                        </xsl:when>
                        <xsl:otherwise>0</xsl:otherwise>
                    </xsl:choose>
                </xsl:variable>

                
                <xsl:variable name="is_new">
                    <xsl:choose>
                        <xsl:when test="$expected_results_test_case">
                            <xsl:text>no</xsl:text>
                        </xsl:when>
                        <xsl:otherwise>yes</xsl:otherwise>
                    </xsl:choose>
                </xsl:variable>
                

                <xsl:variable name="has_explicit_markup" select="count( $test_failures_markup ) > 0 or count( $test_failures_markup2 ) > 0"/>
                    
                <xsl:variable name="expected_result">
                    <xsl:choose>
                        <xsl:when test="$has_explicit_markup">
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
                        <xsl:when test="$expected_result = $actual_result">expected</xsl:when>
                        <xsl:otherwise>unexpected</xsl:otherwise>
                    </xsl:choose>
                </xsl:variable>

                <xsl:variable name="unexpected_success" select="$status = 'unexpected' and $actual_result = 'success'"/>
            
                <xsl:variable name="expected_reason">
                    <xsl:choose>
                        <xsl:when test="$test_failures_markup/@reason">
                            <xsl:value-of select="$test_failures_markup/@reason"/>
                        </xsl:when>
                        <xsl:otherwise>
                            <xsl:value-of select="$test_failures_markup2/@reason"/>
                        </xsl:otherwise>
                    </xsl:choose>
                </xsl:variable>


                <xsl:variable name="notes">
                    
                    <xsl:if test="$unexpected_success and $has_explicit_markup">
                        <note>
                            <span class="auto-note">
                                This test case was explicitly marked up in 
                                <a href="http://svn.boost.org/svn/boost/{$source}/status/explicit-failures-markup.xml">
                                    status/explicit-failures-markup.xml</a> file in the Boost SVN as "expected to fail",
                                but is passing. Please consult the notes/output below for more details.
                            </span>
                        </note>
                    </xsl:if>
                    
                    <xsl:if test="$has_explicit_markup and count( $test_failures_markup2/note ) = 0 and count( $test_failures_markup/note ) = 0">
                        <xsl:choose>
                            <xsl:when test="$unexpected_success">
                                <note>
                                    <span class="auto-note">
                                        No explanation was provided for this markup. Please contact the library 
                                        author(s)/maintainer(s) for more details.
                                    </span>
                                </note>
                            </xsl:when>
                            <xsl:otherwise>
                                <note>
                                    <span class="auto-note">
                                        This failure was explicitly marked as expected in 
                                        <a href="http://svn.boost.org/svn/boost/{$source}/status/explicit-failures-markup.xml">
                                            status/explicit-failures-markup.xml</a> file in the Boost SVN. 
                                        Please contact the library author(s)/maintainer(s) for the explanation of this markup.
                                    </span>
                                </note>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:if>
                    
                    <xsl:if test="count( $test_failures_markup ) > 0">
                        <xsl:for-each select="$test_failures_markup/note">
                            <xsl:copy-of select="."/>
                        </xsl:for-each>
                    </xsl:if>
                    
                    <xsl:if test="count( $test_failures_markup2 ) > 0">
                        <xsl:for-each select="$test_failures_markup2/note">
                            <xsl:copy-of select="."/>
                        </xsl:for-each>
                    </xsl:if>
                    
                    
                    <xsl:if test="$expected_results_test_case and $expected_results_test_case/@result = 'fail'">
                        <xsl:choose>
                            <xsl:when test="$unexpected_success">
                                <note>
                                    <span class="auto-note">
                                        This test case used to fail in the reference ("last-known-good") release.
                                    </span>
                                </note>
                            </xsl:when>
                            <xsl:otherwise>
                                <note>
                                    <span class="auto-note">
                                        This failure was present in the reference ("last-known-good") release.
                                    </span>
                                </note>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:if>

                    <xsl:if test="not( $is_complete ) and not( $has_failures )">
                        <note>
                            <span class="internal-error-note">
                                <b>[Reporting Tools Internal Error]</b> This test case's XML is missing one or more log entries
                                of the regression run's steps associated with the test case's type ("<xsl:value-of select="$test_log/@test-type"/>").
                                Please <a href="mailto:mailto:boost-testing@lists.boost.org">contact reporting tools 
                                maintainers</a> about this problem.
                            </span>
                        </note>
                    </xsl:if>
                </xsl:variable>
                
                <xsl:attribute name="result"><xsl:value-of select="$actual_result"/></xsl:attribute>
                <xsl:attribute name="expected-result"><xsl:value-of select="$expected_result"/></xsl:attribute>
                <xsl:attribute name="expected-reason"><xsl:value-of select="$expected_reason"/></xsl:attribute>
                <xsl:attribute name="status"><xsl:value-of select="$status"/></xsl:attribute>
                <xsl:attribute name="is-new"><xsl:value-of select="$is_new"/></xsl:attribute>
                <xsl:attribute name="category"><xsl:value-of select="$category"/></xsl:attribute>
                <xsl:element name="notes"><xsl:copy-of select="$notes"/></xsl:element>
                
                <xsl:apply-templates select="$test_log/node()" />
            </xsl:for-each>
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
