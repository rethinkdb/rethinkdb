<?xml version="1.0" ?>

<!--
Copyright (c) 2002-2003 The Trustees of Indiana University.
                        All rights reserved.
Copyright (c) 2000-2001 University of Notre Dame. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt) -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:template name="unparse-cpp">
    <xsl:param name="typeref"/>
    <xsl:param name="definition_list"/>
    <xsl:param name="priority">0</xsl:param>
    <xsl:param name="ignore-cv" select="false()"/>
    <xsl:param name="ignore-references" select="false()"/>
    <xsl:param name="notations"/>
    <xsl:param name="ignore-notation" select="false()"/>
    <xsl:param name="print-updated-notation" select="false()"/>
    <xsl:param name="use-typename" select="false()"/>
    <xsl:param name="const-if-not-mutable-value" select="'const-if-not-mutable'"/>

    <xsl:variable name="notation_check">
      <xsl:if test="not($ignore-notation)"> <!-- Prevent infinite recursion -->
	<xsl:call-template name="unparse-cpp">
	  <xsl:with-param name="typeref" select="$typeref"/>
	  <xsl:with-param name="definition_list" select="$definition_list"/>
	  <xsl:with-param name="priority">0</xsl:with-param>
	  <xsl:with-param name="ignore-cv" select="true()"/>
	  <xsl:with-param name="ignore-references" select="true()"/>
	  <xsl:with-param name="notations" select="$notations"/>
	  <xsl:with-param name="ignore-notation" select="true()"/>
	  <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	</xsl:call-template>
      </xsl:if>
    </xsl:variable>

    <!--
    <xsl:message>Notation check: <xsl:value-of select="$notation_check"/>
		 Notations: <xsl:value-of select="$notations"/>
    </xsl:message> -->

    <xsl:variable name="this_op_priority" select="document('cpp-operators.xml')/operator-list/op[@name=name($typeref)]/apply/@priority"/>

    <xsl:variable name="result">

      <xsl:variable name="subcall_priority">
	<xsl:choose>
	  <xsl:when test="true() or ($this_op_priority &gt; $priority)">
	    <xsl:value-of select="$this_op_priority"/>
	  </xsl:when>
	  <!-- <xsl:otherwise>0</xsl:otherwise> -->
	</xsl:choose>
      </xsl:variable>

      <xsl:if test="$this_op_priority &lt;= $priority">(</xsl:if>

      <xsl:choose>

	<xsl:when test="name($typeref)='sample-value'"
	  >boost::sample_value &lt; <xsl:call-template name="unparse-cpp"><xsl:with-param name="typeref" select="$typeref/*[1]"/><xsl:with-param name="definition_list" select="$definition_list"/><xsl:with-param name="ignore-cv" select="$ignore-cv"/><xsl:with-param name="notations" select="$notations"/><xsl:with-param name="ignore-references" select="$ignore-references"/><xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/></xsl:call-template> &gt;()</xsl:when>

	<xsl:when test="name($typeref)='reference-to'"
	  ><xsl:call-template name="unparse-cpp"><xsl:with-param name="typeref" select="$typeref/*[1]"/><xsl:with-param name="definition_list" select="$definition_list"/><xsl:with-param name="priority" select="$subcall_priority"/><xsl:with-param name="ignore-cv" select="$ignore-cv"/><xsl:with-param name="notations" select="$notations"/><xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/></xsl:call-template><xsl:if test="not($ignore-references)"> &amp;</xsl:if></xsl:when>

	<xsl:when test="name($typeref)='pointer-to'"
	  ><xsl:call-template name="unparse-cpp"><xsl:with-param name="typeref" select="$typeref/*[1]"/><xsl:with-param name="definition_list" select="$definition_list"/><xsl:with-param name="priority" select="$subcall_priority"/><xsl:with-param name="ignore-cv" select="$ignore-cv"/><xsl:with-param name="notations" select="$notations"/></xsl:call-template> *</xsl:when>

	<xsl:when test="name($typeref)='const'"
	><xsl:if test="not($ignore-cv)">const </xsl:if><xsl:call-template name="unparse-cpp"><xsl:with-param name="typeref" select="$typeref/*[1]"/><xsl:with-param name="definition_list" select="$definition_list"/><xsl:with-param name="priority" select="$subcall_priority"/><xsl:with-param name="ignore-cv" select="$ignore-cv"/><xsl:with-param name="notations" select="$notations"/><xsl:with-param name="ignore-references" select="$ignore-references"/><xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/></xsl:call-template>
	</xsl:when>

	<xsl:when test="name($typeref)='const-if-not-mutable'"
	><xsl:if test="not($ignore-cv)"><xsl:value-of select="$const-if-not-mutable-value"/><xsl:if test="$const-if-not-mutable-value"><xsl:text> </xsl:text></xsl:if></xsl:if><xsl:call-template name="unparse-cpp"><xsl:with-param name="typeref" select="$typeref/*[1]"/><xsl:with-param name="definition_list" select="$definition_list"/><xsl:with-param name="priority" select="$subcall_priority"/><xsl:with-param name="ignore-cv" select="$ignore-cv"/><xsl:with-param name="notations" select="$notations"/><xsl:with-param name="ignore-references" select="$ignore-references"/><xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/></xsl:call-template>
	</xsl:when>

	<xsl:when test="name($typeref)='volatile'"
	><xsl:if test="not($ignore-cv)">volatile </xsl:if><xsl:call-template name="unparse-cpp"><xsl:with-param name="typeref" select="$typeref/*[1]"/><xsl:with-param name="definition_list" select="$definition_list"/><xsl:with-param name="priority" select="$subcall_priority"/><xsl:with-param name="ignore-cv" select="$ignore-cv"/><xsl:with-param name="notations" select="$notations"/><xsl:with-param name="ignore-references" select="$ignore-references"/><xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/></xsl:call-template>
	</xsl:when>

	<xsl:when test="name($typeref)='apply-template'">
	  <xsl:value-of select="$typeref/@name"/>&lt;<xsl:for-each select="$typeref/*">
	      <xsl:if test="position()!=1">, </xsl:if><xsl:comment/>
	      <xsl:call-template name="unparse-cpp">
		<xsl:with-param name="typeref" select="."/>
		<xsl:with-param name="definition_list" select="$definition_list"/>
		<xsl:with-param name="ignore-cv" select="$ignore-cv"/>
		<xsl:with-param name="notations" select="$notations"/>
		<xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	      </xsl:call-template></xsl:for-each
	    ><xsl:comment/>&gt;</xsl:when>

	<xsl:when test="name($typeref)='get-member-type'">
	  <xsl:call-template name="unparse-cpp"><xsl:with-param name="typeref" select="$typeref/*[1]"/><xsl:with-param name="definition_list" select="$definition_list"/><xsl:with-param name="ignore-cv" select="$ignore-cv"/><xsl:with-param name="notations" select="$notations"/><xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/></xsl:call-template>::<xsl:value-of select="$typeref/@name"/>
	</xsl:when>

	<xsl:when test="name($typeref)='type'">
	  <xsl:variable name="typeref_value" select="normalize-space(substring-before(substring-after($definition_list,concat('@(@',$typeref/@name,'=')),'@)@'))"/>
	  <xsl:choose>
	    <xsl:when test="$typeref_value=''">
	      <xsl:value-of select="$typeref/@name"/><xsl:comment/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="$typeref_value"/><xsl:comment/>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:when>

	<xsl:when test="name($typeref)='documentation'"/>

	<xsl:when test="document('cpp-operators.xml')/operator-list/op[@name=name($typeref)]">
	  <xsl:variable name="op_file" select="document('cpp-operators.xml')/operator-list"/>
	  <xsl:variable name="op_info" select="$op_file/op[@name=name($typeref)]/apply/."/>

	  <xsl:call-template name="unparse-operator-definition">
	    <xsl:with-param name="typeref" select="$typeref"/>
	    <xsl:with-param name="operator_nodeset" select="$op_info/child::node()"/>
	    <xsl:with-param name="my_priority" select="$subcall_priority"/>
	    <xsl:with-param name="definition_list" select="$definition_list"/>
	    <xsl:with-param name="notations" select="$notations"/>
	    <xsl:with-param name="ignore-cv" select="$ignore-cv"/>
	    <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	    <xsl:with-param name="print-updated-notation" select="$print-updated-notation"/>
	  </xsl:call-template>

	</xsl:when>

	<xsl:otherwise>
	  (Unrecognized tag <xsl:value-of select="name($typeref)"/>)
	</xsl:otherwise>
      </xsl:choose>

      <!-- Close parenthesis code moved below -->

    </xsl:variable>

    <!-- <xsl:message>ignore-notation = <xsl:value-of select="$ignore-notation"/></xsl:message> -->
    <!-- <xsl:message>notation_check = <xsl:value-of select="$notation_check"/></xsl:message> -->
    <!-- <xsl:message>notations = <xsl:value-of select="$notations"/></xsl:message> -->
    <!-- <xsl:message>result = <xsl:value-of select="$result"/></xsl:message> -->

    <xsl:variable name="used_notation" select="boolean($notation_check) and boolean(substring-before(substring-after($notations, concat('@@(@@', $notation_check, '@@=@@')),'@@)@@'))"/>

    <xsl:variable name="notations2">
      <!-- Possibly replace from result of unparse-operator-definition -->
      <xsl:choose>
	<xsl:when test="contains($result, ' *@@@* ')">
	  <xsl:value-of select="substring-after($result, ' *@@@* ')"/>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:value-of select="$notations"/>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="result2">
      <!-- Possibly replace from result of unparse-operator-definition -->
      <xsl:choose>
	<xsl:when test="contains($result, ' *@@@* ')">
	  <xsl:value-of select="substring-before($result, ' *@@@* ')"/>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:value-of select="$result"/>
	</xsl:otherwise>
      </xsl:choose>
      <!-- Close parenthesis code -->
      <xsl:if test="$this_op_priority &lt;= $priority">)</xsl:if>
    </xsl:variable>

    <xsl:variable name="notation_varlist">
      <xsl:choose>
	<xsl:when test="$used_notation">
	  <xsl:value-of select="substring-before(substring-after($notations2, concat('@@(@@', $notation_check, '@@=@@')), '@@)@@')"/>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:value-of select="$result2"/>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="new_varlist" select="substring-after(normalize-space($notation_varlist), ' ')"/>

    <xsl:variable name="notation_var">
      <xsl:choose>
	<xsl:when test="not($used_notation)">
	  <xsl:value-of select="$result2"/>
	</xsl:when>
	<xsl:when test="$new_varlist=''">
	  <xsl:value-of select="$notation_varlist"/>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:value-of select="substring-before(normalize-space($notation_varlist), ' ')"/>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- Generate new notation list -->
    <xsl:variable name="new_notations">
      <xsl:choose>
	<xsl:when test="$used_notation">
	  <xsl:value-of select="normalize-space(concat('@@(@@', $notation_check, '@@=@@', $new_varlist, '@@)@@', $notations2))"/>
	  <!-- Duplicate entries always use first occurrance, so I can optimize this -->
	</xsl:when>
	<xsl:otherwise><xsl:value-of select="$notations2"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- <xsl:message>final_result = <xsl:value-of select="normalize-space($final_result)"/></xsl:message> -->

    <xsl:call-template name="add-typename"><xsl:with-param name="really-do-it" select="$use-typename"/><xsl:with-param name="type"><xsl:value-of select="normalize-space($notation_var)"/></xsl:with-param></xsl:call-template><xsl:if test="$print-updated-notation"> *@@@* <xsl:value-of select="$new_notations"/></xsl:if>

  </xsl:template>

  <xsl:template name="unparse-operator-definition">
    <xsl:param name="typeref"/>
    <xsl:param name="operator_nodeset"/>
    <xsl:param name="current_start">1</xsl:param>
    <xsl:param name="my_priority"/>
    <xsl:param name="definition_list"/>
    <xsl:param name="notations"/>
    <xsl:param name="ignore-cv"/>
    <xsl:param name="self"/>
    <xsl:param name="use-code-block" select="false()"/>
    <xsl:param name="print-updated-notation" select="false()"/>
    <xsl:param name="const-if-not-mutable-value"/>

    <xsl:variable name="op_current" select="$operator_nodeset[position()=1]"/>
    <xsl:variable name="op_rest" select="$operator_nodeset[position()!=1]"/>

    <xsl:choose>

      <xsl:when test="count($operator_nodeset)=0">
	<xsl:if test="$print-updated-notation"> *@@@* <xsl:value-of select="$notations"/></xsl:if>
      </xsl:when>

      <xsl:when test="$op_current != $op_current/../*"> <!-- If I am not an element -->
	<xsl:value-of select="$op_current"/>
	<xsl:call-template name="unparse-operator-definition">
	  <xsl:with-param name="typeref" select="$typeref"/>
	  <xsl:with-param name="operator_nodeset" select="$op_rest"/>
	  <xsl:with-param name="my_priority" select="$my_priority"/>
	  <xsl:with-param name="definition_list" select="$definition_list"/>
	  <xsl:with-param name="notations" select="$notations"/>
	  <xsl:with-param name="self" select="$self"/>
	  <xsl:with-param name="use-code-block" select="$use-code-block"/>
	  <xsl:with-param name="print-updated-notation" select="$print-updated-notation"/>
	  <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	</xsl:call-template>
      </xsl:when>

      <xsl:when test="name($op_current)='name'">
	<xsl:value-of select="$typeref/@name"/>
	<xsl:call-template name="unparse-operator-definition">
	  <xsl:with-param name="typeref" select="$typeref"/>
	  <xsl:with-param name="operator_nodeset" select="$op_rest"/>
	  <xsl:with-param name="my_priority" select="$my_priority"/>
	  <xsl:with-param name="definition_list" select="$definition_list"/>
	  <xsl:with-param name="notations" select="$notations"/>
	  <xsl:with-param name="ignore-cv" select="$ignore-cv"/>
	  <xsl:with-param name="self" select="$self"/>
	  <xsl:with-param name="use-code-block" select="$use-code-block"/>
	  <xsl:with-param name="print-updated-notation" select="$print-updated-notation"/>
	  <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	</xsl:call-template>
      </xsl:when>

      <xsl:when test="name($op_current)='self'">
        <xsl:call-template name="concept.link">
          <xsl:with-param name="name" select="string($self)"/>
        </xsl:call-template>
	<xsl:call-template name="unparse-operator-definition">
	  <xsl:with-param name="typeref" select="$typeref"/>
	  <xsl:with-param name="operator_nodeset" select="$op_rest"/>
	  <xsl:with-param name="my_priority" select="$my_priority"/>
	  <xsl:with-param name="definition_list" select="$definition_list"/>
	  <xsl:with-param name="notations" select="$notations"/>
	  <xsl:with-param name="ignore-cv" select="$ignore-cv"/>
	  <xsl:with-param name="self" select="$self"/>
	  <xsl:with-param name="use-code-block" select="$use-code-block"/>
	  <xsl:with-param name="print-updated-notation" select="$print-updated-notation"/>
	  <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	</xsl:call-template>
      </xsl:when>

      <xsl:when test="name($op_current)='arg'">
	<xsl:variable name="num" select="$op_current/@num"/>
	<xsl:variable name="assoc" select="$op_current/../@assoc"/>
	<xsl:variable name="my_priority_before" select="$my_priority"/>
	<xsl:variable name="my_priority">
	  <xsl:choose>
	    <xsl:when test="count($op_current/@priority)">
	      <xsl:value-of select="$op_current/@priority"/>
	    </xsl:when>
	    <xsl:when test="$assoc and ($num = $assoc)">
	      <xsl:value-of select="$my_priority_before - 1"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="$my_priority"/>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:variable>
	<xsl:variable name="typeref-result">
	  <xsl:call-template name="unparse-cpp">
	    <xsl:with-param name="typeref" select="$typeref/*[position()=$num]"/>
	    <xsl:with-param name="definition_list" select="$definition_list"/>
	    <xsl:with-param name="priority" select="$my_priority"/>
	    <xsl:with-param name="ignore-cv" select="$ignore-cv"/>
	    <xsl:with-param name="notations" select="$notations"/>
	    <xsl:with-param name="print-updated-notation" select="true()"/>
	    <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	  </xsl:call-template>
	</xsl:variable>
	<xsl:variable name="typeref-print" select="normalize-space(substring-before($typeref-result, ' *@@@* '))"/>
	<xsl:variable name="new_notations" select="normalize-space(substring-after($typeref-result, ' *@@@* '))"/>

	<xsl:choose>
	  <xsl:when test="$use-code-block">
	    <type><xsl:value-of select="$typeref-print"/></type>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="$typeref-print"/>
	  </xsl:otherwise>
	</xsl:choose>

	<xsl:call-template name="unparse-operator-definition">
	  <xsl:with-param name="typeref" select="$typeref"/>
	  <xsl:with-param name="operator_nodeset" select="$op_rest"/>
	  <xsl:with-param name="my_priority" select="$my_priority_before"/>
	  <xsl:with-param name="definition_list" select="$definition_list"/>
	  <xsl:with-param name="notations" select="$new_notations"/>
	  <xsl:with-param name="ignore-cv" select="$ignore-cv"/>
	  <xsl:with-param name="self" select="$self"/>
	  <xsl:with-param name="use-code-block" select="$use-code-block"/>
	  <xsl:with-param name="print-updated-notation" select="$print-updated-notation"/>
	  <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	</xsl:call-template>
      </xsl:when>

      <xsl:when test="name($op_current)='arg-list'">
	<xsl:variable name="start" select="$op_current/@start"/>
	<xsl:variable name="typeref-result">
	  <xsl:choose>
	    <xsl:when test="$current_start &gt;= $start">
	      <xsl:call-template name="unparse-cpp">
		<xsl:with-param name="typeref" select="$typeref/*[$current_start]"/>
		<xsl:with-param name="definition_list" select="$definition_list"/>
		<xsl:with-param name="priority" select="$my_priority"/>
		<xsl:with-param name="ignore-cv" select="$ignore-cv"/>
		<xsl:with-param name="notations" select="$notations"/>
		<xsl:with-param name="print-updated-notation" select="true()"/>
		<xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	      </xsl:call-template>
	    </xsl:when>

	    <xsl:otherwise>
	       *@@@* <xsl:value-of select="$notations"/>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:variable>

	<xsl:variable name="typeref-print" select="normalize-space(substring-before($typeref-result, ' *@@@* '))"/>
	<xsl:variable name="new_notations" select="normalize-space(substring-after($typeref-result, ' *@@@* '))"/>

	<xsl:choose>
	  <xsl:when test="$use-code-block">
	    <type><xsl:value-of select="$typeref-print"/></type>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="$typeref-print"/>
	  </xsl:otherwise>
	</xsl:choose>

	<xsl:if test="$current_start &gt;= $start">
	  <xsl:if test="$current_start!=count($typeref/*)">, </xsl:if>
	</xsl:if>

	<xsl:choose>
	  <xsl:when test="$current_start != count($typeref/*)">
	    <xsl:call-template name="unparse-operator-definition">
	      <xsl:with-param name="typeref" select="$typeref"/>
	      <xsl:with-param name="operator_nodeset" select="$operator_nodeset"/>
	      <xsl:with-param name="current_start" select="$current_start + 1"/>
	      <xsl:with-param name="my_priority" select="$my_priority"/>
	      <xsl:with-param name="definition_list" select="$definition_list"/>
	      <xsl:with-param name="notations" select="$new_notations"/>
	      <xsl:with-param name="ignore-cv" select="$ignore-cv"/>
	      <xsl:with-param name="self" select="$self"/>
	      <xsl:with-param name="use-code-block" select="$use-code-block"/>
	      <xsl:with-param name="print-updated-notation" select="$print-updated-notation"/>
	      <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	    </xsl:call-template>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:call-template name="unparse-operator-definition">
	      <xsl:with-param name="typeref" select="$typeref"/>
	      <xsl:with-param name="operator_nodeset" select="$op_rest"/>
	      <xsl:with-param name="my_priority" select="$my_priority"/>
	      <xsl:with-param name="definition_list" select="$definition_list"/>
	      <xsl:with-param name="notations" select="$new_notations"/>
	      <xsl:with-param name="ignore-cv" select="$ignore-cv"/>
	      <xsl:with-param name="self" select="$self"/>
	      <xsl:with-param name="use-code-block" select="$use-code-block"/>
	      <xsl:with-param name="const-if-not-mutable-value" select="$const-if-not-mutable-value"/>
	    </xsl:call-template>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:when>

      <xsl:otherwise>Invalid tag in operator definition: <xsl:value-of select="name($op_current)"/></xsl:otherwise>

    </xsl:choose>
  </xsl:template>

  <xsl:template name="add-typename">
    <!-- Adds typename to the front of a string if it is necessary. -->
    <xsl:param name="type"/> <!-- string to prepend to -->
    <xsl:param name="params" select="/concept/param | /concept/define-type | /concept/associated-type"/>
      <!-- nodeset of param tags for concept -->
      <!-- associated types are assumed to be dependent -->
    <xsl:param name="really-do-it"/> <!-- really change anything? -->

    <xsl:variable name="type-after-last-scope">
      <xsl:call-template name="substring-before-last">
	<xsl:with-param name="string" select="$type"/>
	<xsl:with-param name="to-find" select="'::'"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="tokenized-type-after-last-scope">
      <xsl:call-template name="rough-tokenize">
	<xsl:with-param name="string" select="$type-after-last-scope"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$really-do-it and boolean($params[contains($tokenized-type-after-last-scope, concat(' ', @name, ' '))])">
      <!-- If the tokenized string contains any of the param names in a
      token by itself, return true.  Return false otherwise -->
	<xsl:comment/>typename <xsl:value-of select="$type"/><xsl:comment/>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="substring-before-last">
    <xsl:param name="string"/>
    <xsl:param name="to-find"/>
    <xsl:param name="string-processed-so-far"/> <!-- internal -->
    <!-- Find the substring of $string before the last occurrance of
    $to-find, returning '' if it was not found. -->

    <xsl:choose>
      <xsl:when test="contains($string, $to-find)">
	<xsl:call-template name="substring-before-last">
	  <xsl:with-param name="string" select="substring-after($string, $to-find)"/>
	  <xsl:with-param name="to-find" select="$to-find"/>
	  <xsl:with-param name="string-processed-so-far" select="concat($string-processed-so-far, substring-before($string, $to-find), $to-find)"/>
	</xsl:call-template>
      </xsl:when>

      <xsl:otherwise>
	<xsl:value-of select="substring($string-processed-so-far, 1, string-length($string-processed-so-far)-(string-length($to-find)))"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="substring-after-last">
    <xsl:param name="string"/>
    <xsl:param name="to-find"/>
    <!-- Find the substring of $string after the last occurrance of
    $to-find, returning the original string if it was not found. -->

    <xsl:choose>
      <xsl:when test="contains($string, $to-find)">
	<xsl:call-template name="substring-after-last">
	  <xsl:with-param name="string" select="substring-after($string, $to-find)"/>
	  <xsl:with-param name="to-find" select="$to-find"/>
	</xsl:call-template>
      </xsl:when>

      <xsl:otherwise>
	<xsl:value-of select="$string"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="rough-tokenize">
    <xsl:param name="string"/>
    <!-- Do a rough tokenization of the string.  Right now, just translate
    all non-token-chars to spaces, normalize-space the result, and prepend
    and append spaces. -->

    <xsl:value-of select="concat(' ', normalize-space(translate($string, '&lt;&gt;,./?;:[]{}-=\\_+|!@#$%^&amp;*()', '                             ')), ' ')"/>
  </xsl:template>
</xsl:stylesheet>
