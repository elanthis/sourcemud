AUTOMAKE_OPTIONS = subdir-objects

### EXECUTABLE SCRIPTS ###

TOOL_FILES = tools/commit.sh tools/note.sh tools/parse-report.pl tools/new-version.sh

XSL_FILES = tools/script-code.xsl tools/script-doc.xsl tools/events-code.xsl tools/events-doc.xsl tools/events-header.xsl tools/macros-code.xsl tools/hooks-doc.xsl tools/hooks-code.xsl tools/hooks-header.xsl tools/script-header.xsl tools/skills-header.xsl tools/skills-help.xsl

bin_SCRIPTS = awemud

noinst_SCRIPTS = startup bootstrap $(TOOL_FILES)

noinst_DATA = $(XSL_FILES)

EXTRA_DIST = awemud.in startup bootstrap $(TOOL_FILES) $(XSL_FILES)

### XML SOURCES ###

noinst_DATA += src/xml/bindings.xml src/xml/events.xml src/xml/macros.xml src/xml/hooks.xml src/xml/skills.xml

EXTRA_DIST += src/xml/bindings.xml src/xml/events.xml src/xml/macros.xml src/xml/hooks.xml src/xml/skills.xml

### GAME SCRIPTS ###

SX_FILES=\
	scripts/manifest.txt \
	scripts/awemud.sx \
	scripts/string.sx \
	scripts/corerules.sx \
	scripts/player.sx \
	scripts/npc.sx \
	scripts/object.sx \
	scripts/death.sx \
	scripts/start.sx \
	scripts/store.sx \
	scripts/whereis.sx \
	scripts/webserver.sx

AI_FILES=\
	scripts/ai/manifest.txt \
	scripts/ai/monster.ai \
	scripts/ai/animal.ai

EXTRA_DIST += $(SX_FILES) $(AI_FILES)

gamescriptdir = ${localstatedir}/lib/@PACKAGE_TARNAME@/scripts
gamescript_DATA = $(SX_FILES) $(SX_CONTROL)

aiscriptdir = ${localstatedir}/lib/@PACKAGE_TARNAME@/scripts/ai
aiscript_DATA = $(AI_FILES)

### DOCUMENTATION ###

DOCBOOK := doc/awemud-manual.xml
MANUAL_CHAPTERS := \
	doc/ch-introduction.xml \
	doc/ch-installation.xml \
	doc/ch-accounts.xml \
	doc/ch-olc.xml \
	doc/ch-control.xml \
	doc/ch-scripting.xml

if BUILD_DOCS
DOCBOOK_TXT := $(DOCBOOK:%.xml=%.txt)
DOCBOOK_HTML := $(DOCBOOK:%.xml=%.html)
if BUILD_PDFDOCS
DOCBOOK_PDF := $(DOCBOOK:%.xml=%.pdf)
endif
endif

DOCS := $(DOCBOOK_HTML) $(DOCBOOK_PDF) $(DOCBOOK_TXT)
USER_DOCS := doc/parse.txt doc/zmp-x-awemud.txt doc/ruleset.txt

docdir = ${datadir}/doc/@PACKAGE_TARNAME@-@VERSION@
doc_DATA = README NEWS COPYING AUTHORS FAQ $(DOCBOOK_TXT) $(USER_DOCS)

htmldir = ${datadir}/doc/@PACKAGE_TARNAME@-@VERSION@/html
noinst_DATA += $(DOCBOOK_HTML)
if BUILD_DOCS
install-data-local:
	$(mkdir_p) '$(DESTDIR)$(htmldir)'
	$(INSTALL_DATA) doc/awemud-manual.html/* '$(DESTDIR)$(htmldir)'
uninstall-local:
	rm -fr '$(DESTDIR)$(htmldir)'
endif

pdfdir = ${datadir}/doc/@PACKAGE_TARNAME@-@VERSION@/pdf
pdf_DATA = $(DOCBOOK_PDF)

if USE_XSLT
doc/script-ref.xml: tools/script-doc.xsl src/xml/bindings.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/script-doc.xsl' '$(top_srcdir)/src/xml/bindings.xml' > doc/script-ref.xml

doc/event-ref.xml: tools/events-doc.xsl src/xml/events.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/events-doc.xsl' '$(top_srcdir)/src/xml/events.xml' > doc/event-ref.xml

doc/hook-ref.xml: tools/hooks-doc.xsl src/xml/hooks.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/hooks-doc.xsl' '$(top_srcdir)/src/xml/hooks.xml' > doc/hook-ref.xml
endif

doc/awemud-manual.xml: $(MANUAL_CHAPTERS)
	-touch doc/awemud-manual.xml

doc/ch-scripting.xml: doc/script-ref.xml doc/event-ref.xml doc/hook-ref.xml
	-touch doc/ch-scripting.xml

%.html: %.xml
	-@PATH_XMLTO@ --skip-validation -o doc/tmp.html html $<
	-rm -fr $@
	-mv -f doc/tmp.html $@

%.txt: %.xml
	@PATH_XMLTO@ --skip-validation -o doc/ txt $<

%.pdf: %.xml
	@PATH_XMLTO@ --skip-validation -o doc/ pdf $<

EXTRA_DIST += doc/script-ref.xml doc/event-ref.xml doc/hook-ref.xml $(MANUAL_CHAPTERS)
CLEANFILES = $(DOCBOOK_TXT) $(DOCBOOK_PDF)

clean-local:
	-for guide in $(DOCBOOK_HTML) ; do test -d "$$guide" && rm -fr "$$guide" ; done

EXTRA_DIST += NEWS FAQ ChangeLog $(DOCBOOK) $(USER_DOCS)

### CONFIGURATION FILES ###

etcdir = ${sysconfdir}/@PACKAGE_TARNAME@
etc_DATA = awemud.conf hosts.deny

awemud.conf: awemud.conf.in tools/replace.sh
	sh ./tools/replace.sh $(top_srcdir)/awemud.conf.in $(top_builddir)/awemud.conf

EXTRA_DIST += awemud.conf.in hosts.deny
CLEANFILES += awemud.conf

### DATA FILES ###

SQL = data/awemud.sql data/example.sql

PLAYERS = data/admin.ply
PLAYERINFO = data/badnames

ACCOUNTS = data/admin.acct

WORLD_IN = data/time data/weather
WORLD = data/time data/weather
ZONES = \
	data/zones/manifest.txt \
	data/zones/underworld.zone \
	data/zones/saladhin.zone \
	data/zones/catacombs.zone \
	data/zones/darkwood.zone \
	data/zones/example.zone

HELP = data/help/manifest.txt data/help/awemud.help data/help/commands.help data/help/olc.help data/help/skills.help
GAMEDATA = data/messages data/races data/socials data/calendar data/skills data/uniqid data/traits
BLUEPRINTS = \
	data/blueprints/manifest.txt \
	data/blueprints/animals.npcs \
	data/blueprints/undead.npcs \
	data/blueprints/armor.objs \
	data/blueprints/misc.objs \
	data/blueprints/weapons.objs \
	data/blueprints/gems.objs 
HTML_TPL = \
	data/html/manifest.txt \
	data/html/header.tpl \
	data/html/footer.tpl \
	data/html/index.tpl \
	data/html/login.tpl \
	data/html/logout.tpl \
	data/html/account.tpl \
	data/html/stats.tpl

if USE_XSLT
data/help/skills.help: tools/skills-help.xsl src/xml/skills.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/skills-help.xsl' '$(top_srcdir)/src/xml/skills.xml' > $(top_builddir)/data/help/skills.help
endif

DATAFILES = $(WORLD) $(HELP) $(GAMEDATA) $(BLUEPRINTS) $(HTML_TPL)

EXTRA_DIST += $(DATAFILES) $(PLAYERS) $(ACCOUNTS) $(WORLD_IN) $(ZONES) $(PLAYERINFO) $(SQL)

worlddir=${localstatedir}/lib/@PACKAGE_TARNAME@/world
world_DATA = $(WORLD) $(ZONES)

playerdir=${localstatedir}/lib/@PACKAGE_TARNAME@/players
player_DATA = $(PLAYERS) $(PLAYERINFO)

accountdir=${localstatedir}/lib/@PACKAGE_TARNAME@/accounts
account_DATA = $(ACCOUNTS)

helpdir=${localstatedir}/lib/@PACKAGE_TARNAME@/help
help_DATA = $(HELP)

blueprintdir=${localstatedir}/lib/@PACKAGE_TARNAME@/blueprints
blueprint_DATA = $(BLUEPRINTS)

gamedatadir=${localstatedir}/lib/@PACKAGE_TARNAME@/data
gamedata_DATA = $(GAMEDATA)

htmltpldir=${localstatedir}/lib/@PACKAGE_TARNAME@/html
htmltpl_DATA = $(HTML_TPL)

logdatadir=${localstatedir}/log
logdata_DATA =

rundatadir=${localstatedir}/run
rundata_DATA =

### HEADERS ###

noinst_HEADERS=\
	include/common/bitset.h \
	include/common/error.h \
	include/common/fdprintf.h \
	include/common/gcbase.h \
	include/common/gclist.h \
	include/common/gcmap.h \
	include/common/gcset.h \
	include/common/gcvector.h \
	include/common/imanager.h \
	include/common/log.h \
	include/common/mail.h \
	include/common/manifest.h \
	include/common/md5.h \
	include/common/rand.h \
	include/common/regex.h \
	include/common/streams.h \
	include/common/strbuf.h \
	include/common/string.h \
	include/common/time.h \
	include/common/types.h \
	include/scriptix/array.h \
	include/scriptix/atom.h \
	include/scriptix/compiler.h \
	include/scriptix/error.h \
	include/scriptix/function.h \
	include/scriptix/iterator.h \
	include/scriptix/native.h \
	include/scriptix/number.h \
	include/scriptix/opcodes.h \
	include/scriptix/stream.h \
	include/scriptix/string.h \
	include/scriptix/struct.h \
	include/scriptix/system.h \
	include/scriptix/thread.h \
	include/scriptix/type.h \
	include/scriptix/value.h \
	include/scriptix/vimpl.h \
	include/mud/account.h \
	include/mud/action.h \
	include/mud/ai.h \
	include/mud/bindings.h \
	include/mud/body.h \
	include/mud/caffect.h \
	include/mud/clock.h \
	include/mud/color.h \
	include/mud/command.h \
	include/mud/creation.h \
	include/mud/creature.h \
	include/mud/efactory.h \
	include/mud/elist.h \
	include/mud/entity.h \
	include/mud/event.h \
	include/mud/fileobj.h \
	include/mud/filetab.h \
	include/mud/form.h \
	include/mud/gametime.h \
	include/mud/help.h \
	include/mud/http.h \
	include/mud/idmap.h \
	include/mud/login.h \
	include/mud/macro.h \
	include/mud/message.h \
	include/mud/name.h \
	include/mud/network.h \
	include/mud/npc.h \
	include/mud/object.h \
	include/mud/olc.h \
	include/mud/pconn.h \
	include/mud/player.h \
	include/mud/portal.h \
	include/mud/race.h \
	include/mud/room.h \
	include/mud/server.h \
	include/mud/settings.h \
	include/mud/skill.h \
	include/mud/social.h \
	include/mud/tag.h \
	include/mud/telnet.h \
	include/mud/timer.h \
	include/mud/uniqid.h \
	include/mud/weather.h \
	include/mud/zmp.h \
	include/mud/zone.h

if USE_XSLT
include/generated/events.h: tools/events-header.xsl src/xml/events.xml
	mkdir -p $(top_builddir)/include/generated
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/events-header.xsl' '$(top_srcdir)/src/xml/events.xml' > $(top_builddir)/include/generated/events.h

include/generated/skills.h: tools/skills-header.xsl src/xml/skills.xml
	mkdir -p $(top_builddir)/include/generated
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/skills-header.xsl' '$(top_srcdir)/src/xml/skills.xml' > $(top_builddir)/include/generated/skills.h

include/generated/hooks.h: tools/hooks-header.xsl src/xml/hooks.xml
	mkdir -p $(top_builddir)/include/generated
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/hooks-header.xsl' '$(top_srcdir)/src/xml/hooks.xml' > $(top_builddir)/include/generated/hooks.h

include/generated/bindings.h: tools/script-header.xsl src/xml/bindings.xml
	mkdir -p $(top_builddir)/include/generated
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/script-header.xsl' '$(top_srcdir)/src/xml/bindings.xml' > $(top_builddir)/$@
endif

EXTRA_DIST += include/generated/events.h include/generated/hooks.h include/generated/bindings.h include/generated/skills.h
BUILT_SOURCES = include/generated/events.h include/generated/hooks.h include/generated/bindings.h include/generated/skills.h

### EXECUTABLES ###

bin_PROGRAMS=awemud-bin

INCLUDES=-I@top_srcdir@/include

COMMON_SOURCES=\
	src/common/error.cc \
	src/common/fdprintf.cc \
	src/common/gc.cc \
	src/common/imanager.cc \
	src/common/log.cc \
	src/common/manifest.cc \
	src/common/md5.cc \
	src/common/rand.cc \
	src/common/regex.cc \
	src/common/streams.cc \
	src/common/strbuf.cc \
	src/common/strings.cc \
	src/common/time.cc 

SCRIPTIX_SOURCES=\
	src/scriptix/array.cc \
	src/scriptix/atom.cc \
	src/scriptix/compiler.cc \
	src/scriptix/error.cc \
	src/scriptix/eval.cc \
	src/scriptix/function.cc \
	src/scriptix/global.cc \
	src/scriptix/grammar.cc \
	src/scriptix/init_types.cc \
	src/scriptix/iter.cc \
	src/scriptix/lexer.cc \
	src/scriptix/number.cc \
	src/scriptix/optimize.cc \
	src/scriptix/parser.cc \
	src/scriptix/stream.cc \
	src/scriptix/string.cc \
	src/scriptix/struct.cc \
	src/scriptix/system.cc \
	src/scriptix/type.cc \
	src/scriptix/value.cc

EXTRA_DIST += src/scriptix/grammar.hh src/scriptix/grammar.cc src/scriptix/lexer.cc src/scriptix/grammar.yy src/scriptix/lexer.ll

if BUILD_GRAMMAR
src/scriptix/grammar.cc src/scriptix/grammar.hh: src/scriptix/grammar.yy
	@PATH_BISON@ -y -d -o src/scriptix/grammar.cc src/scriptix/grammar.yy
	-[ -f src/scriptix/grammar.cc.h ] && mv -f src/scriptix/grammar.cc.h src/scriptix/grammar.hh

src/scriptix/lexer.cc: src/scriptix/lexer.ll src/scriptix/grammar.hh
	@PATH_FLEX@ -osrc/scriptix/lexer.cc src/scriptix/lexer.ll
endif

GENERATED_SOURCES=\
	src/generated/bindings.cc \
	src/generated/events.cc \
	src/generated/hooks.cc \
	src/generated/macros.cc \
	src/generated/skills.cc

if USE_XSLT
src/generated/bindings.cc: tools/script-code.xsl src/xml/bindings.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/script-code.xsl' '$(top_srcdir)/src/xml/bindings.xml' > src/generated/bindings.cc

src/generated/macros.cc: tools/macros-code.xsl src/xml/macros.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/macros-code.xsl' '$(top_srcdir)/src/xml/macros.xml' > src/generated/macros.cc

src/generated/events.cc: tools/events-code.xsl src/xml/events.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/events-code.xsl' '$(top_srcdir)/src/xml/events.xml' > src/generated/events.cc

src/generated/skills.cc: tools/skills-code.xsl src/xml/skills.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/skills-code.xsl' '$(top_srcdir)/src/xml/skills.xml' > src/generated/skills.cc

src/generated/hooks.cc: tools/hooks-code.xsl src/xml/hooks.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/hooks-code.xsl' '$(top_srcdir)/src/xml/hooks.xml' > src/generated/hooks.cc
endif

EXTRA_DIST += src/generated/bindings.cc src/generated/macros.cc src/generated/events.cc src/generated/hooks.cc src/generated/skills.cc
BUILT_SOURCES += src/generated/bindings.cc src/generated/macros.cc src/generated/events.cc src/generated/hooks.cc src/generated/skills.cc

awemud_bin_SOURCES=\
	$(COMMON_SOURCES) \
	$(SCRIPTIX_SOURCES) \
	$(GENERATED_SOURCES) \
	src/mud/account.cc \
	src/mud/action.cc \
	src/mud/ai.cc \
	src/mud/body.cc \
	src/mud/caffect.cc \
	src/mud/calendar.cc \
	src/mud/char_do.cc \
	src/mud/cmd_admin.cc \
	src/mud/cmd_builder.cc \
	src/mud/cmd_creature.cc \
	src/mud/cmd_gm.cc \
	src/mud/cmd_init.cc \
	src/mud/cmd_player.cc \
	src/mud/combat.cc \
	src/mud/command.cc \
	src/mud/creation.cc \
	src/mud/creature.cc \
	src/mud/efactory.cc \
	src/mud/entity.cc \
	src/mud/event.cc \
	src/mud/fileobj.cc \
	src/mud/filetab.cc \
	src/mud/form.cc \
	src/mud/help.cc \
	src/mud/http.cc \
	src/mud/idmap.cc \
	src/mud/login.cc \
	src/mud/macro.cc \
	src/mud/mail.cc \
	src/mud/main.cc \
	src/mud/menu.cc \
	src/mud/message.cc \
	src/mud/name.cc \
	src/mud/network.cc \
	src/mud/npc.cc \
	src/mud/object.cc \
	src/mud/olc.cc \
	src/mud/player.cc \
	src/mud/pmanager.cc \
	src/mud/portal.cc \
	src/mud/race.cc \
	src/mud/room.cc \
	src/mud/settings.cc \
	src/mud/skill.cc \
	src/mud/social.cc \
	src/mud/telnet.cc \
	src/mud/time.cc \
	src/mud/uniqid.cc \
	src/mud/weather.cc \
	src/mud/zmp.cc \
	src/mud/zone.cc
