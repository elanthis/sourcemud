<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [
]>
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <!-- Wrap a <type> element in documentation -->
  <xsl:template name="type">
    <!-- set $type variable -->
    <xsl:param name="type" select="1"/>
    <xsl:choose>
      <!-- do we actually have the type in the document?  make a link -->
      <xsl:when test="//type[child::name=$type]">
        <xsl:element name="link">
          <xsl:attribute name="linkend"><xsl:value-of select="$type"/></xsl:attribute>
          <xsl:element name="type"><xsl:value-of select="$type"/></xsl:element>
        </xsl:element>
      </xsl:when>
      <!-- just use the <type> DocBook element then -->
      <xsl:otherwise>
        <xsl:element name="type"><xsl:value-of select="$type"/></xsl:element>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- main()  ~,^ -->
  <xsl:template match="script-interface">
    <appendix id="ap_script_ai">
      <title>Script API Reference</title>
      <!-- make an appendix of the global constants -->
      <section>
        <xsl:attribute name="id"><xsl:text>ch_constants</xsl:text></xsl:attribute>
        <title>Constants</title>
        <xsl:element name="para">
          <xsl:text>Constants are named variables in the global scope.  These variables cannot be reassigned.</xsl:text>
        </xsl:element>

        <!-- constants are broken into groups -->
        <xsl:for-each select="global-group">
          <xsl:element name="section">
            <!-- group header/doc -->
            <xsl:attribute name="id"><xsl:value-of select="translate(name,' ','-')"/></xsl:attribute>
            <title><xsl:value-of select="name"/></title>
            <xsl:apply-templates select="doc"/>

            <!-- spit out the globals -->
            <xsl:apply-templates select="global"><xsl:sort select="name"/></xsl:apply-templates>
          </xsl:element>
        </xsl:for-each>

        <!-- ungrouped globals -->
        <xsl:apply-templates select="global"><xsl:sort select="name" /></xsl:apply-templates>
      </section>

      <!-- global functions appendix -->
      <section>
        <xsl:attribute name="id"><xsl:text>ch_functions</xsl:text></xsl:attribute>
        <!-- header/doc -->
        <title>Functions</title>
        <xsl:element name="para">Functions exist in the global scope.  They can be called from within any other body of code.</xsl:element>

        <!-- spit them out -->
        <xsl:apply-templates select="function"><xsl:sort select="name" /></xsl:apply-templates>
      </section>

      <!-- appendix of all types -->
      <section>
        <xsl:attribute name="id"><xsl:text>ch_types</xsl:text></xsl:attribute>
        <!-- header/doc -->
        <title>Types</title>
        <xsl:element name="para">Each type has a set of methods that can be used to query and manipulate the object.</xsl:element>

        <!-- spit out the types -->
        <xsl:apply-templates select="type"><xsl:sort select="name" /></xsl:apply-templates>
      </section>
    </appendix>
  </xsl:template>

  <!-- document a global -->
  <xsl:template match="global">
    <!-- craft the section and id -->
    <xsl:element name="section">
      <xsl:attribute name="id"><xsl:value-of select="name"/></xsl:attribute>
      <!-- no title -->
      <title></title>
      <!-- format -->
      <xsl:element name="para">
        <constant>
          <xsl:value-of select="name" />
        </constant>
        <xsl:text> : </xsl:text>
        <!-- type -->
        <xsl:call-template name="type">
          <xsl:with-param name="type"><xsl:value-of select="type" /></xsl:with-param>
        </xsl:call-template>
      </xsl:element>
      <!-- the doc -->
      <xsl:if test="doc|example">
        <xsl:element name="blockquote">
          <!-- doc paragraphs -->
          <xsl:apply-templates select="doc"/>
          <!-- example -->
          <xsl:if test="example">
            <example>
              <title><xsl:text>Usage of the </xsl:text><xsl:value-of select="name"/> global</title>
              <programlisting>
                <xsl:value-of select="example" />
              </programlisting>
            </example>
          </xsl:if>
        </xsl:element>
      </xsl:if>
    </xsl:element>
  </xsl:template>

  <!-- document a function -->
  <xsl:template match="function">
    <!-- craft section/id -->
    <xsl:element name="section">
      <xsl:attribute name="id"><xsl:value-of select="name"/></xsl:attribute>
      <!-- title -->
      <title>
        <xsl:value-of select="name"/>
      </title>
      <!-- function format -->
      <xsl:element name="para">
        <function>
          <xsl:value-of select="name" />
        </function>
        <xsl:text> (</xsl:text>
        <xsl:apply-templates select="arg" mode="inline" />
        <xsl:if test="varg">
          <xsl:if test="count(arg)">
            <xsl:text>, </xsl:text>
          </xsl:if>
          <xsl:text>...</xsl:text>
        </xsl:if>
        <xsl:text>)</xsl:text>
        <xsl:apply-templates select="return" mode="inline"/>
      </xsl:element>
      <!-- args and stuff -->
      <xsl:if test="arg/doc|return/doc">
        <xsl:element name="blockquote">
          <xsl:apply-templates select="arg" mode="doc"/>
          <xsl:apply-templates select="return" mode="doc"/>
        </xsl:element>
      </xsl:if>
      <!-- doc paragraph -->
      <xsl:apply-templates select="doc"/>
      <!-- example code -->
      <xsl:if test="example">
        <example>
        <title>Usage of the <xsl:value-of select="name"/>() function</title>
          <programlisting><xsl:value-of select="example" /></programlisting>
        </example>
      </xsl:if>
    </xsl:element>
  </xsl:template>
  
  <!-- document a type -->
  <xsl:template match="type">
    <!-- craft section and id -->
    <xsl:element name="section">
      <xsl:attribute name="id"><xsl:value-of select="name"/></xsl:attribute>
      <!-- title -->
      <title><xsl:value-of select="name" /></title>
      <!-- explanatory doc -->
      <xsl:apply-templates select="doc"/>
      <!-- inheritance info -->
      <xsl:if test="parent"><xsl:element name="para"><xsl:text>Inherits from: </xsl:text><link><xsl:attribute name="linkend"><xsl:value-of select="parent"/></xsl:attribute><xsl:value-of select="parent"/></link></xsl:element></xsl:if>
      <xsl:variable name="id" select="name"/>
      <xsl:if test="count(../type[child::parent=$id])">
        <xsl:element name="para"><xsl:text>Inherited by: </xsl:text>
          <xsl:for-each select="../type[child::parent=$id]">
            <xsl:if test="position()!=1"><xsl:text>, </xsl:text></xsl:if>
            <link><xsl:attribute name="linkend"><xsl:value-of select="name"/></xsl:attribute><xsl:value-of select="name"/></link>
          </xsl:for-each>
        </xsl:element>
      </xsl:if>
      <!-- all the methods in the type -->
      <xsl:apply-templates select="method"><xsl:sort select="name" /></xsl:apply-templates>
    </xsl:element>
  </xsl:template>
  
  <!-- document a type method -->
  <xsl:template match="method">
    <!-- craft section/id -->
    <xsl:element name="section">
      <xsl:attribute name="id"><xsl:value-of select="../name"/><xsl:text>.</xsl:text><xsl:value-of select="name"/></xsl:attribute>
      <!-- no title -->
      <title></title>
      <!-- format doc -->
      <xsl:element name="para">
        <function><xsl:value-of select="name" /></function>
        <xsl:text> (</xsl:text>
        <xsl:apply-templates select="arg" mode="inline"/>
        <xsl:if test="varg">
          <xsl:if test="count(arg)"><xsl:text>, </xsl:text></xsl:if>
          <xsl:text>...</xsl:text>
        </xsl:if>
        <xsl:text>)</xsl:text>
        <xsl:if test="return/type"><xsl:text> : </xsl:text><xsl:call-template name="type"><xsl:with-param name="type"><xsl:value-of select="return/type" /></xsl:with-param></xsl:call-template></xsl:if>
      </xsl:element>
      <!-- args n stuff -->
      <xsl:if test="arg/doc|return/doc|doc|example">
        <xsl:element name="blockquote">
          <xsl:apply-templates select="arg" mode="doc"/>
          <xsl:apply-templates select="return" mode="doc"/>
          <!-- doc paragraph -->
          <xsl:apply-templates select="doc"/>
          <!-- example code -->
          <xsl:if test="example">
            <example>
              <title>Usage of the <xsl:value-of select="../name"/><xsl:text>.</xsl:text><xsl:value-of select="name"/>() method</title>
              <programlisting><xsl:value-of select="example" /></programlisting>
            </example>
          </xsl:if>
        </xsl:element>
      </xsl:if>
    </xsl:element>
  </xsl:template>

  <!-- inline argument list for method format -->
  <xsl:template match="arg" mode="inline">
    <!-- comman for all args after first -->
    <xsl:if test="position()>1">, </xsl:if>
    <!-- print the variable name -->
    <xsl:element name="varname">
      <xsl:value-of select="name" />
    </xsl:element>
    <!-- if we have a type specifier, show it all nice and perty -->
    <xsl:if test="type">
      <!-- type separtor -->
      <xsl:text> : </xsl:text>
      <!-- type link -->
      <xsl:call-template name="type">
        <xsl:with-param name="type"><xsl:value-of select="type"/></xsl:with-param>
      </xsl:call-template>
      <!-- allow null(nil) values? -->
      <xsl:if test="nullok">
        <xsl:text>|</xsl:text>
        <xsl:element name="type">
          <xsl:text>nil</xsl:text>
        </xsl:element>
      </xsl:if>
    </xsl:if>
  </xsl:template>

  <!-- document an argument -->
  <xsl:template match="arg" mode="doc">
    <!-- only if we have a doc item -->
    <xsl:if test="string-length(doc)">
      <!-- para element -->
      <xsl:element name="para">
        <!-- show name -->
        <xsl:element name="varname">
          <xsl:value-of select="name"/>
        </xsl:element>
        <!-- spit out the docs -->
        <xsl:text> - </xsl:text>
        <xsl:apply-templates select="doc" mode="inline"/>
      </xsl:element>
    </xsl:if>
  </xsl:template>

  <!-- generate return value type -->
  <xsl:template match="return" mode="inline">
    <!-- separator -->
    <xsl:text> : </xsl:text>
    <!-- generate type name/link -->
    <xsl:call-template name="type">
      <xsl:with-param name="type"><xsl:value-of select="type" /></xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- return document -->
  <xsl:template match="return" mode="doc">
    <!-- only if we have a doc element -->
    <xsl:if test="string-length(doc)">
      <!-- output para -->
      <xsl:element name="para">
        <!-- name is 'return' -->
        <xsl:text>return</xsl:text>
        <!-- the documentation itself -->
        <xsl:text> - </xsl:text>
        <xsl:apply-templates select="doc" mode="inline"/>
      </xsl:element>
    </xsl:if>
  </xsl:template>

  <!-- document info -->
  <xsl:template match="doc">
    <!-- wrap in para element -->
    <xsl:element name="para">
      <!-- spit it out -->
      <xsl:apply-templates select="text()|link"/>
    </xsl:element>
  </xsl:template>

  <!-- inline document info -->
  <xsl:template match="doc" mode="inline">
    <!-- spit it out -->
    <xsl:apply-templates select="text()|link"/>
  </xsl:template>

  <!-- generate links -->
  <xsl:template match="link">
    <xsl:choose>
      <!-- web link? -->
      <xsl:when test="@url">
        <xsl:element name="ulink">
          <xsl:attribute name="url"><xsl:value-of select="@url"/></xsl:attribute>
          <xsl:value-of select="."/>
        </xsl:element>
      </xsl:when>
      <!-- email link? -->
      <xsl:when test="@email">
        <xsl:element name="email">
          <xsl:value-of select="."/>
        </xsl:element>
      </xsl:when>
      <!-- function link? -->
      <xsl:when test="@function">
        <xsl:variable name="function" select="@function" />
        <link linkend="{@function}">
          <xsl:choose>
            <xsl:when test="string-length(.)"><xsl:value-of select="."/></xsl:when>
            <xsl:otherwise><xsl:value-of select="@function"/>()</xsl:otherwise>
          </xsl:choose>
        </link>
      </xsl:when>
      <!-- type link? -->
      <xsl:when test="@type">
        <xsl:variable name="type" select="@type" />
        <link linkend="{@type}">
          <xsl:choose>
            <xsl:when test="string-length(.)"><xsl:value-of select="."/></xsl:when>
            <xsl:otherwise><xsl:value-of select="@type"/></xsl:otherwise>
          </xsl:choose>
        </link>
      </xsl:when>
      <!-- global link? -->
      <xsl:when test="@global">
        <xsl:variable name="global" select="translate(@global,' ','-')" />
        <link linkend="{$global}">
          <xsl:choose>
            <xsl:when test="string-length(.)"><xsl:value-of select="."/></xsl:when>
            <xsl:otherwise><xsl:value-of select="@global"/></xsl:otherwise>
          </xsl:choose>
        </link>
      </xsl:when>
      <!-- method link? -->
      <xsl:when test="@method">
        <link linkend="{@method}">
          <xsl:choose>
            <xsl:when test="string-length(.)"><xsl:value-of select="."/></xsl:when>
            <xsl:otherwise><xsl:value-of select="substring-before(@method,'.')"/>.<xsl:value-of select="substring-after(@method,'.')"/>()</xsl:otherwise>
          </xsl:choose>
        </link>
      </xsl:when>
      <!-- eh -->
      <xsl:otherwise>
        <xsl:text>[link: </xsl:text><xsl:value-of select="."/><xsl:text>]</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
<!-- vim: set filetype=xslt shiftwidth=2 tabstop=2 expandtab : -->
