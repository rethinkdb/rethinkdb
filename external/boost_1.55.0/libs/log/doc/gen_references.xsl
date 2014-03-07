<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!--
              Copyright Andrey Semashev 2007 - 2013.
     Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
              http://www.boost.org/LICENSE_1_0.txt)

    This stylesheet extracts information about headers, classes, etc.
    from the Doxygen-generated reference documentation and writes
    it as QuickBook templates that refer to the according Reference sections.
-->
<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>
<xsl:template match="/library-reference">
<xsl:text disable-output-escaping="yes">[/
              Copyright Andrey Semashev 2007 - 2013.
     Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
              http://www.boost.org/LICENSE_1_0.txt)

    This document is a part of Boost.Log library documentation.

     This document was automatically generated,  DO NOT EDIT!
/]
</xsl:text>
<xsl:apply-templates>
<xsl:with-param name="namespace"/>
<xsl:with-param name="enclosing_namespace"/>
</xsl:apply-templates>
</xsl:template>

<!-- Skip any text nodes -->
<xsl:template match="text()"/>

<!-- Headers -->
<xsl:template match="header">
<xsl:param name="namespace"/>
<xsl:param name="enclosing_namespace"/>
[template <xsl:value-of select="translate(@name, '/.', '__')"/>[][headerref <xsl:value-of select="@name"/>]]
<xsl:apply-templates>
<xsl:with-param name="namespace" select="$namespace"/>
<xsl:with-param name="enclosing_namespace" select="$enclosing_namespace"/>
</xsl:apply-templates>
</xsl:template>

<!-- Namespaces - only needed to construct fully qualified class names -->
<xsl:template match="namespace">
<xsl:param name="namespace"/>
<xsl:param name="enclosing_namespace"/>
<xsl:variable name="namespace_prefix">
<xsl:value-of select="$namespace"/><xsl:if test="string-length($namespace) &gt; 0"><xsl:text>::</xsl:text></xsl:if>
</xsl:variable>
<xsl:apply-templates>
<xsl:with-param name="namespace" select="concat($namespace_prefix, @name)"/>
<xsl:with-param name="enclosing_namespace" select="@name"/>
</xsl:apply-templates>
</xsl:template>

<!-- Classses -->
<xsl:template match="class|struct">
<xsl:param name="namespace"/>
<xsl:param name="enclosing_namespace"/>
[template <xsl:value-of select="concat('class_', $enclosing_namespace, '_', @name)"/>[][classref <xsl:value-of select="concat($namespace, '::', @name)"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>]]
<xsl:apply-templates>
<xsl:with-param name="namespace" select="concat($namespace, '::', @name)"/>
<xsl:with-param name="enclosing_namespace" select="concat($enclosing_namespace, '_', @name)"/>
</xsl:apply-templates>
</xsl:template>

<!-- Free functions - currently disabled because multiple overloads generate duplicate QuickBook templates -->
<!--
<xsl:template match="function">
<xsl:param name="namespace"/>
<xsl:param name="enclosing_namespace"/>
[template <xsl:value-of select="concat('func_', $enclosing_namespace, '_', @name)"/>[][funcref <xsl:value-of select="concat($namespace, '::', @name)"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>]]
</xsl:template>
-->

</xsl:transform>
