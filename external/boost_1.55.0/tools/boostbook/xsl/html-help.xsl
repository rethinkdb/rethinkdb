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

  <!-- Import the HTML chunking stylesheet -->
  <xsl:import
    href="http://docbook.sourceforge.net/release/xsl/current/htmlhelp/htmlhelp.xsl"/>

  <xsl:param name="admon.style"/>
  <xsl:param name="admon.graphics">1</xsl:param>
  <xsl:param name="boostbook.verbose" select="0"/>
  <xsl:param name="html.stylesheet" select="'boostbook.css'"/>
  <xsl:param name="chapter.autolabel" select="1"/>
  <xsl:param name="use.id.as.filename" select="1"/>
  <xsl:param name="refentry.generate.name" select="0"/>
  <xsl:param name="refentry.generate.title" select="1"/>
  <xsl:param name="make.year.ranges" select="1"/>
  <xsl:param name="generate.manifest" select="1"/>
  <xsl:param name="callout.graphics.number.limit">15</xsl:param>
  <xsl:param name="draft.mode">no</xsl:param>
  <xsl:param name="admon.graphics" select="1"/>

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

</xsl:stylesheet>
