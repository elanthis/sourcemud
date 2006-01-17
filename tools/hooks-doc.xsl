<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [
]>
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <!-- Wrap a <type> element in documentation -->
  <xsl:template name="type">
    <!-- set $type variable -->
    <xsl:param name="type" select="1"/>
    <xsl:element name="link">
      <xsl:attribute name="linkend"><xsl:value-of select="$type"/></xsl:attribute>
      <xsl:element name="type"><xsl:value-of select="$type"/></xsl:element>
    </xsl:element>
  </xsl:template>

  <!-- main()  ~,^ -->
  <xsl:template match="hooks">
    <hook-reference>
      <!-- make an appendix of all the hooks -->
      <appendix id="ch_hooks">
        <title>Hooks</title>
        <!-- spit out the hooks -->
        <xsl:apply-templates select="hook"><xsl:sort select="@name"/></xsl:apply-templates>
      </appendix>
    </hook-reference>
  </xsl:template>

  <!-- document an hook -->
  <xsl:template match="hook">
    <!-- craft section/id -->
    <section id="hook_{@name}">
      <!-- title -->
      <title><xsl:value-of select="@name"/></title>
      <!-- doc paragraph -->
      <xsl:apply-templates select="doc"/>
      <xsl:for-each select="arg">
        <para>
          <varname><xsl:value-of select="@name" /></varname>
          <xsl:text> : </xsl:text>
          <xsl:choose>
            <xsl:when test="type">
              <xsl:call-template name="type">
                <xsl:with-param name="type"><xsl:value-of select="@type"/></xsl:with-param>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="type">
                <xsl:with-param name="type">Mixed</xsl:with-param>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </para>
        <xsl:if test="doc">
          <blockquote>
            <xsl:apply-templates select="doc"/>
          </blockquote>
        </xsl:if>
      </xsl:for-each>
      <!-- args and stuff -->
      <xsl:apply-templates select="arg" mode="doc"/>
      <!-- example code -->
      <xsl:if test="example">
        <example>
        <title>Usage of the <xsl:value-of select="@name"/> hook</title>
          <programlisting><xsl:value-of select="example" /></programlisting>
        </example>
      </xsl:if>
    </section>
  </xsl:template>

  <!-- document an argument -->
  <xsl:template match="arg" mode="doc">
    <!-- only if we have a doc item -->
    <xsl:if test="string-length(doc)">
      <!-- para element -->
      <para>
        <!-- show name -->
        <xsl:element name="varname">
          <xsl:value-of select="name"/>
        </xsl:element>
      </para>
      <!-- spit out the docs -->
      <blockquote>
        <xsl:apply-templates select="doc"/>
      </blockquote>
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
        <xsl:variable name="global" select="@global" />
        <link linkend="{@global}">
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
