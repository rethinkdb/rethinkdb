<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                xmlns:sverb="http://nwalsh.com/xslt/ext/com.nwalsh.saxon.Verbatim"
                xmlns:xverb="com.nwalsh.xalan.Verbatim"
                xmlns:lxslt="http://xml.apache.org/xslt"
                xmlns:exsl="http://exslt.org/common"
                exclude-result-prefixes="sverb xverb lxslt exsl"
                version='1.0'>

<!-- Copyright (c) 2005 CrystalClear Software, Inc.
     Subject to the Boost Software License, Version 1.0. 
     (See accompanying file LICENSE_1_0.txt or  http://www.boost.org/LICENSE_1_0.txt)
-->

  <xsl:import href="../../../tools/boostbook/xsl/fo.xsl" />

  <!-- reset the default margin parameters -->
  <xsl:param name="page.margin.bottom" select="'0.25in'"/>
  <xsl:param name="page.margin.inner">
    <xsl:choose>
      <xsl:when test="$double.sided != 0">1.25in</xsl:when>
      <xsl:otherwise>0.65in</xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="page.margin.outer">
    <xsl:choose>
      <xsl:when test="$double.sided != 0">0.75in</xsl:when>
      <xsl:otherwise>0.65in</xsl:otherwise>
    </xsl:choose>
  </xsl:param>

  <!-- with the above margins, we can fit 38 monospace characters per table cell.
  If the margins are changed, set min-reduction to 100 and generate a new pdf. Then
  count the number of characters that fit in a cell. Not the slickest method but it works.-->
  <xsl:variable name="char-per-cell" select="38"/>
  <!-- prevent reducing the font size too much. An 80% reduction 
  gives us a width of 48 characters per cell -->
  <xsl:variable name="min-reduction" select="80"/>

  <!-- recursive font-size reduction template -->
  <!-- the string may span multiple lines. 
  break it up and check the length of each line. 
  adjust font size according to longest line -->
  <xsl:template name="font-size-reduction">
    <xsl:param name="inp" />
    <xsl:param name="max">1</xsl:param>
    <xsl:variable name="result">
      <xsl:choose>
        <!-- &#x000A; is unicode line-feed character -->
        <xsl:when test="contains($inp, '&#x000A;')">
          <xsl:variable name="str" select="substring-before($inp, '&#x000A;')" />
          <xsl:variable name="next" select="substring-after($inp, '&#x000A;')" />

          <xsl:variable name="n_max">
            <xsl:choose>
              <xsl:when test="string-length($str) > $char-per-cell and string-length($str) > $max">
                <xsl:value-of select="string-length($str)" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$max" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          
          <xsl:choose>
            <xsl:when test="contains($next, '&#x000A;')">
              <xsl:call-template name="font-size-reduction">
                <xsl:with-param name="inp" select="$next" />
                <xsl:with-param name="max" select="$n_max" />
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:choose>
                <xsl:when test="$n_max > $char-per-cell and $n_max > string-length($next)">
                  <!-- set size with next.len -->
                  <xsl:value-of select="round($char-per-cell div $n_max * 100)"/>
                </xsl:when>
                <xsl:when test="string-length($next) > $char-per-cell and string-length($next) > $n_max">
                  <!-- set size with n_max -->
                  <xsl:value-of select="round($char-per-cell div string-length($next) * 100)"/>
                </xsl:when>
                <xsl:otherwise>100</xsl:otherwise>
              </xsl:choose>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="string-length($inp) > $char-per-cell">
              <!-- set size with inp.len -->
              <xsl:value-of select="round($char-per-cell div string-length($inp) * 100)"/>
            </xsl:when>
            <xsl:otherwise>100</xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <!-- return either "result" or "min-reduction" -->
    <xsl:choose>
      <xsl:when test="$min-reduction > $result">
        <xsl:value-of select="$min-reduction" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$result" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
            

  
  <!-- this template was copied directly from docbook/boostbook xsl and modified 
  to calculate and resize monospace font to fit cells based on length of string -->
  <xsl:template match="entry/screen">
    <xsl:param name="suppress-numbers" select="'0'"/>
    <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>

    <xsl:variable name="content">
      <xsl:choose>
        <xsl:when test="$suppress-numbers = '0'
                        and @linenumbering = 'numbered'
                        and $use.extensions != '0'
                        and $linenumbering.extension != '0'">
          <xsl:call-template name="number.rtf.lines">
            <xsl:with-param name="rtf">
              <xsl:apply-templates/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="font_size">
      <xsl:call-template name="font-size-reduction">
        <xsl:with-param name="inp" select="." />
      </xsl:call-template>
    </xsl:variable>


    <!-- write out the tag now that font size has been calculated -->
    <xsl:choose>
      <xsl:when test="$shade.verbatim != 0">
        <fo:block id="{$id}"
                  white-space-collapse='false'
                  white-space-treatment='preserve'
                  linefeed-treatment='preserve'
                  xsl:use-attribute-sets="monospace.verbatim.properties shade.verbatim.style" font-size="{$font_size}%">
          <xsl:choose>
            <xsl:when test="$hyphenate.verbatim != 0 and function-available('exsl:node-set')">
              <xsl:apply-templates select="exsl:node-set($content)" mode="hyphenate.verbatim"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:copy-of select="$content"/>
            </xsl:otherwise>
          </xsl:choose>
        </fo:block>
      </xsl:when>
      <xsl:otherwise>
        <fo:block id="{$id}"
                  white-space-collapse='false'
                  white-space-treatment='preserve'
                  linefeed-treatment="preserve"
                  xsl:use-attribute-sets="monospace.verbatim.properties" font-size="{$font_size}%">
          <xsl:choose>
            <xsl:when test="$hyphenate.verbatim != 0 and function-available('exsl:node-set')">
              <xsl:apply-templates select="exsl:node-set($content)" mode="hyphenate.verbatim"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:copy-of select="$content"/>
            </xsl:otherwise>
          </xsl:choose>
        </fo:block>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


</xsl:stylesheet>
