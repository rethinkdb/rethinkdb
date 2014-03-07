<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:template name="print.warning.context">
    <xsl:message>
      <xsl:text>  In </xsl:text>
      <xsl:call-template name="fully-qualified-name">
        <xsl:with-param name="node" select="."/>
      </xsl:call-template>
    </xsl:message>
  </xsl:template>
</xsl:stylesheet>
