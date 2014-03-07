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
  xmlns:meta="http://www.meta-comm.com"
  xmlns:set="http://exslt.org/sets"
  extension-element-prefixes="func exsl"
  exclude-result-prefixes="exsl set meta"
  version="1.0">
  
  <xsl:import href="common.xsl"/>
  
  <xsl:output method="html" 
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" 
    encoding="utf-8" 
    indent="yes"
    />
  
  
  <xsl:param name="source"/>
  <xsl:param name="run_date"/>
  <xsl:param name="warnings"/>
  <xsl:param name="comment_file"/>
  <xsl:param name="expected_results_file"/>
  <xsl:param name="explicit_markup_file"/>
  <xsl:param name="release"/>
  
  <!-- the author-specified expected test results -->
  <xsl:variable name="explicit_markup" select="document( $explicit_markup_file )"/>
  <xsl:variable name="expected_results" select="document( $expected_results_file )" />
  
  <xsl:variable name="release_postfix">
    <xsl:if test="$release='yes'">
      <xsl:text>_release</xsl:text>
    </xsl:if>
  </xsl:variable>
  
  <!-- necessary indexes -->
  <xsl:key 
    name="test_name_key" 
    match="test-log" 
    use="concat( @library, '@', @test-name )"/>
  
  <xsl:key 
    name="library_key" 
    match="test-log" 
    use="@library"/>
  <xsl:key name="toolset_key" match="test-log" use="@toolset"/>
  
  <!-- toolsets -->
  
  <xsl:variable name="required_toolsets" select="$explicit_markup//mark-toolset[ @status='required' ]"/>
  <xsl:variable name="required_toolset_names" select="$explicit_markup//mark-toolset[ @status='required' ]/@name"/>
  
  <!-- libraries -->
  
  <xsl:variable 
    name="failing_tests" 
    select="//test-log[@status='unexpected' and @result='fail' 
                       and @toolset = $required_toolset_names 
                       and meta:is_test_log_a_test_case(.) 
                       and meta:show_library( @library, $release ) 
                       and meta:show_toolset( @toolset, $release ) 
                       and not (meta:is_unusable($explicit_markup, @library, 
                                                 @toolset )) ]"/>

  <xsl:variable name="libraries" select="set:distinct( $failing_tests/@library )"/>

  <xsl:template match="/">
    <xsl:variable name="issues_list" 
      select="concat('issues', $release_postfix, '_.html')"/>
    
    <!-- Issues page -->
    <html>
      <head>
        <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
        <title>Boost regression unresolved issues: <xsl:value-of select="$source"/></title>
      </head>
      <frameset cols="190px,*" frameborder="0" framespacing="0" border="0">
        <frame name="tocframe" src="toc{$release_postfix}.html" scrolling="auto"/>
        <frame name="docframe" src="{$issues_list}" scrolling="auto"/>
      </frameset>
    </html>
    
    <!-- Issues list -->
    <xsl:message>Writing document <xsl:value-of select="$issues_list"/></xsl:message>
    
    <exsl:document href="{$issues_list}"
      method="html" 
      doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
      encoding="utf-8"
      indent="yes">
      
      <html>
        <head>
          <link rel="stylesheet" type="text/css" href="../master.css" title="master" />
        </head>
        <body>
          
          <h1 class="page-title">
            <xsl:text>Unresolved Issues: </xsl:text>
            <a class="hover-link" href="summary{$release_postfix}.html" target="_top"><xsl:value-of select="$source"/></a>
          </h1>

          <xsl:call-template name="insert_report_header">
            <xsl:with-param name="run_date" select="$run_date"/>
            <xsl:with-param name="warnings" select="$warnings"/>
            <xsl:with-param name="purpose" select="'Provides a list of current unresolved test failures.'"/>
          </xsl:call-template>

          <!-- Emit the index -->  
          <h2>Libraries with unresolved failures</h2>
          <div align="center">
            <xsl:for-each select="$libraries">
              <xsl:sort select="." order="ascending"/>
              <xsl:variable name="library" select="."/>
              <a href="#{$library}">
                <xsl:value-of select="$library"/>
              </a>
              <xsl:text>  </xsl:text>
            </xsl:for-each>
          </div>

          <xsl:for-each select="$libraries">
            <xsl:sort select="." order="ascending"/>
            <xsl:variable name="library" select="."/>
            <xsl:variable name="library_page" select="meta:encode_path( $library )" />
            <xsl:variable name="library_tests" select="$failing_tests[@library = $library]"/>
            <xsl:variable name="library_test_names" select="set:distinct( $library_tests/@test-name )"/>
              
            <h2>
              <a name="{$library}"/>
              <a class="hover-link" href="{$library_page}{$release_postfix}.html" target="_top">
                <xsl:value-of select="$library"/>
                <xsl:text> (</xsl:text>
                <xsl:value-of select="count($library_tests)"/>
                <xsl:text> failure</xsl:text>
                <xsl:if test="count($library_tests) &gt; 1">
                  <xsl:text>s</xsl:text>
                </xsl:if>
                <xsl:text>)</xsl:text>
              </a>
            </h2>
            
            <table class="library-issues-table" summary="issues">
              <thead>
                <tr valign="middle">
                  <td class="head">test</td>
                  <td class="head">failures</td>
                </tr>
              </thead>
              <tfoot>
                <tr valign="middle">
                  <td class="head">test</td>
                  <td class="head">failures</td>
                </tr>
              </tfoot>
              
              <tbody>
                <xsl:for-each select="$library_test_names">
                  <xsl:sort select="." order="ascending"/>
                  <xsl:variable name="test_name" select="."/>
                  
                  <xsl:variable name="unexpected_toolsets" select="$library_tests[@test-name = $test_name]/@toolset"/>
                  
                  <xsl:variable name="test_program"  select="$library_tests[@test-name = $test_name]/@test-program"/>
                  <tr>
                    <td class="test-name">
                      <a href="http://svn.boost.org/svn/boost/{$source}/{$test_program}" class="test-link" target="_top">
                        <xsl:value-of select="$test_name"/>
                      </a>
                    </td>
                    <td class="failures-row">
                      <table summary="unexpected fail legend" class="issue-box">
                        <tr class="library-row-single">
                          <xsl:for-each select="$unexpected_toolsets">
                            <xsl:sort select="." order="ascending"/>
                            <xsl:variable name="toolset" select="."/>
                            <xsl:variable name="test_logs" 
                              select="$library_tests[@test-name = $test_name 
                                                     and @toolset = $toolset]"/>
                            <xsl:for-each select="$test_logs">
                              <xsl:call-template name="print_failure_cell">
                                <xsl:with-param name="test_log" select="."/>
                                <xsl:with-param name="toolset" select="$toolset"/>
                              </xsl:call-template>
                            </xsl:for-each>
                          </xsl:for-each>                          
                        </tr>
                      </table>
                    </td>
                  </tr>
                </xsl:for-each>
              </tbody>
              
            </table>
          </xsl:for-each>
          <xsl:copy-of select="document( 'html/issues_legend.html' )"/>
          <xsl:copy-of select="document( 'html/make_tinyurl.html' )"/>
        </body>
      </html>
    </exsl:document>  

    <xsl:message>Writing document issues-email.txt</xsl:message>
    <exsl:document href="issues-email.txt" method="text" encoding="utf-8">
      <xsl:text>Boost regression test failures
------------------------------
Report time: </xsl:text>
      
      <xsl:value-of select="$run_date"/>
      
      <xsl:text>

This report lists all regression test failures on release platforms.

Detailed report: 
        http://beta.boost.org/development/tests/</xsl:text>
        <xsl:value-of select="$source"/>
        <xsl:text>/developer/issues.html

</xsl:text>
      <xsl:value-of select="count($failing_tests)"/>
      <xsl:text> failure</xsl:text>
      <xsl:if test="count($failing_tests) &gt; 1">
        <xsl:text>s</xsl:text>
      </xsl:if>
      <xsl:text> in </xsl:text>
      <xsl:value-of select="count($libraries)"/>
      <xsl:text> librar</xsl:text>
      <xsl:choose>
        <xsl:when test="count($libraries) &gt; 1">
          <xsl:text>ies</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>y</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>:
</xsl:text>
      <xsl:for-each select="$libraries">
        <xsl:sort select="." order="ascending"/>
        <xsl:variable name="library" select="."/>
        <xsl:text>  </xsl:text>
        <xsl:value-of select="$library"/>
        <xsl:text> (</xsl:text>
        <xsl:value-of select="count($failing_tests[@library = $library])"/>
        <xsl:text>)
</xsl:text>
      </xsl:for-each>

      <xsl:for-each select="$libraries">
        <xsl:sort select="." order="ascending"/>
        <xsl:variable name="library" select="."/>
        <xsl:variable name="library_page" select="meta:encode_path( $library )" />
        <xsl:variable name="library_tests" select="$failing_tests[@library = $library]"/>
        <xsl:variable name="library_test_names" select="set:distinct( $library_tests/@test-name )"/>

        <xsl:text>
|</xsl:text>
        <xsl:value-of select="$library"/>
        <xsl:text>|
</xsl:text>

        <xsl:for-each select="$library_test_names">
          <xsl:sort select="." order="ascending"/>
          <xsl:variable name="test_name" select="."/>
          
          <xsl:variable name="unexpected_toolsets" select="$library_tests[@test-name = $test_name]/@toolset"/>
          
          <xsl:variable name="test_program"  select="$library_tests[@test-name = $test_name]/@test-program"/>
          <xsl:text>  </xsl:text>
          <xsl:value-of select="$test_name"/>
          <xsl:text>:</xsl:text>
          <xsl:for-each select="$unexpected_toolsets">
            <xsl:sort select="." order="ascending"/>
            <xsl:text>  </xsl:text>
            <xsl:value-of select="."/>
          </xsl:for-each>
          <xsl:text>
</xsl:text>
        </xsl:for-each>
      </xsl:for-each>
    </exsl:document>
  </xsl:template>

  <xsl:template name="print_failure_cell">
    <xsl:param name="test_log" select="."/>
    <xsl:param name="toolset"/>

    <xsl:variable name="test_run" select="$test_log/.."/>

    <xsl:variable name="log_link">
      <xsl:value-of select="meta:log_file_path($test_log, $test_run/@runner, 
                                               $release_postfix )"/>
    </xsl:variable>
    <xsl:variable name="class">
      <xsl:choose>
        <xsl:when test="$test_log/@is-new = 'yes'">
          <xsl:text>library-fail-unexpected-new</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>library-fail-unexpected</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    
    <td class="{$class}">
      <span>
        <a href="{$log_link}" class="log-link" target="_top">
          <xsl:value-of select="$toolset"/>
        </a>
      </span>
    </td>
  </xsl:template>
</xsl:stylesheet>
