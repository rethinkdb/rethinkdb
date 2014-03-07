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
    href="http://docbook.sourceforge.net/release/xsl/current/html/chunk.xsl"/>
  <xsl:import
    href="http://docbook.sourceforge.net/release/xsl/current/html/math.xsl"/>

  <!-- Bring in the fast chunking overrides.  There's nothing
       that we need to override, so include instead of importing it. -->
  <xsl:include
    href="http://docbook.sourceforge.net/release/xsl/current/html/chunkfast.xsl"/>
  
  <!-- We have to make sure that our templates override all
       docbook templates.  Therefore, we include our own templates
       instead of importing them.  In order for this to work,
       the stylesheets included here cannot also include each other -->
  <xsl:include href="chunk-common.xsl"/>
  <xsl:include href="docbook-layout.xsl"/>
  <xsl:include href="navbar.xsl"/>
  <xsl:include href="admon.xsl"/>
  <xsl:include href="xref.xsl"/>
  <xsl:include href="relative-href.xsl"/>
  <xsl:include href="callout.xsl"/>
  <xsl:include href="html-base.xsl"/>

</xsl:stylesheet>
