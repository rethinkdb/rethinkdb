<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
   Copyright (c) 2007 Frank Mori Hess <fmhess@users.sourceforge.net>

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:include href="global.xsl"/>
  <xsl:strip-space elements="inherit purpose"/>

  <!-- When true, the stylesheet will emit compact definitions of
       enumerations when the enumeration does not have any detailed
       description. A compact definition renders the enum definition along
       with a comment for the purpose of the enum (if it exists) directly
       within the synopsis. A non-compact definition will create a
       separate refentry element for the enum. -->
  <xsl:param name="boost.compact.enum">1</xsl:param>

  <!-- When true, the stylesheet will emit compact definitions of
       typedefs when the typedef does not have any detailed
       description. -->
  <xsl:param name="boost.compact.typedef">1</xsl:param>

  <xsl:template match="class|struct|union" mode="generate.id">
    <xsl:call-template name="fully-qualified-id">
      <xsl:with-param name="node" select="."/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="class-specialization|struct-specialization|union-specialization" mode="generate.id">
    <xsl:call-template name="fully-qualified-id">
      <xsl:with-param name="node" select="."/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="typedef" mode="generate.id">
    <xsl:call-template name="fully-qualified-id">
      <xsl:with-param name="node" select="."/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="enum" mode="generate.id">
    <xsl:call-template name="fully-qualified-id">
      <xsl:with-param name="node" select="."/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="enumvalue" mode="generate.id">
    <xsl:call-template name="fully-qualified-id">
      <xsl:with-param name="node" select="parent::enum"/>
    </xsl:call-template>
    <xsl:text>.</xsl:text>
    <xsl:value-of select="@name"/>
  </xsl:template>

  <!-- Display the full name of the current node, e.g., "Class
       template function". -->
  <xsl:template name="type.display.name">
    <xsl:choose>
      <xsl:when test="contains(local-name(.), 'class')">
        <xsl:text>Class </xsl:text>
      </xsl:when>
      <xsl:when test="contains(local-name(.), 'struct')">
        <xsl:text>Struct </xsl:text>
      </xsl:when>
      <xsl:when test="contains(local-name(.), 'union')">
        <xsl:text>Union </xsl:text>
      </xsl:when>
      <xsl:when test="local-name(.)='enum'">
        <xsl:text>Type </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
Unknown type element "<xsl:value-of select="local-name(.)"/>" in type.display.name
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="template and count(template/*) &gt; 0">
      <xsl:text>template </xsl:text>
    </xsl:if>
    <xsl:call-template name="monospaced">
      <xsl:with-param name="text">
        <xsl:value-of select="@name"/>
        <xsl:apply-templates select="specialization"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- Determine the class key for the given node -->
  <xsl:template name="type.class.key">
    <xsl:param name="node" select="."/>
    <xsl:choose>
      <xsl:when test="contains(local-name($node), '-specialization')">
        <xsl:value-of select="substring-before(local-name($node), '-')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="local-name($node)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Emit class synopsis -->
  <xsl:template match="class|class-specialization|
                       struct|struct-specialization|
                       union|union-specialization" mode="synopsis">
    <xsl:param name="indentation"/>

    <!-- The keyword used to declare this class type, e.g., class,
         struct, or union. -->
    <xsl:variable name="class-key">
      <xsl:call-template name="type.class.key"/>
    </xsl:variable>

    <!-- Spacing -->
    <xsl:if test="not (local-name(preceding-sibling::*[position()=1])=local-name(.)) and (position() &gt; 1)">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <xsl:text>&#10;</xsl:text>

    <!-- Calculate how long this declaration would be if we put it all
         on one line -->
    <xsl:variable name="full-decl-string">
      <xsl:apply-templates select="template" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation"/>
        <xsl:with-param name="wrap" select="false()"/>
      </xsl:apply-templates>
      <xsl:value-of select="$class-key"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:apply-templates select="specialization"/>
      <xsl:call-template name="highlight-text">
        <xsl:with-param name="text" select="';'"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="(string-length($full-decl-string) +
                       string-length($indentation)) &lt; $max-columns">
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
        <xsl:apply-templates select="template" mode="synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:apply-templates>

        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="$class-key"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id"/>
          </xsl:with-param>
          <xsl:with-param name="text">
            <xsl:value-of select="@name"/>
          </xsl:with-param>
        </xsl:call-template>
        <xsl:apply-templates select="specialization"/>
        <xsl:call-template name="highlight-special">
          <xsl:with-param name="text" select="';'"/>
        </xsl:call-template>
      </xsl:when>

      <xsl:otherwise>
        <!-- Template header -->
        <xsl:if test="template">
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
          <xsl:apply-templates select="template" mode="synopsis">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:apply-templates>
          <xsl:text>&#10;</xsl:text>

          <!-- Indent class templates' names in the synopsis -->
          <xsl:text>  </xsl:text>
        </xsl:if>

        <!-- Class name -->
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="$class-key"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="string(@name)"/>
        </xsl:call-template>
        <xsl:apply-templates select="specialization"/>
        <xsl:call-template name="highlight-special">
          <xsl:with-param name="text" select="';'"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>

    <!-- Free functions associated with the class -->
    <xsl:apply-templates select="free-function-group" mode="header-synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="class" select="@name"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Emit a typedef synopsis -->
  <xsl:template name="type.typedef.display.aligned">
    <xsl:param name="compact"/>
    <xsl:param name="indentation"/>
    <xsl:param name="is-reference"/>
    <xsl:param name="max-type-length"/>
    <xsl:param name="max-name-length"/>

    <!-- What type of link the typedef name should have. This shall
         be one of 'anchor' (the typedef output will be the target of
         links), 'link' (the typedef output will link to a definition), or
         'none' (the typedef output will not be either a link or a link
         target) -->
    <xsl:param name="link-type">
      <xsl:choose>
        <xsl:when test="$is-reference">
          <xsl:text>anchor</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>link</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:param>

    <!-- The id we should link to or anchor as -->
    <xsl:param name="link-to">
      <xsl:call-template name="generate.id"/>
    </xsl:param>

    <!-- The id we should link to or anchor as -->
    <xsl:param name="typedef-name">
      <xsl:value-of select="@name"/>
    </xsl:param>

    <!-- Padding for the typedef types -->
    <xsl:variable name="type-padding">
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$max-type-length"/>
      </xsl:call-template>
    </xsl:variable>

    <!-- Padding for the typedef names -->
    <xsl:variable name="name-padding">
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$max-name-length"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:text>&#10;</xsl:text>
    <xsl:choose>
      <!-- Create a vertical ellipsis -->
      <xsl:when test="@name = '...'">
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation + 3"/>
        </xsl:call-template>
        <xsl:text>.&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation + 3"/>
        </xsl:call-template>
        <xsl:text>.&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation + 3"/>
        </xsl:call-template>
        <xsl:text>.</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'typedef'"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>

        <!-- Length of the type -->
        <xsl:variable name="type-length">
          <xsl:choose>
            <xsl:when test="@type">
              <xsl:message>
                <xsl:text>Warning: `type' attribute of `typedef' element is deprecated. Use 'type' element instead.</xsl:text>
              </xsl:message>
              <xsl:call-template name="print.warning.context"/>
              <xsl:value-of select="string-length(@type)"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="string-length(type)"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <!-- Emit the type -->
        <xsl:choose>
          <xsl:when test="@type">
            <xsl:value-of select="@type"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="type/*|type/text()"
              mode="highlight"/>
            <!--
            <xsl:call-template name="source-highlight">
              <xsl:with-param name="text">
                <xsl:apply-templates select="type/*|type/text()"/>
              </xsl:with-param>
            </xsl:call-template>
            -->
          </xsl:otherwise>
        </xsl:choose>

        <xsl:choose>
          <xsl:when test="$max-type-length &gt; 0">
            <xsl:value-of select="substring($type-padding, 1,
                                            $max-type-length - $type-length)"/>
            <xsl:text> </xsl:text>
            <xsl:variable name="truncated-typedef-name" select="substring(@name,
              1, $max-name-length)"/>
            <xsl:call-template name="link-or-anchor">
              <xsl:with-param name="to" select="$link-to"/>
              <xsl:with-param name="text" select="$truncated-typedef-name"/>
              <xsl:with-param name="link-type" select="$link-type"/>
              <xsl:with-param name="highlight" select="true()"/>
            </xsl:call-template>
            <xsl:call-template name="highlight-text">
              <xsl:with-param name="text" select="substring(concat(';', $name-padding),
                1, $max-name-length - string-length($truncated-typedef-name))"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text> </xsl:text>
            <xsl:call-template name="link-or-anchor">
              <xsl:with-param name="to" select="$link-to"/>
              <xsl:with-param name="text" select="$typedef-name"/>
              <xsl:with-param name="link-type" select="$link-type"/>
              <xsl:with-param name="highlight" select="true()"/>
            </xsl:call-template>
            <xsl:call-template name="highlight-special">
              <xsl:with-param name="text" select="';'"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>

        <xsl:if test="$compact and purpose">
          <xsl:text>  </xsl:text>
          <xsl:call-template name="highlight-comment">
            <xsl:with-param name="text">
              <xsl:text>// </xsl:text>
              <xsl:apply-templates select="purpose" mode="comment"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="typedef" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:param name="max-type-length" select="0"/>
    <xsl:param name="max-name-length" select="0"/>
    <xsl:param name="allow-anchor" select="true()"/>

    <!-- True if we should compact this typedef -->
    <xsl:variable name="compact"
      select="not (para|description) and ($boost.compact.typedef='1')"/>

    <xsl:choose>
      <xsl:when test="$compact">
        <!-- Spacing -->
        <xsl:if test="not (local-name(preceding-sibling::*[position()=1])=local-name(.)) and (position() &gt; 1)">
          <xsl:text>&#10;</xsl:text>
        </xsl:if>

        <xsl:call-template name="type.typedef.display.aligned">
          <xsl:with-param name="compact" select="$compact"/>
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="is-reference" select="$allow-anchor"/>
          <xsl:with-param name="max-type-length" select="$max-type-length"/>
          <xsl:with-param name="max-name-length" select="$max-name-length"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="type.typedef.display.aligned">
          <xsl:with-param name="compact" select="$compact"/>
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="is-reference" select="false()"/>
          <xsl:with-param name="max-type-length" select="$max-type-length"/>
          <xsl:with-param name="max-name-length" select="$max-name-length"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Emit a typedef reference entry -->
  <xsl:template match="typedef" mode="namespace-reference">
    <!-- True if this typedef was compacted -->
    <xsl:variable name="compact"
      select="not (para|description) and ($boost.compact.typedef='1')"/>

    <xsl:if test="not ($compact)">
      <xsl:call-template name="reference-documentation">
        <xsl:with-param name="refname" select="@name"/>
        <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
        <xsl:with-param name="anchor">
          <xsl:call-template name="generate.id"/>
        </xsl:with-param>
        <xsl:with-param name="name">
          <xsl:text>Type definition </xsl:text>
          <xsl:call-template name="monospaced">
            <xsl:with-param name="text" select="@name"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="synopsis">
          <xsl:call-template name="header-link"/>
          <xsl:call-template name="type.typedef.display.aligned">
            <xsl:with-param name="compact" select="false()"/>
            <xsl:with-param name="indentation" select="0"/>
            <xsl:with-param name="is-reference" select="true()"/>
            <xsl:with-param name="link-type" select="'none'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="text">
          <xsl:apply-templates select="description"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="typedef" mode="reference">
    <!-- True if this typedef was compacted -->
    <xsl:variable name="compact"
      select="not (para|description) and ($boost.compact.typedef='1')"/>

    <xsl:if test="not ($compact)">
      <listitem>
        <para>
          <xsl:call-template name="type.typedef.display.aligned">
            <xsl:with-param name="compact" select="false()"/>
            <xsl:with-param name="indentation" select="0"/>
            <xsl:with-param name="is-reference" select="true()"/>
            <xsl:with-param name="link-type" select="'anchor'"/>
          </xsl:call-template>
        </para>
        <xsl:apply-templates select="description"/>
      </listitem>
    </xsl:if>
  </xsl:template>

  <!-- Emit a list of static constants -->
  <xsl:template match="static-constant" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:text>&#10;</xsl:text>

    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text" select="'static const '"/>
    </xsl:call-template>

    <xsl:apply-templates select="type" mode="highlight"/>

    <xsl:if test="not(@name = '')">
      <xsl:text> </xsl:text>
      <xsl:call-template name="source-highlight">
        <xsl:with-param name="text" select="@name"/>
      </xsl:call-template>
    </xsl:if>

    <xsl:text> = </xsl:text>

    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text">
        <xsl:apply-templates select="default/*|default/text()"/>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:call-template name="highlight-special">
      <xsl:with-param name="text" select="';'"/>
    </xsl:call-template>

    <xsl:if test="purpose">
      <xsl:text>  </xsl:text>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text">
          <xsl:text>// </xsl:text>
          <xsl:apply-templates select="purpose" mode="comment"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Format base classes on a single line -->
  <xsl:template name="print.base.classes.single">
    <xsl:apply-templates select="inherit"/>
  </xsl:template>

  <xsl:template name="print.base.classes.multi">
    <xsl:param name="indentation"/>

    <xsl:variable name="n" select="count(inherit)"/>
    <xsl:for-each select="inherit">
      <!-- Indentation -->
      <xsl:if test="position() &gt; 1">
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:if>

      <!-- Output the access specifier -->
      <xsl:variable name="access">
        <xsl:choose>
          <xsl:when test="@access">
            <xsl:value-of select="@access"/>
          </xsl:when>
          <xsl:when test="parent::class|parent::class-specialization">
            <xsl:text>private</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>public</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:call-template name="highlight-keyword">
        <xsl:with-param name="keyword" select="@access"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>

      <!-- Output the type -->
      <xsl:choose>
        <xsl:when test="type">
          <xsl:apply-templates select="type/*|type/text()" mode="annotation">
            <xsl:with-param name="highlight" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message>
            <xsl:text>Warning: missing 'type' element inside 'inherit'</xsl:text>
          </xsl:message>
          <xsl:call-template name="print.warning.context"/>
          <xsl:apply-templates mode="annotation"/>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:if test="@pack=1"><xsl:text>...</xsl:text></xsl:if>

      <!-- Output a comma if not at the end -->
      <xsl:if test="position() &lt; $n">
        <xsl:text>,</xsl:text>
      </xsl:if>

      <!-- Output a comment if we have one -->
      <xsl:if test="purpose">
        <xsl:choose>
          <xsl:when test="position() &lt; $n">
            <xsl:text>  </xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>   </xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="highlight-comment">
          <xsl:with-param name="text">
            <xsl:text>// </xsl:text>
            <xsl:apply-templates select="purpose"
              mode="comment"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:if>

      <xsl:if test="position() &lt; $n">
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="print.base.classes">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="base-indentation" select="0"/>

    <xsl:variable name="single-line-candidate" select="not(inherit/purpose)"/>
    <xsl:variable name="single-line">
      <xsl:if test="$single-line-candidate">
        <xsl:call-template name="print.base.classes.single"/>
      </xsl:if>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$single-line-candidate and
                      (string-length($single-line) + $indentation + 3
                        &lt; $max-columns)">
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="' : '"/>
        </xsl:call-template>
        <xsl:call-template name="print.base.classes.single"/>
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="' {'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$single-line-candidate and
                      (string-length($single-line) + $base-indentation + 2
                        &lt; $max-columns)">
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="' :&#10;'"/>
        </xsl:call-template>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$base-indentation + 2"/>
        </xsl:call-template>
        <xsl:call-template name="print.base.classes.single"/>
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$base-indentation"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-special">
          <xsl:with-param name="text" select="'{'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="' : '"/>
        </xsl:call-template>
        <xsl:call-template name="print.base.classes.multi">
          <xsl:with-param name="indentation" select="$indentation + 3"/>
        </xsl:call-template>
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$base-indentation"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="'{'"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Emit a list of base classes without comments and on a single line -->
  <xsl:template match="inherit">
    <xsl:choose>
      <xsl:when test="position()=1">
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>, </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="highlight-keyword">
      <xsl:with-param name="keyword" select="@access"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <xsl:apply-templates mode="annotation">
      <xsl:with-param name="highlight" select="true()"/>
    </xsl:apply-templates>
    <xsl:if test="@pack=1"><xsl:text>...</xsl:text></xsl:if>
  </xsl:template>

  <!-- Find the maximum length of the types in typedefs -->
  <xsl:template name="find-max-type-length">
    <xsl:param name="typedefs" select="typedef"/>
    <xsl:param name="max-length" select="0"/>
    <xsl:param name="want-name" select="false()"/>

    <xsl:choose>
      <xsl:when test="$typedefs">
        <xsl:variable name="typedef" select="$typedefs[position()=1]"/>
        <xsl:variable name="rest" select="$typedefs[position()!=1]"/>

        <!-- Find the length of the type -->
        <xsl:variable name="length">
          <xsl:choose>
            <xsl:when test="$typedef/@name != '...'">
              <xsl:choose>
                <xsl:when test="$want-name">
                  <xsl:value-of select="string-length($typedef/@name) + 1"/>
                </xsl:when>
                <xsl:when test="$typedef/@type">
                  <xsl:value-of select="string-length($typedef/@type)"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="string-length($typedef/type)"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:when>
            <xsl:otherwise>
              0
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <!-- Pass on the larger length -->
        <xsl:choose>
          <xsl:when test="$length &gt; $max-length">
            <xsl:call-template name="find-max-type-length">
              <xsl:with-param name="typedefs" select="$rest"/>
              <xsl:with-param name="max-length" select="$length"/>
              <xsl:with-param name="want-name" select="$want-name"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="find-max-type-length">
              <xsl:with-param name="typedefs" select="$rest"/>
              <xsl:with-param name="max-length" select="$max-length"/>
              <xsl:with-param name="want-name" select="$want-name"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$max-length"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="constructor" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:call-template name="function">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="context" select="../@name"/>
      <xsl:with-param name="is-reference" select="false()"/>
      <xsl:with-param name="constructor-for">
        <xsl:call-template name="object-name"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="copy-assignment" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:call-template name="function">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="context" select="../@name"/>
      <xsl:with-param name="is-reference" select="false()"/>
      <xsl:with-param name="copy-assign-for">
        <xsl:call-template name="object-name"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="destructor" mode="synopsis">
    <xsl:param name="indentation"/>
    <xsl:call-template name="function">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="context" select="../@name"/>
      <xsl:with-param name="is-reference" select="false()"/>
      <xsl:with-param name="destructor-for">
        <xsl:call-template name="object-name"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="class-members-synopsis">
    <xsl:param name="indentation" select="0"/>
    <!-- Used to suppress anchors in nested synopsis, so we don't get multiple anchors -->
    <xsl:param name="allow-synopsis-anchors" select="false()"/>

    <!-- Typedefs -->
    <xsl:if test="typedef">
      <xsl:text>&#10;</xsl:text>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text">
          <xsl:text>// </xsl:text>
          <!-- True if there are any non-compacted typedefs -->
          <xsl:variable name="have-typedef-references"
            select="typedef and ((typedef/para|typedef/description) or ($boost.compact.typedef='0'))"/>
          <xsl:choose>
            <xsl:when test="$have-typedef-references">
              <xsl:call-template name="internal-link">
                <xsl:with-param name="to">
                  <xsl:call-template name="generate.id"/>
                  <xsl:text>types</xsl:text>
                </xsl:with-param>
                <xsl:with-param name="text" select="'types'"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text>types</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>

      <xsl:variable name="max-type-length">
        <xsl:call-template name="find-max-type-length"/>
      </xsl:variable>
      <xsl:variable name="max-name-length">
        <xsl:call-template name="find-max-type-length">
          <xsl:with-param name="want-name" select="true()"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:apply-templates select="typedef" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
        <xsl:with-param name="max-type-length"
          select="$max-type-length"/>
        <xsl:with-param name="max-name-length"
          select="$max-name-length"/>
        <xsl:with-param name="allow-anchor" select="$allow-synopsis-anchors"/>
      </xsl:apply-templates>
    </xsl:if>

    <!-- Static constants -->
    <xsl:if test="static-constant">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="typedef">
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text" select="'// static constants'"/>
      </xsl:call-template>
      <xsl:apply-templates select="static-constant" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:apply-templates>
    </xsl:if>

    <!-- Nested classes/structs/unions -->
    <xsl:if test="class|class-specialization|struct|struct-specialization|union|union-specialization">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="typedef|static-constant">
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text" select="'// member classes/structs/unions'"/>
      </xsl:call-template>
      <xsl:apply-templates select="class|class-specialization|
                                 struct|struct-specialization|
                                 union|union-specialization"
        mode="reference">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:apply-templates>
    </xsl:if>

    <!-- Enums -->
    <xsl:apply-templates select="enum" mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>

    <!-- Construct/Copy/Destruct -->
    <xsl:call-template name="construct-copy-destruct-synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:call-template>

    <!-- Member functions -->
    <xsl:apply-templates
      select="method-group|method|overloaded-method"
      mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation + 2"/>
    </xsl:apply-templates>

    <!-- Data members -->
    <xsl:if test="data-member">
      <xsl:text>&#10;&#10;</xsl:text>
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text">
          <xsl:text>// </xsl:text>
          <xsl:choose>
            <xsl:when test="data-member/description">
              <xsl:call-template name="internal-link">
                <xsl:with-param name="to">
                  <xsl:call-template name="generate.id"/>
                  <xsl:text>public-data-members</xsl:text>
                </xsl:with-param>
                <xsl:with-param name="text" select="'public data members'"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text>public data members</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:apply-templates select="data-member" mode="synopsis">
        <xsl:with-param name="indentation" select="$indentation + 2"/>
      </xsl:apply-templates>
    </xsl:if>
  </xsl:template>

  <xsl:template name="print-access-specification">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="specification" select="'public'"/>

    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="highlight-keyword">
      <xsl:with-param name="keyword" select="$specification"/>
    </xsl:call-template>
    <xsl:call-template name="highlight-special">
      <xsl:with-param name="text" select="':'"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="access" mode="synopsis">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="allow-synopsis-anchors" select="false()"/>

    <xsl:call-template name="print-access-specification">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="specification" select="@name"/>
    </xsl:call-template>
    <xsl:call-template name="class-members-synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="allow-synopsis-anchors" select="$allow-synopsis-anchors"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="class-type-synopsis">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="allow-synopsis-anchors" select="false()"/>

    <!-- The keyword used to declare this class type, e.g., class,
         struct, or union. -->
    <xsl:variable name="class-key">
      <xsl:call-template name="type.class.key"/>
    </xsl:variable>

    <xsl:if test="ancestor::class|ancestor::class-specialization|
                  ancestor::struct|ancestor::struct-specialization|
                  ancestor::union|ancestor::union-specialization">
      <xsl:text>&#10;</xsl:text>

      <!-- If this nested class has a "purpose" element, use it as a
           comment. -->
      <xsl:if test="purpose">
        <xsl:text>&#10;</xsl:text>
        <xsl:variable name="spaces">
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:copy-of select="$spaces"/>
        <xsl:call-template name="highlight-comment">
          <xsl:with-param name="text">
            <xsl:text>// </xsl:text>
            <xsl:apply-templates select="purpose" mode="comment">
              <xsl:with-param name="wrap" select="true()"/>
              <xsl:with-param name="prefix" select="concat($spaces, '// ')"/>
            </xsl:apply-templates>
          </xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
    </xsl:if>

    <!-- Template header -->
    <xsl:if test="template">
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:call-template>
      <xsl:apply-templates select="template" mode="reference">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:apply-templates>
    </xsl:if>
    <xsl:text>&#10;</xsl:text>

    <!-- Class name -->
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="highlight-keyword">
      <xsl:with-param name="keyword" select="$class-key"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>

    <!--  Make the class name a link to the class reference page (useful for nested classes) -->
    <xsl:call-template name="internal-link">
      <xsl:with-param name="to">
        <xsl:call-template name="generate.id">
          <xsl:with-param name="node" select="."/>
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="text">
        <xsl:value-of select="@name"/>
      </xsl:with-param>
    </xsl:call-template>

    <xsl:apply-templates select="specialization"/>

    <xsl:choose>
      <xsl:when test="inherit">
        <!-- Base class list (with opening brace) -->
        <xsl:call-template name="print.base.classes">
          <xsl:with-param name="indentation"
            select="string-length($class-key) + string-length(@name)
                    + $indentation + 1"/>
          <xsl:with-param name="base-indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <!-- Opening brace -->
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="' {'"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>

    <!-- Default public designator for members not inside explicit <access> elements -->
    <xsl:if test="contains(local-name(.), 'class')">
      <xsl:if test="count(static-constant|typedef|enum|
        copy-assignment|constructor|destructor|method-group|
        method|overloaded-method|data-member|
        class|class-specialization|
        struct|struct-specialization|
        union|union-specialization) &gt; 0">
        <xsl:call-template name="print-access-specification">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:if>

    <xsl:call-template name="class-members-synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="allow-synopsis-anchors" select="$allow-synopsis-anchors"/>
    </xsl:call-template>

    <xsl:apply-templates select="access" mode="synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="allow-synopsis-anchors" select="$allow-synopsis-anchors"/>
    </xsl:apply-templates>

    <!-- Closing brace -->
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="highlight-text">
      <xsl:with-param name="text" select="'};'"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Emit nested class reference documentation -->
  <xsl:template match="class|class-specialization|
                       struct|struct-specialization|
                       union|union-specialization" mode="reference">
    <xsl:param name="indentation"/>

    <xsl:call-template name="class-type-synopsis">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Document template parameters -->
  <xsl:template name="class-templates-reference">
    <xsl:if test="(template/template-type-parameter/purpose|
                  template/template-nontype-parameter/purpose)
                  and not($template.param.brief)">
      <refsect2>
        <title>Template Parameters</title>
        <orderedlist>
          <xsl:for-each select="template/template-type-parameter|
                template/template-nontype-parameter">
            <listitem>
              <para>
                <xsl:variable name="link-to">
                  <xsl:call-template name="generate.id"/>
                </xsl:variable>

                <xsl:call-template name="preformatted">
                  <xsl:with-param name="text">
                    <xsl:call-template name="template.parameter">
                      <xsl:with-param name="parameter" select="."/>
                      <xsl:with-param name="highlight" select="true()"/>
                    </xsl:call-template>
                  </xsl:with-param>
                </xsl:call-template>
              </para>
              <xsl:if test="purpose">
                <para>
                  <xsl:apply-templates select="purpose/*"/>
                </para>
              </xsl:if>
            </listitem>
          </xsl:for-each>
        </orderedlist>
      </refsect2>
    </xsl:if>
  </xsl:template>

  <xsl:template name="member-typedefs-reference">
    <!-- True if there are any non-compacted typedefs -->
    <xsl:variable name="have-typedef-references"
      select="typedef and ((typedef/para|typedef/description) or ($boost.compact.typedef='0'))"/>
    <xsl:if test="$have-typedef-references">
      <xsl:call-template name="member-documentation">
        <xsl:with-param name="name">
          <xsl:call-template name="anchor">
            <xsl:with-param name="to">
              <xsl:call-template name="generate.id"/>
              <xsl:text>types</xsl:text>
            </xsl:with-param>
            <xsl:with-param name="text" select="''"/>
          </xsl:call-template>
          <xsl:call-template name="monospaced">
            <xsl:with-param name="text">
              <xsl:call-template name="object-name"/>
            </xsl:with-param>
          </xsl:call-template>
          <xsl:text> </xsl:text>
          <xsl:call-template name="access-name"/>
          <xsl:text> types</xsl:text>
        </xsl:with-param>
        <xsl:with-param name="text">
          <orderedlist>
            <xsl:apply-templates select="typedef" mode="reference"/>
          </orderedlist>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
 </xsl:template>

  <xsl:template name="data-member-reference">
    <xsl:if test="data-member/description">
      <xsl:call-template name="member-documentation">
        <xsl:with-param name="name">
          <xsl:call-template name="anchor">
            <xsl:with-param name="to">
              <xsl:call-template name="generate.id"/>
              <xsl:text>public-data-members</xsl:text>
            </xsl:with-param>
            <xsl:with-param name="text" select="''"/>
          </xsl:call-template>
          <xsl:call-template name="monospaced">
            <xsl:with-param name="text">
              <xsl:call-template name="object-name"/>
            </xsl:with-param>
          </xsl:call-template>
          <xsl:text> </xsl:text>
          <xsl:call-template name="access-name"/>
          <xsl:text> public data members</xsl:text>
        </xsl:with-param>
        <xsl:with-param name="text">
          <orderedlist>
            <xsl:apply-templates select="data-member" mode="reference"/>
          </orderedlist>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="class-members-reference">

    <xsl:call-template name="member-typedefs-reference"/>

    <xsl:call-template name="construct-copy-destruct-reference"/>

    <xsl:apply-templates
      select="method-group|method|overloaded-method"
      mode="reference"/>
    
    <xsl:call-template name="data-member-reference"/>

    <!-- Emit reference docs for nested classes -->
    <xsl:apply-templates
      select="class|class-specialization|
              struct|struct-specialization|
              union|union-specialization"
      mode="namespace-reference"/>

    <!-- Emit reference docs for nested enums -->
    <xsl:apply-templates
      select="enum"
      mode="namespace-reference"/>
  </xsl:template>

  <xsl:template match="access" mode="namespace-reference">
    <xsl:call-template name="class-members-reference"/>
  </xsl:template>

  <!-- Emit namespace-level class reference documentation -->
  <xsl:template match="class|class-specialization|
                       struct|struct-specialization|
                       union|union-specialization" mode="namespace-reference">
    <xsl:param name="indentation" select="0"/>

    <xsl:call-template name="separator"/>
    <xsl:call-template name="reference-documentation">
      <xsl:with-param name="refname">
        <xsl:call-template name="fully-qualified-name">
          <xsl:with-param name="node" select="."/>
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
      <xsl:with-param name="anchor">
        <xsl:call-template name="generate.id">
          <xsl:with-param name="node" select="."/>
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="name">
        <xsl:call-template name="type.display.name"/>
      </xsl:with-param>
      <xsl:with-param name="synopsis">
        <xsl:call-template name="header-link"/>
        <xsl:call-template name="class-type-synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="allow-synopsis-anchors" select="true()"/>
        </xsl:call-template>
        <!-- Associated free functions -->
        <xsl:apply-templates select="free-function-group"
          mode="synopsis">
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="class" select="@name"/>
        </xsl:apply-templates>
      </xsl:with-param>
      <xsl:with-param name="text">
        <!-- Paragraphs go into the top of the "Description" section. -->
        <xsl:if test="para">
          <xsl:message>
            <xsl:text>Warning: Use of 'para' elements in 'class' element is deprecated.&#10;Wrap them in a 'description' element.</xsl:text>
          </xsl:message>
          <xsl:call-template name="print.warning.context"/>
          <xsl:apply-templates select="para" mode="annotation"/>
        </xsl:if>
        <xsl:apply-templates select="description"/>
        <xsl:call-template name="class-templates-reference"/>
        <xsl:call-template name="class-members-reference"/>
        <xsl:apply-templates select="access" mode="namespace-reference"/>

        <xsl:apply-templates select="free-function-group" mode="reference">
          <xsl:with-param name="class" select="@name"/>
        </xsl:apply-templates>

        <!-- Specializations of this class -->
        <!-- TBD: fix this. We should key off the class name and match
             fully-qualified names -->
        <xsl:variable name="name" select="@name"/>
        <xsl:if test="local-name(.)='class' and
                      ../class-specialization[@name=$name]">
          <refsect2>
            <title>Specializations</title>
            <itemizedlist>
              <xsl:apply-templates
                select="../class-specialization[@name=$name]"
                mode="specialization-list"/>
            </itemizedlist>
          </refsect2>
        </xsl:if>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- Output a link to a specialization -->
  <xsl:template match="class-specialization|
                       struct-specialization|
                       union-specialization" mode="specialization-list">
    <listitem>
      <para>
        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text">
            <xsl:call-template name="type.display.name"/>
          </xsl:with-param>
        </xsl:call-template>
      </para>
    </listitem>
  </xsl:template>

  <!-- Data member synopsis -->
  <xsl:template match="data-member" mode="synopsis">
    <xsl:param name="indentation"/>

    <xsl:choose>
      <xsl:when test="ancestor::class|ancestor::class-specialization|
                      ancestor::struct|ancestor::struct-specialization|
                      ancestor::union|ancestor::union-specialization">

        <!-- Spacing -->
        <xsl:if
          test="not(local-name(preceding-sibling::*[position()=1])=local-name(.)) and (position() &gt; 1)">
          <xsl:text>&#10;</xsl:text>
        </xsl:if>

        <!-- Indent -->
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>

        <xsl:if test="@specifiers">
          <xsl:call-template name="highlight-keyword">
            <xsl:with-param name="keyword" select="@specifiers"/>
          </xsl:call-template>
          <xsl:text> </xsl:text>
        </xsl:if>

        <xsl:apply-templates select="type" mode="highlight"/>
        <xsl:text> </xsl:text>
        <xsl:choose>
          <xsl:when test="description">
            <xsl:variable name="link-to">
              <xsl:call-template name="generate.id"/>
            </xsl:variable>
            <xsl:call-template name="link-or-anchor">
              <xsl:with-param name="to" select="$link-to"/>
              <xsl:with-param name="text" select="@name"/>
              <xsl:with-param name="link-type" select="'link'"/>
              <xsl:with-param name="highlight" select="true()"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="highlight-identifier">
              <xsl:with-param name="identifier" select="@name"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="';'"/>
        </xsl:call-template>

      </xsl:when>
      <xsl:otherwise>
       <xsl:call-template name="global-synopsis">
         <xsl:with-param name="indentation" select="$indentation"/>
       </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>

    <!-- If there is a <purpose>, then add it as an
         inline comment immediately following the data
         member definition in the synopsis -->
    <xsl:if test="purpose">
      <xsl:call-template name="indent">
        <xsl:with-param name="indentation" select="$indentation"/>
      </xsl:call-template>
      <xsl:call-template name="highlight-comment">
        <xsl:with-param name="text">
          <xsl:text>// </xsl:text>
          <xsl:apply-templates select="purpose/*|purpose/text()"
            mode="purpose"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Data member reference -->
  <xsl:template match="data-member" mode="reference">
    <xsl:choose>
      <xsl:when test="ancestor::class|ancestor::class-specialization|
                      ancestor::struct|ancestor::struct-specialization|
                      ancestor::union|ancestor::union-specialization">
        <xsl:if test="description">
          <listitem>
            <para>
              <xsl:variable name="link-to">
                <xsl:call-template name="generate.id"/>
              </xsl:variable>
            
              <xsl:call-template name="preformatted">
                <xsl:with-param name="text">
                  <xsl:if test="@specifiers">
                    <xsl:call-template name="highlight-keyword">
                      <xsl:with-param name="keyword" select="@specifiers"/>
                    </xsl:call-template>
                    <xsl:text> </xsl:text>
                  </xsl:if>

                  <xsl:apply-templates select="type" mode="highlight"/>
                  <xsl:text> </xsl:text>
                  <xsl:call-template name="link-or-anchor">
                    <xsl:with-param name="to" select="$link-to"/>
                    <xsl:with-param name="text" select="@name"/>
                    <xsl:with-param name="link-type" select="'anchor'"/>
                    <xsl:with-param name="highlight" select="true()"/>
                  </xsl:call-template>
                  <xsl:call-template name="highlight-special">
                    <xsl:with-param name="text" select="';'"/>
                  </xsl:call-template>
                </xsl:with-param>
              </xsl:call-template>
            </para>
            <xsl:apply-templates select="description"/>
          </listitem>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="global-reference"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Enumeration synopsis -->
  <xsl:template match="enum" mode="synopsis">
    <xsl:param name="indentation"/>

    <!-- Spacing -->
    <xsl:if
      test="(not (local-name(preceding-sibling::*[position()=1])=local-name(.))
             and (position() &gt; 1)) or
            not (para or description or not ($boost.compact.enum=1))">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <!-- Indent -->
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>

    <xsl:choose>
      <!-- When there is a detailed description, we only put the
           declaration in the synopsis and will put detailed documentation
           in either a <refentry/> or in class documentation. -->
      <xsl:when test="para or description or not ($boost.compact.enum=1)">
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'enum'"/>
        </xsl:call-template>

        <xsl:text> </xsl:text>

        <xsl:call-template name="internal-link">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="string(@name)"/>
          <xsl:with-param name="higlhight" select="false()"/>
        </xsl:call-template>
      </xsl:when>
      <!-- When there is no detailed description, we put the enum
           definition within the synopsis. The purpose of the enum (if
           available) is formatted as a comment prior to the
           definition. This way, we do not create a separate block of text
           for what is generally a very small amount of information. -->
      <xsl:otherwise>
        <xsl:if test="purpose">
          <xsl:call-template name="highlight-comment">
            <xsl:with-param name="text">
              <xsl:text>// </xsl:text>
              <xsl:apply-templates select="purpose" mode="comment"/>
            </xsl:with-param>
          </xsl:call-template>

          <xsl:text>&#10;</xsl:text>
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
        </xsl:if>

        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'enum'"/>
        </xsl:call-template>

        <xsl:text> </xsl:text>

        <xsl:call-template name="anchor">
          <xsl:with-param name="to">
            <xsl:call-template name="generate.id">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="text" select="@name"/>
          <xsl:with-param name="higlhight" select="false()"/>
        </xsl:call-template>

        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="' { '"/>
        </xsl:call-template>
        <xsl:call-template name="type.enum.list.compact">
          <xsl:with-param name="indentation"
            select="$indentation + string-length(@name) + 8"/>
          <xsl:with-param name="compact" select="true()"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="' }'"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="highlight-text">
      <xsl:with-param name="text" select="';'"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Enumeration reference at namespace level -->
  <xsl:template match="enum" mode="namespace-reference">
    <xsl:if test="para or description or not ($boost.compact.enum=1)">
      <xsl:call-template name="reference-documentation">
        <xsl:with-param name="name">
          <xsl:call-template name="type.display.name"/>
        </xsl:with-param>
        <xsl:with-param name="refname">
          <xsl:call-template name="fully-qualified-name">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
        <xsl:with-param name="anchor">
          <xsl:call-template name="generate.id">
            <xsl:with-param name="node" select="."/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="synopsis">
          <xsl:call-template name="header-link"/>
          <xsl:call-template name="type.enum.display"/>
        </xsl:with-param>

        <xsl:with-param name="text">
          <!-- Paragraphs go into the top of the "Description" section. -->
          <xsl:if test="para">
            <xsl:message>
              <xsl:text>Warning: Use of 'para' elements in 'enum' element is deprecated.&#10;Wrap them in a 'description' element.</xsl:text>
            </xsl:message>
            <xsl:call-template name="print.warning.context"/>
            <xsl:apply-templates select="para" mode="annotation"/>
          </xsl:if>
          <xsl:apply-templates select="description"/>
          <xsl:if test="enumvalue/purpose | enumvalue/description">
            <variablelist spacing="compact">
              <xsl:apply-templates select="enumvalue" mode="reference"/>
            </variablelist>
          </xsl:if>
        </xsl:with-param>

      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Output an enumeration along with its values -->
  <xsl:template name="type.enum.display">
    <!-- Spacing -->
    <xsl:if test="position() &gt; 1">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

    <xsl:text>&#10;</xsl:text>

    <xsl:call-template name="highlight-keyword">
      <xsl:with-param name="keyword" select="'enum'"/>
    </xsl:call-template>

    <!-- Header -->
    <xsl:variable name="header" select="concat(' ', @name, ' { ')"/>
    <xsl:call-template name="highlight-text">
      <xsl:with-param name="text" select="$header"/>
    </xsl:call-template>

    <!-- Print the enumeration values -->
    <xsl:call-template name="type.enum.list.compact">
      <xsl:with-param name="indentation" select="4 + string-length($header)"/>
    </xsl:call-template>

    <xsl:call-template name="highlight-text">
      <xsl:with-param name="text" select="' };'"/>
    </xsl:call-template>
  </xsl:template>

  <!-- List enumeration values in a compact form e.g.,
       enum Name { value1 = foo, value2 = bar, ... };
       This routine prints only the enumeration values; the caller is
       responsible for printing everything outside the braces
       (inclusive). -->
  <xsl:template name="type.enum.list.compact">
    <xsl:param name="indentation"/>
    <xsl:param name="compact" select="false()"/>

    <!-- Internal: The column we are on -->
    <xsl:param name="column" select="$indentation"/>

    <!-- Internal: The index of the current enumvalue
         we're processing -->
    <xsl:param name="pos" select="1"/>

    <!-- Internal: a prefix that we need to print prior to printing
         this value. -->
    <xsl:param name="prefix" select="''"/>

    <xsl:if test="not($pos &gt; count(enumvalue))">
      <xsl:variable name="value" select="enumvalue[position()=$pos]"/>

      <!-- Compute the string to be printed for this value -->
      <xsl:variable name="result">
        <xsl:value-of select="$prefix"/>
        <xsl:value-of select="$value/attribute::name"/>

        <xsl:if test="$value/default">
          <xsl:text> = </xsl:text>
          <xsl:value-of select="$value/default/*|$value/default/text()"/>
        </xsl:if>
      </xsl:variable>

      <!-- The column we will end on, assuming that this value fits on
           this line -->
      <xsl:variable name="end" select="$column + string-length($result)"/>

      <!-- The column we will actually end on -->
      <xsl:variable name="end2">
        <xsl:choose>
          <!-- If the enumeration value fits on this line, put it there -->
          <xsl:when test="$end &lt; $max-columns">
            <xsl:value-of select="$end"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$indentation
                                + string-length($result)
                                - string-length($prefix)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:value-of select="$prefix"/>

      <!-- If the enumeration value doesn't fit on this line,
           put it on a new line -->
      <xsl:if test="not($end &lt; $max-columns)">
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:if>

      <!-- If the enumeration value has a description, link it
           to its description. -->
      <xsl:choose>
        <xsl:when test="($value/purpose or $value/description) and not($compact)">
          <xsl:call-template name="internal-link">
            <xsl:with-param name="to">
              <xsl:call-template name="generate.id">
                <xsl:with-param name="node" select="$value"/>
              </xsl:call-template>
            </xsl:with-param>
            <xsl:with-param name="text" select="$value/attribute::name"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$value/attribute::name"/>
        </xsl:otherwise>
      </xsl:choose>

      <!-- If the enumeration value has a default,
           print it. -->
      <xsl:if test="$value/default">
        <xsl:text> = </xsl:text>
        <xsl:apply-templates
          select="$value/default/*|$value/default/text()"/>
      </xsl:if>

      <!-- Recursively generate the rest of the enumeration list -->
      <xsl:call-template name="type.enum.list.compact">
        <xsl:with-param name="indentation" select="$indentation"/>
        <xsl:with-param name="compact" select="$compact"/>
        <xsl:with-param name="column" select="$end2"/>
        <xsl:with-param name="pos" select="$pos + 1"/>
        <xsl:with-param name="prefix" select="', '"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Enumeration reference at namespace level -->
  <xsl:template match="enumvalue" mode="reference">
    <xsl:if test="purpose or description">
      <varlistentry>
        <term>
          <xsl:call-template name="monospaced">
            <xsl:with-param name="text" select="@name"/>
          </xsl:call-template>
          <!-- Note: the anchor must come after the text here, and not
               before it; otherwise, FOP goes into an infinite loop. -->
          <xsl:call-template name="anchor">
            <xsl:with-param name="to">
              <xsl:call-template name="generate.id"/>
            </xsl:with-param>
            <xsl:with-param name="text" select="''"/>
          </xsl:call-template>
        </term>
        <listitem>
          <xsl:apply-templates select="purpose|description" mode="comment"/>
        </listitem>
      </varlistentry>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
