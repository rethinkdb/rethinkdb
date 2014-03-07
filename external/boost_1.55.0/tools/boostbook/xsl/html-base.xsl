<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:rev="http://www.cs.rpi.edu/~gregod/boost/tools/doc/revision"
                version="1.0">
  
  <xsl:param name="html.stylesheet">
    <xsl:choose>
      <xsl:when test = "$boost.defaults = 'Boost'">
        <xsl:value-of select = "concat($boost.root, '/doc/src/boostbook.css')"/>
      </xsl:when>
      <xsl:otherwise>
        boostbook.css
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>

  <xsl:param name="boost.graphics.root">
    <xsl:choose>
      <xsl:when test = "$boost.defaults = 'Boost'">
        <xsl:value-of select = "concat($boost.root, '/doc/src/images/')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select = "'images/'"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>

  <xsl:param name="boost.mathjax" select="0"/>
  <xsl:param name="boost.mathjax.script"
             select="'http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML'"/>
  <!--See usage below for explanation of this param-->
  <xsl:param name="boost.noexpand.chapter.toc" select="0"/>

  <xsl:param name="admon.style"/>
  <xsl:param name="admon.graphics">1</xsl:param>
  <xsl:param name="boostbook.verbose" select="0"/>
  <xsl:param name="navig.graphics" select="1"/>
  <xsl:param name="navig.graphics.extension" select="'.png'"/>
  <xsl:param name="chapter.autolabel" select="1"/>
  <xsl:param name="use.id.as.filename" select="1"/>
  <xsl:param name="refentry.generate.name" select="0"/>
  <xsl:param name="refentry.generate.title" select="1"/>
  <xsl:param name="make.year.ranges" select="1"/>
  <xsl:param name="generate.manifest" select="1"/>
  <xsl:param name="generate.section.toc.level" select="3"/>
  <xsl:param name="doc.standalone">false</xsl:param>
  <xsl:param name="chunker.output.indent">yes</xsl:param>
  <xsl:param name="chunker.output.encoding">US-ASCII</xsl:param>
  <xsl:param name="chunk.quietly" select="not(number($boostbook.verbose))"/>
  <xsl:param name="toc.max.depth">2</xsl:param>
  <xsl:param name="callout.graphics.number.limit">15</xsl:param>
  <xsl:param name = "admon.graphics.path" select="$boost.graphics.root" />
  <xsl:param name = "navig.graphics.path" select="$boost.graphics.root" />
  <xsl:param name = "callout.graphics.path"
            select = "concat($boost.graphics.root, 'callouts/')"/>


  <xsl:param name="admon.style">
    <!-- Remove the style. Let the CSS do the styling -->
</xsl:param>

<!-- Always have graphics -->
<xsl:param name="admon.graphics" select="1"/>

  <xsl:param name="generate.toc">
appendix  toc,title
article/appendix  nop
article   toc,title
book      toc,title
chapter   toc,title
part      toc,title
preface   toc,title
qandadiv  toc
qandaset  toc
reference toc,title
sect1     toc
sect2     toc
sect3     toc
sect4     toc
sect5     toc
section   toc
set       toc,title
  </xsl:param>


  <xsl:template name="format.cvs.revision">
    <xsl:param name="text"/>

    <!-- Remove the "$Date: " -->
    <xsl:variable name="text.noprefix"
      select="substring-after($text, '$Date: ')"/>

    <!-- Grab the year -->
    <xsl:variable name="year" select="substring-before($text.noprefix, '/')"/>
    <xsl:variable name="text.noyear"
      select="substring-after($text.noprefix, '/')"/>

    <!-- Grab the month -->
    <xsl:variable name="month" select="substring-before($text.noyear, '/')"/>
    <xsl:variable name="text.nomonth"
      select="substring-after($text.noyear, '/')"/>

    <!-- Grab the year -->
    <xsl:variable name="day" select="substring-before($text.nomonth, ' ')"/>
    <xsl:variable name="text.noday"
      select="substring-after($text.nomonth, ' ')"/>

    <!-- Get the time -->
    <xsl:variable name="time" select="substring-before($text.noday, ' ')"/>

    <xsl:variable name="month.name">
      <xsl:choose>
        <xsl:when test="$month=1">January</xsl:when>
        <xsl:when test="$month=2">February</xsl:when>
        <xsl:when test="$month=3">March</xsl:when>
        <xsl:when test="$month=4">April</xsl:when>
        <xsl:when test="$month=5">May</xsl:when>
        <xsl:when test="$month=6">June</xsl:when>
        <xsl:when test="$month=7">July</xsl:when>
        <xsl:when test="$month=8">August</xsl:when>
        <xsl:when test="$month=9">September</xsl:when>
        <xsl:when test="$month=10">October</xsl:when>
        <xsl:when test="$month=11">November</xsl:when>
        <xsl:when test="$month=12">December</xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:value-of select="concat($month.name, ' ', $day, ', ', $year, ' at ',
                                 $time, ' GMT')"/>
  </xsl:template>


  <xsl:template name="format.svn.revision">
    <xsl:param name="text"/>

    <!-- Remove the "$Date: " or "$Date:: " -->
    <xsl:variable name="text.noprefix"
      select="substring-after($text, ': ')"/>

    <!-- Grab the year -->
    <xsl:variable name="year" select="substring-before($text.noprefix, '-')"/>
    <xsl:variable name="text.noyear"
      select="substring-after($text.noprefix, '-')"/>

    <!-- Grab the month -->
    <xsl:variable name="month" select="substring-before($text.noyear, '-')"/>
    <xsl:variable name="text.nomonth"
      select="substring-after($text.noyear, '-')"/>

    <!-- Grab the year -->
    <xsl:variable name="day" select="substring-before($text.nomonth, ' ')"/>
    <xsl:variable name="text.noday"
      select="substring-after($text.nomonth, ' ')"/>

    <!-- Get the time -->
    <xsl:variable name="time" select="substring-before($text.noday, ' ')"/>
    <xsl:variable name="text.notime"
      select="substring-after($text.noday, ' ')"/>

    <!-- Get the timezone -->
    <xsl:variable name="timezone" select="substring-before($text.notime, ' ')"/>

    <xsl:variable name="month.name">
      <xsl:choose>
        <xsl:when test="$month=1">January</xsl:when>
        <xsl:when test="$month=2">February</xsl:when>
        <xsl:when test="$month=3">March</xsl:when>
        <xsl:when test="$month=4">April</xsl:when>
        <xsl:when test="$month=5">May</xsl:when>
        <xsl:when test="$month=6">June</xsl:when>
        <xsl:when test="$month=7">July</xsl:when>
        <xsl:when test="$month=8">August</xsl:when>
        <xsl:when test="$month=9">September</xsl:when>
        <xsl:when test="$month=10">October</xsl:when>
        <xsl:when test="$month=11">November</xsl:when>
        <xsl:when test="$month=12">December</xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:value-of select="concat($month.name, ' ', $day, ', ', $year)"/>
    <xsl:if test="$time != ''">
      <xsl:value-of select="concat(' at ', $time, ' ', $timezone)"/>
    </xsl:if>
  </xsl:template>

  <!-- Footer Copyright -->
  <xsl:template match="copyright" mode="boost.footer">
    <xsl:if test="position() &gt; 1">
      <br/>
    </xsl:if>
    <xsl:call-template name="gentext">
      <xsl:with-param name="key" select="'Copyright'"/>
    </xsl:call-template>
    <xsl:call-template name="gentext.space"/>
    <xsl:call-template name="dingbat">
      <xsl:with-param name="dingbat">copyright</xsl:with-param>
    </xsl:call-template>
    <xsl:call-template name="gentext.space"/>
    <xsl:call-template name="copyright.years">
      <xsl:with-param name="years" select="year"/>
      <xsl:with-param name="print.ranges" select="$make.year.ranges"/>
      <xsl:with-param name="single.year.ranges"
        select="$make.single.year.ranges"/>
    </xsl:call-template>
    <xsl:call-template name="gentext.space"/>
    <xsl:apply-templates select="holder" mode="titlepage.mode"/>
  </xsl:template>

  <!-- Footer License -->
  <xsl:template match="legalnotice" mode="boost.footer">
    <xsl:apply-templates select="para" mode="titlepage.mode" />
  </xsl:template>

  <xsl:template name="user.footer.content">
    <table width="100%">
      <tr>
        <td align="left">
          <xsl:variable name="revision-nodes"
            select="ancestor-or-self::*
                    [not (attribute::rev:last-revision='')]"/>
          <xsl:if test="count($revision-nodes) &gt; 0">
            <xsl:variable name="revision-node"
              select="$revision-nodes[last()]"/>
            <xsl:variable name="revision-text">
              <xsl:value-of
                select="normalize-space($revision-node/attribute::rev:last-revision)"/>
            </xsl:variable>
            <xsl:if test="string-length($revision-text) &gt; 0">
              <p>
                <small>
                  <xsl:text>Last revised: </xsl:text>
                  <xsl:choose>
                    <xsl:when test="contains($revision-text, '/')">
                      <xsl:call-template name="format.cvs.revision">
                        <xsl:with-param name="text" select="$revision-text"/>
                      </xsl:call-template>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:call-template name="format.svn.revision">
                        <xsl:with-param name="text" select="$revision-text"/>
                      </xsl:call-template>
                    </xsl:otherwise>
                  </xsl:choose>
                </small>
              </p>
            </xsl:if>
          </xsl:if>
        </td>
        <td align="right">
          <div class = "copyright-footer">
            <xsl:apply-templates select="ancestor::*/*/copyright"
              mode="boost.footer"/>
            <xsl:apply-templates select="ancestor::*/*/legalnotice"
              mode="boost.footer"/>
          </div>
        </td>
      </tr>
    </table>
  </xsl:template>

  <!-- We don't want refentry's to show up in the TOC because they
       will merely be redundant with the synopsis. -->
  <xsl:template match="refentry" mode="toc"/>

  <!-- override the behaviour of some DocBook elements for better
       rendering facilities -->

  <xsl:template match = "programlisting[ancestor::informaltable]">
     <pre class = "table-{name(.)}"><xsl:apply-templates/></pre>
  </xsl:template>

  <xsl:template match = "refsynopsisdiv">
     <h2 class = "{name(.)}-title">Synopsis</h2>
     <div class = "{name(.)}">
        <xsl:apply-templates/>
     </div>
  </xsl:template>

  <xsl:template name="generate.html.title"/>

  <xsl:template match="*" mode="detect-math">
    <xsl:variable name="is-chunk">
      <xsl:call-template name="chunk"/>
    </xsl:variable>
    <xsl:if test="$is-chunk = '0'">
      <xsl:apply-templates mode="detect-math"/>
    </xsl:if>
  </xsl:template>
  
  <xsl:template match="text()" mode="detect-math"/>
  
  <xsl:template match="textobject[@role='tex']" mode="detect-math">
    <xsl:text>1</xsl:text>
  </xsl:template>
  
  <xsl:template name="user.head.content">
    <xsl:if test="$boost.mathjax = 1">
      <xsl:variable name="has-math">
        <xsl:apply-templates mode="detect-math" select="*"/>
      </xsl:variable>
      <xsl:if test="string($has-math) != ''">
        <script type="text/javascript" src="{$boost.mathjax.script}"/>
      </xsl:if>
    </xsl:if>
  </xsl:template>
  
  <xsl:template match="inlinemediaobject">
    <xsl:choose>
      <xsl:when test="$boost.mathjax = 1 and textobject[@role='tex']">
        <xsl:variable name="content" select="string(textobject[@role='tex'])"/>
        <xsl:variable name="plain-content">
          <xsl:choose>
            <!--strip $$-->
            <xsl:when test="substring($content, 1, 1) = '$' and
                            substring($content, string-length($content), 1) = '$'">
              <xsl:value-of select="substring($content, 2, string-length($content) - 2)"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$content"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <script type="math/tex">
          <xsl:value-of select="$plain-content"/>
        </script>
        <noscript>
          <xsl:apply-imports/>
        </noscript>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-imports/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
<!-- ============================================================ -->

<xsl:template name="output.html.stylesheets">
    <xsl:param name="stylesheets" select="''"/>

    <xsl:choose>
        <xsl:when test="contains($stylesheets, ' ')">
            <link rel="stylesheet">
                <xsl:attribute name="href">
                    <xsl:call-template name="href.target.relative">
                        <xsl:with-param name="target" select="substring-before($stylesheets, ' ')"/>
                    </xsl:call-template>
                </xsl:attribute>
                <xsl:if test="$html.stylesheet.type != ''">
                    <xsl:attribute name="type">
                        <xsl:value-of select="$html.stylesheet.type"/>
                    </xsl:attribute>
                </xsl:if>
            </link>
            <xsl:call-template name="output.html.stylesheets">
                <xsl:with-param name="stylesheets" select="substring-after($stylesheets, ' ')"/>
            </xsl:call-template>
        </xsl:when>
        <xsl:when test="$stylesheets != ''">
            <link rel="stylesheet">
                <xsl:attribute name="href">
                    <xsl:call-template name="href.target.relative">
                        <xsl:with-param name="target" select="$stylesheets"/>
                    </xsl:call-template>
                </xsl:attribute>
                <xsl:if test="$html.stylesheet.type != ''">
                    <xsl:attribute name="type">
                        <xsl:value-of select="$html.stylesheet.type"/>
                    </xsl:attribute>
                </xsl:if>
            </link>
        </xsl:when>
    </xsl:choose>
</xsl:template>

<xsl:template match="itemizedlist[@role = 'index']" mode="class.value">
   <xsl:value-of select="'index'"/>
</xsl:template>

<xsl:template match="preface|chapter|appendix|article" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <!--
      When boost.noexpand.chapter.toc is set to 1, then the TOC for
      chapters is only one level deep (ie toc.max.depth has no effect)
      and nested sections within chapters are not shown.  TOC's and LOC's 
      at other levels are not effected and respond to toc.max.depth as normal.
  -->
  <xsl:choose>
    <xsl:when test="local-name($toc-context) = 'book' and $boost.noexpand.chapter.toc = 1">
      <xsl:call-template name="subtoc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
        <xsl:with-param name="nodes" select="foo"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="subtoc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
        <xsl:with-param name="nodes"
              select="section|sect1|glossary|bibliography|index
                     |bridgehead[$bridgehead.in.toc != 0]"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>


