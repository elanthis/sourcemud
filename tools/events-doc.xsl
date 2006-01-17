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
    <event-reference>
      <!-- make an appendix of all the events -->
      <appendix id="ch_events">
        <title>Events</title>
        <para>Events are sent between <link linkend="Entity">Entities</link> to indicate that some action has been taken or occured.</para>
        <para>Most events have both an actor and a target.  These are <link linkend="Entity">Entities</link>.  All events have a <link linkend="Room">Room</link>, which is the room they occured in.</para>

        <!-- spit out the events -->
        <xsl:apply-templates select="event"><xsl:sort select="name"/></xsl:apply-templates>
      </appendix>
    </event-reference>
  </xsl:template>

  <!-- document an event -->
  <xsl:template match="event">
    <!-- craft section/id -->
    <section id="event_{name}">
      <!-- title -->
      <title><xsl:value-of select="name"/></title>
      <!-- doc paragraph -->
      <xsl:apply-templates select="doc"/>
      <!-- the room argument -->
      <xsl:if test="room">
        <para>
          <varname>room</varname>
          <xsl:text> : </xsl:text>
          <xsl:call-template name="type">
            <xsl:with-param name="type">Room</xsl:with-param>
          </xsl:call-template>
        </para>
        <blockquote>
          <xsl:choose>
            <xsl:when test="room/doc">
              <xsl:apply-templates select="room/doc"/>
            </xsl:when>
            <xsl:otherwise>
              <para><xsl:text>Room where the event occured.</xsl:text></para>
            </xsl:otherwise>
          </xsl:choose>
        </blockquote>
      </xsl:if>
      <!-- the main actor argument -->
      <xsl:if test="actor">
        <para>
          <varname>actor</varname>
          <xsl:text> : </xsl:text>
          <xsl:choose>
            <xsl:when test="actor/type">
              <xsl:call-template name="type">
                <xsl:with-param name="type"><xsl:value-of select="actor/type"/></xsl:with-param>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="type">
                <xsl:with-param name="type">Entity</xsl:with-param>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </para>
        <blockquote>
          <xsl:choose>
            <xsl:when test="actor/doc">
              <xsl:apply-templates select="actor/doc"/>
            </xsl:when>
            <xsl:otherwise>
              <para><xsl:text>Cause or invoker of the action which triggered the event.</xsl:text></para>
            </xsl:otherwise>
          </xsl:choose>
        </blockquote>
      </xsl:if>
      <!-- auxillary data -->
      <xsl:for-each select="arg">
        <para>
          <varname><xsl:value-of select="name" /></varname>
          <xsl:text> : </xsl:text>
          <xsl:choose>
            <xsl:when test="type">
              <xsl:call-template name="type">
                <xsl:with-param name="type"><xsl:value-of select="type"/></xsl:with-param>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="type">
                <xsl:with-param name="type">Mixed</xsl:with-param>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </para>
        <blockquote>
          <xsl:choose>
            <xsl:when test="doc">
              <xsl:apply-templates select="doc"/>
            </xsl:when>
            <xsl:otherwise>
              <para><xsl:text>Auxillary data passed with the event.  FIXME: why isn't this data documented, eh?</xsl:text></para>
            </xsl:otherwise>
          </xsl:choose>
        </blockquote>
      </xsl:for-each>
      <!-- args and stuff -->
      <xsl:apply-templates select="arg" mode="doc"/>
      <!-- example code -->
      <xsl:if test="example">
        <example>
        <title>Usage of the <xsl:value-of select="name"/> event</title>
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
