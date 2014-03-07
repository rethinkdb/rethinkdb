<?xml version="1.0" encoding="utf-8" ?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:template name="global-synopsis">
    <xsl:param name="indentation" select="0" />
    <xsl:if test="not(local-name(preceding-sibling::*[position()=1])=local-name(.)) and (position() &gt; 1)">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:text>&#10;</xsl:text>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation" />
    </xsl:call-template>
    <xsl:call-template name="global-synopsis-impl">
      <xsl:with-param name="link-type" select="'link'" />
    </xsl:call-template>
  </xsl:template>
  <xsl:template name="global-reference">
    <xsl:call-template name="reference-documentation">
      <xsl:with-param name="refname">
        <xsl:call-template name="fully-qualified-name">
          <xsl:with-param name="node" select="." />
        </xsl:call-template>
        <xsl:apply-templates select="specialization" />
      </xsl:with-param>
      <xsl:with-param name="purpose" select="purpose/*|purpose/text()" />
      <xsl:with-param name="anchor">
        <xsl:call-template name="generate.id" />
      </xsl:with-param>
      <xsl:with-param name="name">
        <xsl:text>Global </xsl:text>
        <xsl:call-template name="monospaced">
          <xsl:with-param name="text" select="@name" />
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="synopsis">
        <xsl:call-template name="header-link"/>
        <xsl:call-template name="global-synopsis-impl">
          <xsl:with-param name="link-type" select="'none'" />
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="text">
        <xsl:apply-templates select="description" />
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>
  <xsl:template name="global-synopsis-impl">
    <xsl:param name="link-type" />
    <xsl:if test="@specifiers">
      <xsl:call-template name="highlight-keyword">
        <xsl:with-param name="keyword" select="@specifiers" />
      </xsl:call-template>
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="type/*|type/text()" mode="annotation">
      <xsl:with-param name="highlight" select="true()"/>
    </xsl:apply-templates>
    <xsl:text> </xsl:text>
    <xsl:call-template name="link-or-anchor">
      <xsl:with-param name="to">
        <xsl:call-template name="generate.id" select="." />
      </xsl:with-param>
      <xsl:with-param name="text" select="@name" />
      <xsl:with-param name="link-type" select="$link-type" />
    </xsl:call-template>
    <xsl:call-template name="highlight-text">
      <xsl:with-param name="text" select="';'"/>
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="data-member" mode="generate.id">
    <xsl:call-template name="fully-qualified-id">
      <xsl:with-param name="node" select="."/>
    </xsl:call-template>
  </xsl:template>
</xsl:stylesheet>
