<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:template match="macro" mode="synopsis">
    <xsl:param name="indentation" select="0"/>

    <xsl:text>&#10;</xsl:text>
    <xsl:if 
      test="not(local-name(preceding-sibling::*[position()=1])=local-name(.))">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:call-template name="indent">
      <xsl:with-param name="indentation" select="$indentation"/>
    </xsl:call-template>
    <xsl:call-template name="macro-synopsis">
      <xsl:with-param name="link-type" select="'link'"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="macro" mode="reference">
    <xsl:call-template name="reference-documentation">
      <xsl:with-param name="refname" select="@name"/>
      <xsl:with-param name="purpose" select="purpose/*|purpose/text()"/>
      <xsl:with-param name="anchor">
        <xsl:call-template name="generate.id"/>
      </xsl:with-param>
      <xsl:with-param name="name">
        <xsl:text>Macro </xsl:text>
        <xsl:call-template name="monospaced">
          <xsl:with-param name="text" select="@name"/>
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="synopsis">
        <xsl:call-template name="header-link"/>
        <xsl:call-template name="macro-synopsis">
          <xsl:with-param name="link-type" select="'none'"/>
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="text">
        <xsl:apply-templates select="description"/>

        <xsl:if test="macro-parameter/description">
          <variablelist spacing="compact">
            <xsl:processing-instruction name="dbhtml">
              list-presentation="table"
            </xsl:processing-instruction>

            <!-- Document parameters -->
            <xsl:if test="macro-parameter/description">
              <varlistentry>
                <term>Parameters:</term>
                <listitem>
                  <variablelist spacing="compact">
                    <xsl:processing-instruction name="dbhtml">
                      list-presentation="table"
                    </xsl:processing-instruction>
                    <xsl:for-each select="macro-parameter">
                      <xsl:sort select="attribute::name"/>
                      <xsl:if test="description">
                        <varlistentry>
                          <term>
                            <xsl:call-template name="monospaced">
                              <xsl:with-param name="text" select="@name"/>
                            </xsl:call-template>
                          </term>
                          <listitem>
                            <xsl:apply-templates select="description/*"/>
                          </listitem>
                        </varlistentry>
                      </xsl:if>
                    </xsl:for-each>
                  </variablelist>
                </listitem>
              </varlistentry>
            </xsl:if>
          </variablelist>
        </xsl:if>
        
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="macro-synopsis">
    <xsl:param name="link-type"/>

    <xsl:call-template name="link-or-anchor">
      <xsl:with-param name="to">
        <xsl:call-template name="generate.id" select="."/>
      </xsl:with-param>
      <xsl:with-param name="text" select="@name"/>
      <xsl:with-param name="link-type" select="$link-type"/>
    </xsl:call-template>

    <xsl:if test="@kind='functionlike'">
      <xsl:text>(</xsl:text>
      <xsl:for-each select="macro-parameter">
        <xsl:if test="position() &gt; 1">
          <xsl:text>, </xsl:text>
        </xsl:if>
        <xsl:value-of select="@name"/>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template match="macro" mode="generate.id">
    <xsl:value-of select="@name"/>
    <xsl:if test="count(key('named-entities',
        translate(@name, $uppercase-letters, $lowercase-letters)))!=1">
      <xsl:text>_</xsl:text>
      <xsl:value-of select="generate-id(.)"/>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
