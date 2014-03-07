<?xml version="1.0" encoding="utf-8"?>
<!--

Copyright MetaCommunications, Inc. 2006.

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)

-->

<xsl:stylesheet
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:meta="http://www.meta-comm.com"
    exclude-result-prefixes="meta"
    version="1.0">

    <xsl:import href="common.xsl"/>

    <xsl:output method="xml" encoding="utf-8"/>
  
    <xsl:template match="/">
        <xsl:for-each select="expected-failures/toolset">
            <xsl:sort select="@name"/>
            <xsl:value-of select="@name"/>
            <xsl:if test="count(./toolset-alias)">
                <xsl:text> aka </xsl:text>
                <xsl:for-each select="toolset-alias">
                    <xsl:sort select="@name"/>
                    <xsl:value-of select="@name"/>
                    <xsl:text> </xsl:text>
                </xsl:for-each>        
            </xsl:if>
<xsl:text>
</xsl:text>
        </xsl:for-each>
    </xsl:template>

</xsl:stylesheet>
