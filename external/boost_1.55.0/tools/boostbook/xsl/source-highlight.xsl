<?xml version="1.0" encoding="utf-8"?>
<!--
   Copyright (c) 2002 Douglas Gregor <doug.gregor -at- gmail.com>
  
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:param name="boost.syntax.highlight">1</xsl:param>

  <xsl:template name="source-highlight">
    <xsl:param name="text" select="."/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <xsl:call-template name="highlight-text">
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:variable name="id-start-chars" select="'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_'"/>
  <xsl:variable name="id-chars" select="'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_'"/>
  <xsl:variable name="digits" select="'1234567890'"/>
  <xsl:variable name="number-chars" select="'1234567890abcdefABCDEFxX.'"/>
  <xsl:variable name="keywords"
    select="' alignas ailgnof asm auto bool break case catch char char16_t char32_t class const const_cast constexpr continue decltype default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new noexcept nullptr operator private protected public register reinterpret_cast return short signed sizeof static static_cast struct switch template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while '"/>
  <xsl:variable name="operators4" select="'%:%:'"/>
  <xsl:variable name="operators3" select="'&gt;&gt;= &lt;&lt;= -&gt;* ...'"/>
  <xsl:variable name="operators2" select="'.* :: ## &lt;: :&gt; &lt;% %&gt; %: += -= *= /= %= ^= &amp;= |= &lt;&lt; &gt;&gt; == != &lt;= &gt;= &amp;&amp; || ++ -- -&gt;'"/>
  <xsl:variable name="operators1" select="'. ? { } [ ] # ( ) ; : + - * / % ^ &amp; | ~ ! = &lt; &gt; ,'"/>
  <xsl:variable name="single-quote">'</xsl:variable>
  
  <!-- Syntax highlighting -->
  <xsl:template name="highlight-keyword">
    <xsl:param name="keyword"/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="keyword">
          <xsl:value-of select="$keyword"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$keyword"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <xsl:template name="highlight-identifier">
    <xsl:param name="identifier"/>
    <xsl:choose>
      <xsl:when test="contains($keywords, concat(' ', $identifier, ' '))">
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="$identifier"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="identifier">
          <xsl:value-of select="$identifier"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$identifier"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-comment">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="comment">
          <xsl:copy-of select="$text"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-special">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="special">
          <xsl:value-of select="$text"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-number">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="number">
          <xsl:value-of select="$text"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-string">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="string">
          <xsl:value-of select="$text"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-char">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="char">
          <xsl:value-of select="$text"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <xsl:template name="highlight-pp-directive">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="$boost.syntax.highlight='1'">
        <phrase role="preprocessor">
          <xsl:value-of select="$text"/>
        </phrase>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <xsl:template name="highlight-text-ident-length">
    <xsl:param name="text"/>
    <xsl:param name="pos" select="1"/>
    <xsl:choose>
      <xsl:when test="string-length($text) + 1 = $pos">
        <xsl:value-of select="$pos - 1"/>
      </xsl:when>
      <xsl:when test="contains($id-chars, substring($text, $pos, 1))">
        <xsl:call-template name ="highlight-text-ident-length">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="pos" select="$pos + 1"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$pos - 1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <xsl:template name="highlight-text-number-length">
    <xsl:param name="text"/>
    <xsl:param name="pos" select="1"/>
    <xsl:choose>
      <xsl:when test="string-length($text) + 1 = $pos">
        <xsl:value-of select="$pos - 1"/>
      </xsl:when>
      <xsl:when test="contains($number-chars, substring($text, $pos, 1))">
        <xsl:call-template name ="highlight-text-ident-length">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="pos" select="$pos + 1"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$pos - 1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-text-string-length">
    <xsl:param name="text"/>
    <xsl:param name="terminator"/>
    <xsl:param name="pos" select="2"/>
    <xsl:choose>
      <xsl:when test="string-length($text) + 1 = $pos">
        <xsl:value-of select="$pos - 1"/>
      </xsl:when>
      <xsl:when test="substring($text, $pos, 1) = $terminator">
        <xsl:value-of select="$pos"/>
      </xsl:when>
      <xsl:when test="substring($text, $pos, 1) = '\' and
                      string-length($text) != $pos">
        <xsl:call-template name="highlight-text-string-length">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="terminator" select="$terminator"/>
          <xsl:with-param name="pos" select="$pos + 2"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="highlight-text-string-length">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="terminator" select="$terminator"/>
          <xsl:with-param name="pos" select="$pos + 1"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-text-operator-length">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="string-length($text) &gt;= 4 and
                      not(contains(substring($text, 1, 4), ' ')) and
                      contains($operators4, substring($text, 1, 4))">
        <xsl:value-of select="4"/>
      </xsl:when>
      <xsl:when test="string-length($text) &gt;= 3 and
                      not(contains(substring($text, 1, 3), ' ')) and
                      contains($operators3, substring($text, 1, 3))">
        <xsl:value-of select="3"/>
      </xsl:when>
      <xsl:when test="string-length($text) &gt;= 2 and
                      not(contains(substring($text, 1, 2), ' ')) and
                      contains($operators2, substring($text, 1, 2))">
        <xsl:value-of select="2"/>
      </xsl:when>
      <xsl:when test="string-length($text) &gt;= 1 and
                      not(contains(substring($text, 1, 1), ' ')) and
                      contains($operators1, substring($text, 1, 1))">
        <xsl:value-of select="1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="0"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-text-pp-directive-length">
    <xsl:param name="text"/>
    <!-- Assume that the first character is a # -->
    <xsl:param name="pos" select="2"/>
    <xsl:choose>
      <xsl:when test="contains($id-chars, substring($text, $pos, 1))">
        <xsl:call-template name="highlight-text-ident-length">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="pos" select="$pos + 1"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="contains(' &#x9;', substring($text, $pos, 1))">
        <xsl:call-template name="highlight-text-pp-directive-length">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="pos" select="$pos + 1"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$pos - 1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-text-impl-leading-whitespace">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="string-length($text) = 0"/>
      <xsl:when test="contains(' &#xA;&#xD;&#x9;', substring($text, 1, 1))">
        <xsl:value-of select="substring($text, 1, 1)"/>
        <xsl:call-template name="highlight-text-impl-leading-whitespace">
          <xsl:with-param name="text" select="substring($text, 2)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="'#' = substring($text, 1, 1)">
        <xsl:variable name="pp-length">
          <xsl:call-template name="highlight-text-pp-directive-length">
            <xsl:with-param name="text" select="$text"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="highlight-pp-directive">
          <xsl:with-param name="text" select="substring($text, 1, $pp-length)"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-text-impl-root">
          <xsl:with-param name="text" select="substring($text, $pp-length + 1)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="highlight-text-impl-root">
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <xsl:template name="highlight-text-impl-root">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="string-length($text) = 0"/>
      <xsl:when test="contains($id-start-chars, substring($text, 1, 1))">
        <xsl:variable name="ident-length">
          <xsl:call-template name="highlight-text-ident-length">
            <xsl:with-param name="text" select="$text"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="highlight-identifier">
          <xsl:with-param name="identifier" select="substring($text, 1, $ident-length)"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-text-impl-root">
          <xsl:with-param name="text" select="substring($text, $ident-length + 1)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="contains($digits, substring($text, 1, 1))">
        <xsl:variable name="number-length">
          <xsl:call-template name="highlight-text-number-length">
            <xsl:with-param name="text" select="$text"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="highlight-number">
          <xsl:with-param name="text" select="substring($text, 1, $number-length)"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-text-impl-root">
          <xsl:with-param name="text" select="substring($text, $number-length + 1)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="substring($text, 1, 1) = '&#x22;'">
        <xsl:variable name="string-length">
          <xsl:call-template name="highlight-text-string-length">
            <xsl:with-param name="text" select="$text"/>
            <xsl:with-param name="terminator" select="'&#x22;'"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="highlight-string">
          <xsl:with-param name="text" select="substring($text, 1, $string-length)"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-text-impl-root">
          <xsl:with-param name="text" select="substring($text, $string-length + 1)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="substring($text, 1, 1) = $single-quote">
        <xsl:variable name="char-length">
          <xsl:call-template name="highlight-text-string-length">
            <xsl:with-param name="text" select="$text"/>
            <xsl:with-param name="terminator" select="$single-quote"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="highlight-char">
          <xsl:with-param name="text" select="substring($text, 1, $char-length)"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-text-impl-root">
          <xsl:with-param name="text" select="substring($text, $char-length + 1)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="substring($text, 1, 2) = '//'">
        <xsl:choose>
          <xsl:when test="contains($text, '&#xA;')">
            <xsl:call-template name="highlight-comment">
              <xsl:with-param name="text" select="substring-before($text, '&#xA;')"/>
            </xsl:call-template>
            <xsl:call-template name="highlight-text-impl-root">
              <xsl:with-param name="text" select="concat('&#xA;', substring-after($text, '&#xA;'))"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="highlight-comment">
              <xsl:with-param name="text" select="$text"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="substring($text, 1, 2) = '/*'">
        <xsl:variable name="after-start" select="substring($text, 3)" />
        <xsl:choose>
          <xsl:when test="contains($after-start, '*/')">
            <xsl:call-template name="highlight-comment">
              <xsl:with-param name="text" select="concat('/*', substring-before($after-start, '*/'), '*/')"/>
            </xsl:call-template>
            <xsl:call-template name="highlight-text-impl-root">
              <xsl:with-param name="text" select="substring-after($after-start, '*/')"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="highlight-comment">
              <xsl:with-param name="text" select="$text"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="contains('&#xA;&#xD;', substring($text, 1, 1))">
        <xsl:value-of select="substring($text, 1, 1)"/>
        <xsl:call-template name="highlight-text-impl-leading-whitespace">
          <xsl:with-param name="text" select="substring($text, 2)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="contains(' &#x9;', substring($text, 1, 1))">
        <xsl:value-of select="substring($text, 1, 1)"/>
        <xsl:call-template name="highlight-text-impl-root">
          <xsl:with-param name="text" select="substring($text, 2)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="operator-length">
          <xsl:call-template name="highlight-text-operator-length">
            <xsl:with-param name="text" select="$text"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$operator-length = 0">
            <xsl:value-of select="substring($text, 1, 1)"/>
            <xsl:call-template name="highlight-text-impl-root">
              <xsl:with-param name="text" select="substring($text, 2)"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="highlight-special">
              <xsl:with-param name="text" select="substring($text, 1, $operator-length)"/>
            </xsl:call-template>
            <xsl:call-template name="highlight-text-impl-root">
              <xsl:with-param name="text" select="substring($text, $operator-length + 1)"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Jam syntax highlighting -->

  <xsl:variable name="jam-keywords" select="' actions bind case class default else for if ignore in include local module on piecemeal quietly return rule switch together updated while '"/>
  <xsl:variable name="jam-operators" select="' ! != &amp; &amp;&amp; ( ) += : ; &lt; &lt;= = &gt; &gt;= ?= [ ] { | || } '"/>

  <xsl:template name="highlight-jam-word">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="contains($jam-keywords, concat(' ', $text, ' '))">
        <xsl:call-template name="highlight-keyword">
          <xsl:with-param name="keyword" select="$text"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="contains($jam-operators, concat(' ', $text, ' '))">
        <xsl:call-template name="highlight-special">
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="jam-word-length">
    <xsl:param name="text"/>
    <xsl:param name="pos" select="1"/>
    <xsl:choose>
      <xsl:when test="string-length($text) + 1= $pos">
        <xsl:value-of select="$pos - 1"/>
      </xsl:when>
      <xsl:when test="contains(' &#xA;&#xD;&#x9;', substring($text, $pos, 1))">
        <xsl:value-of select="$pos - 1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="jam-word-length">
          <xsl:with-param name="text" select="$text"/>
          <xsl:with-param name="pos" select="$pos + 1"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="highlight-jam-text">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="string-length($text) = 0"/>
      <xsl:when test="contains(' &#xA;&#xD;&#x9;', substring($text, 1, 1))">
        <xsl:value-of select="substring($text, 1, 1)"/>
        <xsl:call-template name="highlight-jam-text">
          <xsl:with-param name="text" select="substring($text, 2)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="substring($text, 1, 1) = '#'">
        <xsl:choose>
          <xsl:when test="contains($text, '&#xA;')">
            <xsl:call-template name="highlight-comment">
              <xsl:with-param name="text" select="substring-before($text, '&#xA;')"/>
            </xsl:call-template>
            <xsl:call-template name="highlight-jam-text">
              <xsl:with-param name="text" select="concat('&#xA;', substring-after($text, '&#xA;'))"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="highlight-comment">
              <xsl:with-param name="text" select="$text"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="length">
          <xsl:call-template name="jam-word-length">
            <xsl:with-param name="text" select="$text"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="highlight-jam-word">
          <xsl:with-param name="text" select="substring($text, 1, $length)"/>
        </xsl:call-template>
        <xsl:call-template name="highlight-jam-text">
          <xsl:with-param name="text" select="substring($text, $length + 1)"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Perform C++ syntax highlighting on the given text -->
  <xsl:template name="highlight-text">
    <xsl:param name="text" select="."/>
    <xsl:call-template name="highlight-text-impl-leading-whitespace">
      <xsl:with-param name="text" select="$text"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="*" mode="highlight">
    <xsl:element name="{name(.)}">
      <xsl:for-each select="./@*">
        <xsl:choose>
          <xsl:when test="local-name(.)='last-revision'">
            <xsl:attribute
              name="rev:last-revision"
              namespace="http://www.cs.rpi.edu/~gregod/boost/tools/doc/revision"
>
              <xsl:value-of select="."/>
            </xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:copy-of select="."/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:apply-templates mode="highlight"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="text()" mode="highlight">
    <xsl:call-template name="source-highlight">
      <xsl:with-param name="text" select="."/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="classname|methodname|functionname|libraryname|enumname|
                       conceptname|macroname|globalname" mode="highlight">
    <xsl:apply-templates select="." mode="annotation"/>
  </xsl:template>

  <xsl:template match="type" mode="highlight">
    <xsl:apply-templates mode="highlight"/>
  </xsl:template>

  <xsl:template match="*" mode="highlight-jam">
    <xsl:apply-templates select="." mode="annotation"/>
  </xsl:template>

  <xsl:template match="text()" mode="highlight-jam">
    <xsl:call-template name="highlight-jam-text">
      <xsl:with-param name="text" select="."/>
    </xsl:call-template>
  </xsl:template>

</xsl:stylesheet>
