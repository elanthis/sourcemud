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
  <xsl:template match="events">
    <appendix id="ap_events">
      <title>Events Reference</title>
      <para>Events are sent between <link linkend="Entity">Entities</link> to indicate that some action has been taken or occured.</para>
      <para>Most events have both an actor and a target.  These are <link linkend="Entity">Entities</link>.  All events have a <link linkend="Room">Room</link>, which is the room they occured in.</para>

      <!-- spit out the events -->
      <xsl:apply-templates select="event"><xsl:sort select="@name"/></xsl:apply-templates>
    </appendix>
  </xsl:template>

  <!-- document an event -->
  <xsl:template match="event">
    <!-- craft section/id -->
    <section id="event_{@name}">
      <!-- title -->
      <title><xsl:value-of select="@name"/></title>
      <!-- doc paragraph -->
      <xsl:apply-templates select="doc"/>
      <!-- the room argument -->
      <para>
        <varname>room</varname>
        <xsl:text> : </xsl:text>
        <xsl:call-template name="type">
          <xsl:with-param name="type">Room</xsl:with-param>
        </xsl:call-template>
      </para>
      <blockquote>
        <para><xsl:text>Room where the event occured.</xsl:text></para>
      </blockquote>
      <!-- the main actor argument -->
      <xsl:if test="actor">
        <para>
          <varname>actor</varname>
          <xsl:text> : </xsl:text>
          <xsl:call-template name="type">
            <xsl:with-param name="type"><xsl:value-of select="actor/@type"/></xsl:with-param>
          </xsl:call-template>
        </para>
        <blockquote>
          <para><xsl:value-of select="actor/@doc" /></para>
        </blockquote>
      </xsl:if>
      <!-- target -->
      <xsl:if test="target">
        <para>
          <varname>target</varname>
          <xsl:text> : </xsl:text>
          <xsl:call-template name="type">
            <xsl:with-param name="type"><xsl:value-of select="target/@type"/></xsl:with-param>
          </xsl:call-template>
        </para>
        <blockquote>
          <para><xsl:value-of select="target/@doc" /></para>
        </blockquote>
      </xsl:if>
      <!-- auxillary entity -->
      <xsl:if test="aux">
        <para>
          <varname>aux</varname>
          <xsl:text> : </xsl:text>
          <xsl:call-template name="type">
            <xsl:with-param name="type"><xsl:value-of select="aux/@type"/></xsl:with-param>
          </xsl:call-template>
        </para>
        <blockquote>
          <para><xsl:value-of select="aux/@doc" /></para>
        </blockquote>
      </xsl:if>
      <!-- additional arguments -->
      <xsl:for-each select="arg">
        <para>
          <varname><xsl:value-of select="@name" /></varname>
          <xsl:text> : </xsl:text>
          <xsl:call-template name="type">
            <xsl:with-param name="type"><xsl:value-of select="@type"/></xsl:with-param>
          </xsl:call-template>
        </para>
        <blockquote>
          <para><xsl:value-of select="@doc" /></para>
        </blockquote>
      </xsl:for-each>
    </section>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: set filetype=xslt shiftwidth=2 tabstop=2 expandtab : -->
