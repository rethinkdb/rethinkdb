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
    xmlns:set="http://exslt.org/sets"
    xmlns:meta="http://www.meta-comm.com"
    extension-element-prefixes="func exsl"
    exclude-result-prefixes="exsl func set meta"
    version="1.0">

    <xsl:import href="common.xsl"/>

    <xsl:output method="html" 
        doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" 
        encoding="utf-8" 
        indent="yes"
        />

    <xsl:param name="mode"/>
    <xsl:param name="source"/>
    <xsl:param name="run_date"/>
    <xsl:param name="warnings"/>
    <xsl:param name="comment_file"/>
    <xsl:param name="explicit_markup_file"/>
    <xsl:param name="release"/>

    <xsl:variable name="explicit_markup" select="document( $explicit_markup_file )"/>

    <!-- necessary indexes -->
    <xsl:key 
        name="library_test_name_key" 
        match="test-log" 
        use="concat( @library, '&gt;@&lt;', @test-name )"/>
    <xsl:key name="toolset_key" match="test-log" use="@toolset"/>
    <xsl:key name="test_name_key"  match="test-log" use="@test-name "/>

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

    <!--<xsl:variable name="expected_results" select="document( $expected_results_file )" />-->

    <!-- runs / toolsets -->
    <xsl:variable name="run_toolsets" select="meta:test_structure( /, $release )"/>

    <!-- libraries -->

    <xsl:variable name="test_case_logs" select="//test-log[ meta:is_test_log_a_test_case(.) and meta:show_library( @library, $release ) and meta:show_toolset( @toolset, $release )]"/>
    <xsl:variable name="libraries" select="set:distinct( $test_case_logs/@library )"/>

    <xsl:variable name="sorted_libraries_output">
        <xsl:for-each select="$libraries[ meta:show_library( ., $release )]">
            <xsl:sort select="." order="ascending" />
            <library><xsl:copy-of select="."/></library>
        </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="sorted_libraries" select="exsl:node-set( $sorted_libraries_output )/library/@library"/>

    <!-- modes -->

    <xsl:variable name="alternate_mode">
        <xsl:choose>
        <xsl:when test="$mode='user'">developer</xsl:when>
        <xsl:otherwise>user</xsl:otherwise>
        </xsl:choose>
    </xsl:variable>

    <xsl:variable name="release_postfix">
        <xsl:if test="$release='yes'">
            <xsl:text>_release</xsl:text>
        </xsl:if>
    </xsl:variable>
     
    <xsl:template match="/">

        <xsl:variable name="summary_results" select="concat( 'summary', $release_postfix, '_.html' )"/>

        <!-- Summary page -->
        <html>
            <head>
                <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
                <title>Boost regression summary: <xsl:value-of select="$source"/></title>
            </head>
            <frameset cols="190px,*" frameborder="0" framespacing="0" border="0">
            <frame name="tocframe" src="toc{$release_postfix}.html" scrolling="auto"/>
            <frame name="docframe" src="{$summary_results}" scrolling="auto"/>
            </frameset>
        </html>

        <!-- Summary results -->
        <xsl:message>Writing document <xsl:value-of select="$summary_results"/></xsl:message>
        
        <exsl:document href="{$summary_results}"
            method="html" 
            doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
            encoding="utf-8"
            indent="yes">

            <html>
            <head>
                <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
            </head>
            <body>

            <xsl:call-template name="insert_page_links">
                <xsl:with-param name="page" select="'summary'"/>
                <xsl:with-param name="release" select="$release"/>
                <xsl:with-param name="mode" select="$alternate_mode"/>
            </xsl:call-template>

            <h1 class="page-title">
                <xsl:text>Summary: </xsl:text>
                <a class="hover-link" href="summary{$release_postfix}.html" target="_top"><xsl:value-of select="$source"/></a>
            </h1>

            <xsl:call-template name="insert_report_header">
                <xsl:with-param name="run_date" select="$run_date"/>
                <xsl:with-param name="warnings" select="$warnings"/>
            </xsl:call-template>

            <div class="statistics">
            Unusable: <xsl:value-of select="count( $test_case_logs[ meta:test_case_status( $explicit_markup, . ) = 'unusable' ] )"/>
            &#160;|&#160;
            Regressions: <xsl:value-of select="count( $test_case_logs[ meta:test_case_status( $explicit_markup, . ) = 'fail-unexpected' ] )"/>
            &#160;|&#160;
            New failures: <xsl:value-of select="count( $test_case_logs[ meta:test_case_status( $explicit_markup, . ) = 'fail-unexpected-new' ] )"/>
            </div>
            
            <!-- summary table -->

            <table border="0" cellspacing="0" cellpadding="0" width="1%" class="summary-table" summary="Overall summary">

            <thead>
                <xsl:call-template name="insert_runners_rows">
                    <xsl:with-param name="mode" select="'summary'"/>
                    <xsl:with-param name="top_or_bottom" select="'top'"/>
                    <xsl:with-param name="run_toolsets" select="$run_toolsets"/>
                    <xsl:with-param name="run_date" select="$run_date"/>
                </xsl:call-template>

                <xsl:call-template name="insert_toolsets_row">
                    <xsl:with-param name="mode" select="'summary'"/>
                    <xsl:with-param name="run_date" select="$run_date"/>
                </xsl:call-template>
            </thead>

            <tfoot>
                <xsl:call-template name="insert_toolsets_row">
                    <xsl:with-param name="mode" select="'summary'"/>
                    <xsl:with-param name="run_date" select="$run_date"/>
                </xsl:call-template>
                <xsl:call-template name="insert_runners_rows">
                    <xsl:with-param name="mode" select="'summary'"/>
                    <xsl:with-param name="top_or_bottom" select="'bottom'"/>
                    <xsl:with-param name="run_toolsets" select="$run_toolsets"/>
                    <xsl:with-param name="run_date" select="$run_date"/>
                </xsl:call-template>
            </tfoot>
          
            <tbody>
                <xsl:variable name="test_logs" select="$test_case_logs"/>

                <!-- for each library -->
                <xsl:for-each select="$sorted_libraries">
                    <xsl:variable name="library" select="."/>
                    <xsl:variable name="library_page" select="meta:encode_path( $library )" />
                    <xsl:variable name="current_row" select="$test_logs[ @library=$library ]"/>

                    <xsl:variable name="expected_test_count" select="count( $current_row[ generate-id(.) = generate-id( key('test_name_key',@test-name)[1] ) ] )"/>
                    <xsl:variable name="library_header">
                        <td class="library-name">
                            <a href="{$library_page}{$release_postfix}.html" class="library-link" target="_top">
                                <xsl:value-of select="$library"/>
                            </a>
                        </td>
                    </xsl:variable>

                <xsl:variable name="line_mod">
                    <xsl:choose>
                    <xsl:when test="1 = last()">
                        <xsl:text>-single</xsl:text>
                    </xsl:when>
                    <xsl:when test="generate-id( . ) = generate-id( $sorted_libraries[1] )">
                        <xsl:text>-first</xsl:text>
                    </xsl:when>
                    <xsl:when test="generate-id( . ) = generate-id( $sorted_libraries[ last() ] )">
                        <xsl:text>-last</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text></xsl:text>
                    </xsl:otherwise>
                    </xsl:choose>
                </xsl:variable>


                <tr class="summary-row{$line_mod}">
                    <xsl:copy-of select="$library_header"/>

                    <xsl:for-each select="$run_toolsets/platforms/platform/runs/run/toolset">
                        <xsl:variable name="toolset" select="@name" />
                        <xsl:variable name="runner" select="../@runner" />

                        <xsl:variable name="current_cell" select="$current_row[ @toolset=$toolset and ../@runner = $runner ]"/>

                        <xsl:choose>
                            <xsl:when test="$mode='user'">
                                <xsl:call-template name="insert_cell_user">
                                    <xsl:with-param name="current_cell" select="$current_cell"/>
                                    <xsl:with-param name="library" select="$library"/>
                                    <xsl:with-param name="toolset" select="$toolset"/>
                                    <xsl:with-param name="expected_test_count" select="$expected_test_count"/>
                                </xsl:call-template>
                            </xsl:when>
                            <xsl:when test="$mode='developer'">
                                <xsl:call-template name="insert_cell_developer">
                                    <xsl:with-param name="current_cell" select="$current_cell"/>
                                    <xsl:with-param name="library" select="$library"/>
                                    <xsl:with-param name="toolset" select="$toolset"/>
                                    <xsl:with-param name="expected_test_count" select="$expected_test_count"/>
                                </xsl:call-template>
                            </xsl:when>
                        </xsl:choose>
                    </xsl:for-each>
                    
                    <xsl:copy-of select="$library_header"/>
                </tr>          
                </xsl:for-each>
            </tbody>
            </table>

            <div id="legend">
                <xsl:copy-of select="document( concat( 'html/summary_', $mode, '_legend.html' ) )"/>
            </div>

            <xsl:call-template name="insert_page_links">
                <xsl:with-param name="page" select="'summary'"/>
                <xsl:with-param name="release" select="$release"/>
                <xsl:with-param name="mode" select="$alternate_mode"/>
            </xsl:call-template>

            </body>
            </html>
        </exsl:document>  

    </xsl:template>

    <!-- report developer status -->
    <xsl:template name="insert_cell_developer">
        <xsl:param name="current_cell"/>
        <xsl:param name="library"/>
        <xsl:param name="toolset"/>
        <xsl:param name="expected_test_count"/>

    <xsl:variable name="class" select="concat( 'summary-', meta:result_cell_class( $library, $toolset, $current_cell ) )"/>
      
    <xsl:variable name="library_page" select="meta:encode_path( $library )" />

    <td class="{$class}" title="{$library}/{$toolset}">
        <xsl:choose>
        <xsl:when test="$class='summary-unusable'">
            <xsl:text>&#160;&#160;</xsl:text>
            <a href="{$library_page}{$release_postfix}.html" class="log-link" target="_top">
                <xsl:text>n/a</xsl:text>
            </a>          
            <xsl:text>&#160;&#160;</xsl:text>
        </xsl:when>
        <xsl:when test="$class='summary-missing'">
            <xsl:text>&#160;&#160;&#160;&#160;</xsl:text>
        </xsl:when>
        <xsl:when test="$class='summary-fail-unexpected'">
            <a href="{$library_page}{$release_postfix}.html" class="log-link" target="_top">
                <xsl:text>broken</xsl:text>
            </a>
        </xsl:when>
        <xsl:when test="$class='summary-fail-unexpected-new' ">
            <xsl:text>&#160;&#160;</xsl:text>
            <a href="{$library_page}{$release_postfix}.html" class="log-link" target="_top">
                <xsl:text>fail</xsl:text>
            </a>
            <xsl:text>&#160;&#160;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>&#160;&#160;OK&#160;&#160;</xsl:text>
        </xsl:otherwise>
        </xsl:choose>
    </td>
      
    </xsl:template>


    <!-- report user status -->
    <xsl:template name="insert_cell_user">
    <xsl:param name="current_cell"/>
    <xsl:param name="library"/>
    <xsl:param name="toolset"/>
    <xsl:param name="expected_test_count"/>

    <xsl:variable name="class" select="concat( 'summary-', meta:result_cell_class( $library, $toolset, $current_cell ) )"/>
    
    <xsl:variable name="library_page" select="meta:encode_path( $library )" />

    <td class="{$class} user-{$class}" title="{$library}/{$toolset}">
        <xsl:choose>
        <xsl:when test="$class='summary-unusable'">
            <xsl:text>&#160;</xsl:text>
            <a href="{$library_page}{$release_postfix}.html" class="log-link" target="_top">
                <xsl:text>unusable</xsl:text>
            </a>
            <xsl:text>&#160;</xsl:text>
        </xsl:when>
        <xsl:when test="$class='summary-missing'">
            <xsl:text>&#160;no&#160;results&#160;</xsl:text>
        </xsl:when>
        <xsl:when test="$class='summary-fail-unexpected'">
            <xsl:text>&#160;</xsl:text>
            <a href="{$library_page}{$release_postfix}.html" class="log-link" target="_top">
                <xsl:text>regress.</xsl:text>
            </a>
            <xsl:text>&#160;</xsl:text>
        </xsl:when>
        <xsl:when test="$class='summary-fail-unexpected-new'
                     or $class='summary-fail-expected'
                     or $class='summary-unknown-status'
                     or $class='summary-fail-expected-unresearched'">
            <xsl:text>&#160;</xsl:text>
            <a href="{$library_page}{$release_postfix}.html" class="log-link" target="_top">
                <xsl:text>details</xsl:text>
            </a>
            <xsl:text>&#160;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>&#160;pass&#160;</xsl:text>
        </xsl:otherwise>
        </xsl:choose>
    </td>
      
    </xsl:template>

</xsl:stylesheet>
