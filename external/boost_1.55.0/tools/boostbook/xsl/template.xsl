<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <xsl:param name="template.param.brief" select="false()"/>
  
  <!-- Determine the length of a template header synopsis -->
  <xsl:template name="template.synopsis.length">
    <xsl:variable name="text">
      <xsl:apply-templates select="template" mode="synopsis">
        <xsl:with-param name="indentation" select="0"/>
        <xsl:with-param name="wrap" select="false()"/>
        <xsl:with-param name="highlight" select="false()"/>
      </xsl:apply-templates>
    </xsl:variable>
    <xsl:value-of select="string-length($text)"/>
  </xsl:template>

  <!-- Determine the length of a template header reference -->
  <xsl:template name="template.reference.length">
    <xsl:choose>
      <xsl:when test="not(template)">
        0
      </xsl:when>
      <xsl:when test="template/*/purpose">
        <!-- TBD: The resulting value need only be greater than the number of
             columns. We chose to add 17 because it's funny for C++
             programmers. :) -->
        <xsl:value-of select="$max-columns + 17"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="template.synopsis.length"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Output a template header in synopsis mode -->
  <xsl:template match="template" mode="synopsis">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="wrap" select="true()"/>
    <xsl:param name="highlight" select="true()"/>

    <xsl:call-template name="template.synopsis"> 
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="wrap" select="$wrap"/>
      <xsl:with-param name="highlight" select="$highlight"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Output a template header in reference mode -->
  <xsl:template match="template" mode="reference">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="highlight" select="true()"/>
    <xsl:call-template name="template.reference">
      <xsl:with-param name="indentation" select="$indentation"/>
      <xsl:with-param name="highlight" select="$highlight"/>
    </xsl:call-template>
  </xsl:template>

  <!-- Emit a template header synopsis -->
  <xsl:template name="template.synopsis">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="wrap" select="true()"/>
    <xsl:param name="highlight" select="true()"/>

    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'template'"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-special">
          <xsl:with-param name="text" select="'&lt;'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>template</xsl:text>
        <xsl:text>&lt;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="template.synopsis.parameters">
      <xsl:with-param name="indentation" select="$indentation + 9"/>
      <xsl:with-param name="wrap" select="$wrap"/>
      <xsl:with-param name="highlight" select="$highlight"/>
    </xsl:call-template>
    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="highlight-special">
          <xsl:with-param name="text" select="'&gt;'"/>
        </xsl:call-template>
        <xsl:text> </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&gt; </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Display a list of template parameters for a synopsis (no comments) -->
  <xsl:template name="template.synopsis.parameters">
    <xsl:param name="indentation"/>
    <xsl:param name="wrap" select="true()"/>
    <xsl:param name="highlight" select="true()"/>

    <xsl:param name="column" select="$indentation"/>
    <xsl:param name="prefix" select="''"/>
    <xsl:param name="parameters" select="template-type-parameter|template-varargs|template-nontype-parameter"/>
    <xsl:param name="first-on-line" select="true()"/>

    <xsl:if test="$parameters">
      <!-- Emit the prefix (either a comma-space, or empty if this is
           the first parameter) -->
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-text">
            <xsl:with-param name="text" select="$prefix"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$prefix"/>
        </xsl:otherwise>
      </xsl:choose>

      <!-- Get the actual parameter and its attributes -->
      <xsl:variable name="parameter" select="$parameters[position()=1]"/>
      <xsl:variable name="rest" select="$parameters[position()!=1]"/>

      <!-- Compute the actual text of this parameter -->
      <xsl:variable name="text">
        <xsl:call-template name="template.parameter">
          <xsl:with-param name="parameter" select="$parameter"/>
          <xsl:with-param name="is-last" select="not(rest)"/>
          <xsl:with-param name="highlight" select="false()"/>
        </xsl:call-template>
      </xsl:variable>

      <!-- Where will this template parameter finish? -->
      <xsl:variable name="end-column" 
        select="$column + string-length($prefix) + string-length($text)"/>

      <!-- Should the text go on this line or on the next? -->
      <xsl:choose>
        <xsl:when test="$first-on-line or ($end-column &lt; $max-columns) or
                        not($wrap)">
          <!-- Print on this line -->
          <xsl:call-template name="template.parameter">
            <xsl:with-param name="parameter" select="$parameter"/>
            <xsl:with-param name="is-last" select="not($rest)"/>
            <xsl:with-param name="highlight" select="$highlight"/>
          </xsl:call-template>
          
          <!-- Recurse -->
          <xsl:call-template name="template.synopsis.parameters">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="wrap" select="$wrap"/>
            <xsl:with-param name="highlight" select="$highlight"/>
            <xsl:with-param name="column" select="$end-column"/>
            <xsl:with-param name="prefix" select="', '"/>
            <xsl:with-param name="parameters" select="$rest"/>
            <xsl:with-param name="first-on-line" select="false()"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <!-- Print on next line -->
          <xsl:text>&#10;</xsl:text>
          <xsl:call-template name="indent">
            <xsl:with-param name="indentation" select="$indentation"/>
          </xsl:call-template>
          <xsl:call-template name="template.parameter">
            <xsl:with-param name="parameter" select="$parameter"/>
            <xsl:with-param name="is-last" select="not($rest)"/>
            <xsl:with-param name="highlight" select="$highlight"/>
          </xsl:call-template>

          <xsl:call-template name="template.synopsis.parameters">
            <xsl:with-param name="indentation" select="$indentation"/>
            <xsl:with-param name="wrap" select="$wrap"/>
            <xsl:with-param name="highlight" select="$highlight"/>
            <xsl:with-param name="column" 
              select="$indentation + string-length($text)"/>
            <xsl:with-param name="prefix" select="', '"/>
            <xsl:with-param name="parameters" select="$rest"/>
            <xsl:with-param name="first-on-line" select="false()"/>           
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- Emit a template header reference -->
  <xsl:template name="template.reference">
    <xsl:param name="indentation" select="0"/>
    <xsl:param name="highlight" select="true()"/>

    <xsl:if test="template-type-parameter|template-varargs|template-nontype-parameter">
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-keyword">
            <xsl:with-param name="keyword" select="'template'"/>
          </xsl:call-template>
          <xsl:call-template name="highlight-special">
            <xsl:with-param name="text" select="'&lt;'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>template</xsl:text>
          <xsl:text>&lt;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="template.reference.parameters">
        <xsl:with-param name="indentation" select="$indentation + 9"/>
        <xsl:with-param name="highlight" select="$highlight"/>
      </xsl:call-template>
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-special">
            <xsl:with-param name="text" select="'&gt;'"/>
          </xsl:call-template>
          <xsl:text> </xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>&gt; </xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- Display a set of template parameters for a reference -->
  <xsl:template name="template.reference.parameters">
    <xsl:param name="indentation"/>
    <xsl:param name="highlight" select="true()"/>
    <xsl:param name="parameters" select="template-type-parameter|template-varargs|template-nontype-parameter"/>

    <xsl:choose>
      <xsl:when test="$parameters/purpose and $template.param.brief">
        <xsl:call-template name="template.reference.parameters.comments">
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="highlight" select="$highlight"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="template.synopsis.parameters">
          <xsl:with-param name="indentation" select="$indentation"/>
          <xsl:with-param name="wrap" select="true()"/>
          <xsl:with-param name="highlight" select="$highlight"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Output template parameters when there are comments with the parameters.
       For clarity, we output each template parameter on a separate line. -->
  <xsl:template name="template.reference.parameters.comments">
    <xsl:param name="indentation"/>
    <xsl:param name="highlight" select="true()"/>
    <xsl:param name="parameters" select="template-type-parameter|template-varargs|template-nontype-parameter"/>

    <xsl:if test="$parameters">
      <!-- Get the actual parameter and its attributes -->
      <xsl:variable name="parameter" select="$parameters[position()=1]"/>
      <xsl:variable name="rest" select="$parameters[position()!=1]"/>

      <!-- Display the parameter -->
      <xsl:call-template name="template.parameter">
        <xsl:with-param name="parameter" select="$parameter"/>
        <xsl:with-param name="is-last" select="not($rest)"/>
        <xsl:with-param name="highlight" select="$highlight"/>
      </xsl:call-template>

      <xsl:if test="$rest">
        <xsl:choose>
          <xsl:when test="$highlight">
            <xsl:call-template name="highlight-text">
              <xsl:with-param name="text" select="', '"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>, </xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <!-- Display the comment -->
      <xsl:if test="$parameter/purpose">
        <xsl:variable name="param-text">
          <!-- Display the parameter -->
          <xsl:call-template name="template.parameter">
            <xsl:with-param name="parameter" select="$parameter"/>
            <xsl:with-param name="is-last" select="not($rest)"/>
            <xsl:with-param name="highlight" select="false()"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="highlight-comment">
          <xsl:with-param name="text">
            <xsl:text>  // </xsl:text>
            <xsl:apply-templates
              select="$parameter/purpose/*|$parameter/purpose/text()" mode="comment">
              <xsl:with-param name="wrap" select="true()"/>
              <xsl:with-param name="prefix">
                <xsl:call-template name="indent">
                  <xsl:with-param name="indentation" select="$indentation + string-length($param-text)"/>
                </xsl:call-template>
                <xsl:if test="$rest">
                  <xsl:text>  </xsl:text>
                </xsl:if>
                <xsl:text>  // </xsl:text>
              </xsl:with-param>
            </xsl:apply-templates>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:if>

      <!-- Indent the next line -->
      <xsl:if test="$parameter/purpose or $rest">
        <xsl:text>&#10;</xsl:text>
        <xsl:call-template name="indent">
          <xsl:with-param name="indentation" select="$indentation"/>
        </xsl:call-template>
      </xsl:if>

      <!-- Recurse to print the rest of the parameters -->
      <xsl:call-template name="template.reference.parameters.comments">
        <xsl:with-param name="indentation" select="$indentation"/>
        <xsl:with-param name="highlight" select="$highlight"/>
        <xsl:with-param name="parameters" select="$rest"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Print a template parameter -->
  <xsl:template name="template.parameter">
    <xsl:param name="parameter"/>
    <xsl:param name="is-last"/>
    <xsl:param name="highlight" select="true()"/>
    <xsl:apply-templates select="$parameter"
      mode="print.parameter">
      <xsl:with-param name="parameter" select="$parameter"/>
      <xsl:with-param name="is-last" select="$is-last"/>
      <xsl:with-param name="highlight" select="$highlight"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template name="template.parameter.name">
    <xsl:param name="name" select="@name"/>
    <xsl:param name="highlight" select="true()"/>
    
    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="concept.link">
          <xsl:with-param name="name" 
            select="translate($name, '0123456789', '')"/>
          <xsl:with-param name="text" select="$name"/>
          <xsl:with-param name="warn" select="false"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="template-type-parameter" mode="print.parameter">
    <xsl:param name="parameter"/>
    <xsl:param name="is-last"/>
    <xsl:param name="highlight"/>

    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="'typename'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>typename</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$parameter/@pack=1">
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-text">
            <xsl:with-param name="text" select="'...'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>...</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:text> </xsl:text>

    <xsl:call-template name="template.parameter.name">
      <xsl:with-param name="name" select="$parameter/@name"/>
      <xsl:with-param name="highlight" select="$highlight"/>
    </xsl:call-template>

    <xsl:variable name="def">
      <xsl:choose>
        <xsl:when test="$parameter/@default">
          <xsl:message>
            <xsl:text>Warning: 'default' attribute of template parameter element is deprecated. Use 'default' element.</xsl:text>
            <xsl:call-template name="print.warning.context"/>
          </xsl:message>
          <xsl:choose>
            <xsl:when test="$highlight and false()">
              <xsl:call-template name="source-highlight">
                <xsl:with-param name="text">
                  <xsl:value-of select="$parameter/@default"/>
                </xsl:with-param>
              </xsl:call-template>       
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$parameter/@default"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$parameter/default">
          <xsl:choose>
            <xsl:when test="$highlight">
              <xsl:apply-templates 
                select="$parameter/default/*|$parameter/default/text()" 
                mode="highlight"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="string($parameter/default)"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:if test="not($def='')">
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-text">
            <xsl:with-param name="text" select="' = '"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text> = </xsl:text>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:copy-of select="$def"/>

      <!-- If this is the last parameter, add an extra space to
           avoid printing >> -->
      <xsl:if 
        test="$is-last and (substring($def, string-length($def))='&gt;')">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:if>    
  </xsl:template>

  <xsl:template match="template-nontype-parameter" mode="print.parameter">
    <xsl:param name="parameter"/>
    <xsl:param name="is-last"/>
    <xsl:param name="highlight"/>

    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="source-highlight">
          <xsl:with-param name="text">
            <xsl:apply-templates 
              select="$parameter/type/*|$parameter/type/text()"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$parameter/type/*|$parameter/type/text()"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$parameter/@pack=1">
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-text">
            <xsl:with-param name="text" select="'...'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>...</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:text> </xsl:text>

    <xsl:call-template name="template.parameter.name">
      <xsl:with-param name="name" select="$parameter/@name"/>
      <xsl:with-param name="highlight" select="$highlight"/>
    </xsl:call-template>

    <xsl:variable name="def">
      <xsl:value-of select="string($parameter/default)"/>
    </xsl:variable>

    <xsl:if test="not($def='')">
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-text">
            <xsl:with-param name="text" select="' = '"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text> = </xsl:text>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:apply-templates select="$parameter/default/*|$parameter/default/text()" mode="highlight"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$def"/>
        </xsl:otherwise>
      </xsl:choose>

      <!-- If this is the last parameter, add an extra space to
           avoid printing >> -->
      <xsl:if 
        test="$is-last and (substring($def, string-length($def))='&gt;')">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:if>    
  </xsl:template>

  <xsl:template match="template-varargs" mode="print.parameter">
    <xsl:param name="highlight" select="true()"/>
    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="'...'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>...</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="specialization">
    <xsl:param name="highlight" select="true()"/>
    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="'&lt;'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&lt;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="template-arg">
      <xsl:with-param name="highlight" select="$highlight"/>
    </xsl:apply-templates>
    <xsl:choose>
      <xsl:when test="$highlight">
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="'&gt;'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&gt;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="template-arg">
    <xsl:param name="highlight" select="true()"/>
    <xsl:if test="position() &gt; 1">
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-text">
            <xsl:with-param name="text" select="', '"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>, </xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:apply-templates mode="highlight"/>
    <xsl:if test="@pack=1">
      <xsl:choose>
        <xsl:when test="$highlight">
          <xsl:call-template name="highlight-text">
            <xsl:with-param name="text" select="'...'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>...</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>

