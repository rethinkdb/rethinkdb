<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:set="http://exslt.org/sets"
                extension-element-prefixes="set"
                version="1.0">

  <xsl:output method="xml" encoding="utf-8"/>

  <xsl:template match="/">
      <expected-failures>
          <xsl:variable name="toolsets" select="set:distinct(//test-result/@toolset)"/>
          <xsl:for-each select="$toolsets">
              <xsl:variable name="toolset" select="."/>
              <toolset name="{$toolset}">
                  <xsl:variable name="toolset_test_cases" select="//test-result[@toolset = $toolset]"/>
                  <xsl:variable name="libraries" select="set:distinct($toolset_test_cases/@library)"/>
                  <xsl:for-each select="$libraries">
                      <xsl:variable name="library" select="."/>
                      <library name="{$library}">
                          <xsl:variable name="test_results" select="$toolset_test_cases[@library = $library]"/>
                          <xsl:for-each select="$test_results">
                              <xsl:variable name="test_result" select="."/>
                              <test-result test-name="{$test_result/@test-name}" result="{$test_result/@result}"/>
                          </xsl:for-each>
                      </library>
                  </xsl:for-each>
              </toolset>
          </xsl:for-each>
      </expected-failures>
  </xsl:template>

</xsl:stylesheet>
