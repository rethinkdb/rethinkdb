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

  <!-- Import the HTML stylesheet -->
  <xsl:import 
    href="http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl"/>
  <xsl:import
    href="http://docbook.sourceforge.net/release/xsl/current/html/math.xsl"/>

  <xsl:output method="html" encoding="US-ASCII" indent="no"/>
  
  <!-- We have to make sure that our templates override all
       docbook templates.  Therefore, we include our own templates
       instead of importing them.  In order for this to work,
       the stylesheets included here cannot also include each other -->
  <xsl:include href="docbook-layout.xsl"/>
  <xsl:include href="admon.xsl"/>
  <xsl:include href="xref.xsl"/>
  <xsl:include href="relative-href.xsl"/>
  <xsl:include href="callout.xsl"/>

  <xsl:param name="admon.style"/>
  <xsl:param name="admon.graphics">1</xsl:param>
  <xsl:param name="chapter.autolabel" select="0"/>
  <xsl:param name="refentry.generate.name" select="0"/>
  <xsl:param name="refentry.generate.title" select="1"/>
  <xsl:param name="make.year.ranges" select="1"/>

</xsl:stylesheet>
