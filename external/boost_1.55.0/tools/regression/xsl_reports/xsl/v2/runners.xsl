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
    extension-element-prefixes="exsl"
    version="1.0">

  <xsl:output method="html"/>

  <xsl:template match="/">
    <html>
      <body bgcolor="#FFFFFF">
        <xsl:apply-templates/>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="test-run">
    <table>
      <tr>
        <td>
            <xsl:message>Writing runner document <xsl:value-of select="@runner"/></xsl:message>
          <a href="{@runner}.html"><xsl:value-of select="@runner"/></a>
          <exsl:document href="{@runner}.html"
            method="html" 
            doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
            encoding="utf-8"
            indent="yes">
            <html>
              <head>
                <title><xsl:value-of select="@runner"/></title>
              </head>
              <body>
                <h1><xsl:value-of select="@runner"/></h1>
                <hr></hr>
                <xsl:value-of select="comment/text()" disable-output-escaping="yes"/>
              </body>
            </html>
          </exsl:document>
        </td>     
      </tr>
    </table>    
  </xsl:template>

</xsl:stylesheet>

