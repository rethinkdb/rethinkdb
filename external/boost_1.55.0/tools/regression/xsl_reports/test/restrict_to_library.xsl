<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

    <xsl:output method="xml" encoding="ascii"/>
    <xsl:param name="library"/>


    <xsl:template match="/">
        <xsl:message>
            <xsl:value-of select="$library"/>
        </xsl:message>
        <xsl:apply-templates/>
    </xsl:template>

    <xsl:template match="*">
        <xsl:copy>
            <xsl:apply-templates select="@*"/>
            <xsl:apply-templates />
        </xsl:copy>
    </xsl:template>
    
    <xsl:template match="test-log">          
      <xsl:if test="@library=$library">
          <xsl:copy>
              <xsl:apply-templates select="@*"/>
              <xsl:apply-templates/>
          </xsl:copy>
      </xsl:if>
  </xsl:template>
  
  <xsl:template match="@*">
      <xsl:copy-of select="."/>
  </xsl:template>
  
</xsl:stylesheet>
