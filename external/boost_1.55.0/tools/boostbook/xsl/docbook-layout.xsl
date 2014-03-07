<?xml version = "1.0" encoding = "utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->

<xsl:stylesheet version = "1.0"
   xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
>

  <!-- needed for calsTable template -->
  
  <xsl:import
    href="http://docbook.sourceforge.net/release/xsl/current/html/formal.xsl"/>

  <!-- Optionally add the section id to each section's class.
       This is useful if you want to style individual sections differently. -->
  <xsl:param name="boost.section.class.add.id" select="0"/>

  <!--
     Override the behaviour of some DocBook elements for better
     integration with the new look & feel.
  -->

  <xsl:template match = "programlisting[ancestor::informaltable]">
     <pre class = "table-{name(.)}"><xsl:apply-templates/></pre>
  </xsl:template>

  <xsl:template match = "refsynopsisdiv">
     <h2 class = "{name(.)}-title">Synopsis</h2>
     <div class = "{name(.)}"><xsl:apply-templates/></div>
  </xsl:template>

  <!-- table: remove border = '1' -->

  <xsl:template match = "table|informaltable">
     <xsl:choose>
        <xsl:when test = "self::table and tgroup|mediaobject|graphic">
           <xsl:apply-imports/>
        </xsl:when><xsl:when test = "self::informaltable and tgroup|mediaobject|graphic">
           <xsl:call-template name = "informal.object">
              <xsl:with-param name = "class"><xsl:choose>
                 <xsl:when test = "@tabstyle">
                    <xsl:value-of select = "@tabstyle"/>
                 </xsl:when><xsl:otherwise>
                    <xsl:value-of select = "local-name(.)"/>
                 </xsl:otherwise>
              </xsl:choose></xsl:with-param>
           </xsl:call-template>
        </xsl:when><xsl:otherwise>
           <table class = "table"><xsl:copy-of select = "@*[not(local-name(.)='border')]"/>
              <xsl:call-template name = "htmlTable"/>
           </table>
        </xsl:otherwise>
     </xsl:choose>
  </xsl:template>

  <xsl:template match = "tgroup" name = "tgroup">
     <xsl:variable name="summary"><xsl:call-template name="dbhtml-attribute">
        <xsl:with-param name="pis" select="processing-instruction('dbhtml')"/>
        <xsl:with-param name="attribute" select="'table-summary'"/>
     </xsl:call-template></xsl:variable>

     <xsl:variable name="cellspacing"><xsl:call-template name="dbhtml-attribute">
        <xsl:with-param name="pis" select="processing-instruction('dbhtml')"/>
        <xsl:with-param name="attribute" select="'cellspacing'"/>
     </xsl:call-template></xsl:variable>

     <xsl:variable name="cellpadding"><xsl:call-template name="dbhtml-attribute">
        <xsl:with-param name="pis" select="processing-instruction('dbhtml')[1]"/>
        <xsl:with-param name="attribute" select="'cellpadding'"/>
     </xsl:call-template></xsl:variable>

     <table class = "table">
        <xsl:choose>
           <xsl:when test="../textobject/phrase">
              <xsl:attribute name="summary">
                 <xsl:value-of select="../textobject/phrase"/>
              </xsl:attribute>
           </xsl:when><xsl:when test="$summary != ''">
              <xsl:attribute name="summary">
                 <xsl:value-of select="$summary"/>
              </xsl:attribute>
           </xsl:when><xsl:when test="../title">
              <xsl:attribute name="summary">
                 <xsl:value-of select="string(../title)"/>
              </xsl:attribute>
           </xsl:when>
           <xsl:otherwise/>
        </xsl:choose><xsl:if test="$cellspacing != '' or $html.cellspacing != ''">
           <xsl:attribute name="cellspacing"><xsl:choose>
              <xsl:when test="$cellspacing != ''"><xsl:value-of select="$cellspacing"/></xsl:when>
              <xsl:otherwise><xsl:value-of select="$html.cellspacing"/></xsl:otherwise>
           </xsl:choose></xsl:attribute>
        </xsl:if><xsl:if test="$cellpadding != '' or $html.cellpadding != ''">
           <xsl:attribute name="cellpadding"><xsl:choose>
              <xsl:when test="$cellpadding != ''"><xsl:value-of select="$cellpadding"/></xsl:when>
              <xsl:otherwise><xsl:value-of select="$html.cellpadding"/></xsl:otherwise>
           </xsl:choose></xsl:attribute>
        </xsl:if><xsl:if test="../@pgwide=1">
           <xsl:attribute name="width">100%</xsl:attribute>
        </xsl:if>

        <xsl:variable name="colgroup">
           <colgroup><xsl:call-template name="generate.colgroup">
              <xsl:with-param name="cols" select="@cols"/>
           </xsl:call-template></colgroup>
        </xsl:variable>

        <xsl:variable name="explicit.table.width"><xsl:call-template name="dbhtml-attribute">
           <xsl:with-param name="pis" select="../processing-instruction('dbhtml')[1]"/>
           <xsl:with-param name="attribute" select="'table-width'"/>
        </xsl:call-template></xsl:variable>

        <xsl:variable name="table.width"><xsl:choose>
           <xsl:when test="$explicit.table.width != ''">
              <xsl:value-of select="$explicit.table.width"/>
           </xsl:when><xsl:when test="$default.table.width = ''">
              <xsl:text>100%</xsl:text>
           </xsl:when><xsl:otherwise>
              <xsl:value-of select="$default.table.width"/>
           </xsl:otherwise>
        </xsl:choose></xsl:variable>

        <xsl:if test="$default.table.width != '' or $explicit.table.width != ''">
           <xsl:attribute name="width"><xsl:choose>
              <xsl:when test="contains($table.width, '%')">
                 <xsl:value-of select="$table.width"/>
              </xsl:when><xsl:when test="$use.extensions != 0 and $tablecolumns.extension != 0">
                 <xsl:choose>
                    <xsl:when test="function-available('stbl:convertLength')">
                       <xsl:value-of select="stbl:convertLength($table.width)"/>
                    </xsl:when><xsl:when test="function-available('xtbl:convertLength')">
                       <xsl:value-of select="xtbl:convertLength($table.width)"/>
                    </xsl:when><xsl:otherwise>
                       <xsl:message terminate="yes">
                          <xsl:text>No convertLength function available.</xsl:text>
                       </xsl:message>
                    </xsl:otherwise>
                 </xsl:choose>
              </xsl:when><xsl:otherwise>
                 <xsl:value-of select="$table.width"/>
              </xsl:otherwise>
           </xsl:choose></xsl:attribute>
        </xsl:if>

        <xsl:choose>
           <xsl:when test="$use.extensions != 0 and $tablecolumns.extension != 0">
              <xsl:choose>
                 <xsl:when test="function-available('stbl:adjustColumnWidths')">
                    <xsl:copy-of select="stbl:adjustColumnWidths($colgroup)"/>
                 </xsl:when><xsl:when test="function-available('xtbl:adjustColumnWidths')">
                    <xsl:copy-of select="xtbl:adjustColumnWidths($colgroup)"/>
                 </xsl:when><xsl:when test="function-available('ptbl:adjustColumnWidths')">
                    <xsl:copy-of select="ptbl:adjustColumnWidths($colgroup)"/>
                 </xsl:when><xsl:otherwise>
                    <xsl:message terminate="yes">
                       <xsl:text>No adjustColumnWidths function available.</xsl:text>
                    </xsl:message>
                 </xsl:otherwise>
              </xsl:choose>
           </xsl:when><xsl:otherwise>
              <xsl:copy-of select="$colgroup"/>
           </xsl:otherwise>
        </xsl:choose>

        <xsl:apply-templates select="thead"/>
        <xsl:apply-templates select="tfoot"/>
        <xsl:apply-templates select="tbody"/>

        <xsl:if test=".//footnote"><tbody class="footnotes">
           <tr><td colspan="{@cols}">
              <xsl:apply-templates select=".//footnote" mode="table.footnote.mode"/>
           </td></tr>
        </tbody></xsl:if>
     </table>
  </xsl:template>

  <!-- table of contents 
       
    The standard Docbook template selects, amoung others,
    the 'refentry' element for inclusion in TOC. In some
    cases, this creates empty TOC. The most possible reason
    is that there's 'refentry' element without 'refentrytitle',
    but it's a mistery why it occurs. Even if we fix that
    problem, we'll get non-empty TOC where no TOC is desired
    (e.g. for section corresponding to each header file in
    library doc). So, don't bother for now.
  -->

  <xsl:template name="section.toc">
     <xsl:param name="toc-context" select="."/>
     <xsl:param name="toc.title.p" select="true()"/>

     <xsl:call-template name="make.toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
        <xsl:with-param name="toc.title.p" select="$toc.title.p"/>
        <xsl:with-param name="nodes" select="
           section|sect1|sect2|sect3|sect4|sect5|
           bridgehead[$bridgehead.in.toc != 0]
        "/>
     </xsl:call-template>
  </xsl:template>

  <!-- When there is both a title and a caption for a table, only use the 
       title. -->
  <xsl:template match="table" mode="title.markup">
    <xsl:param name="allow-anchors" select="0"/>
    <xsl:apply-templates select="(title|caption)[1]" mode="title.markup">
      <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
    </xsl:apply-templates>
  </xsl:template>
  
  
  <!-- Adds role class for section element resulting div. So that
       we can style them in the resulting HTML.
       Also, add the section id, if boost.section.class.add.id = 1.
       This can be used to style individual sections differently. -->
  <xsl:template match="section" mode="class.value">
    <xsl:param name="class" select="local-name(.)"/>
    <xsl:param name="node" select="."/>
    <xsl:variable name="id">
      <xsl:if test="$boost.section.class.add.id">
        <xsl:call-template name="object.id">
          <xsl:with-param name="object" select="$node"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:value-of select="normalize-space(concat($class, ' ',
        @role, ' ', translate($id, '.', '_')))"/>
  </xsl:template>
  
  <!-- Adds role class for simplesect element resulting div. So that
       we can style them in the resulting HTML. -->
  <xsl:template match="simplesect" mode="class.value">
    <xsl:param name="class" select="local-name(.)"/>
    <xsl:param name="node" select="."/>
    <xsl:value-of select="normalize-space(concat($class,' ',@role))"/>
  </xsl:template>
  
  <!-- Allow for specifying that a section should not include the parents
       labeling. This allows us to start clean numering of a sub-section. -->
  <xsl:template match="section[@label-style='no-parent']" mode="label.markup">
	  <xsl:choose>
	    <xsl:when test="@label">
	      <xsl:value-of select="@label"/>
	    </xsl:when>
	    <xsl:when test="$label != 0">
	      <xsl:variable name="format">
	        <xsl:call-template name="autolabel.format">
	          <xsl:with-param name="format" select="$section.autolabel"/>
	        </xsl:call-template>
	      </xsl:variable>
	      <xsl:number format="{$format}" count="section"/>
	    </xsl:when>
	  </xsl:choose>
  </xsl:template>
  
</xsl:stylesheet>
