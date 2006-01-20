AUTOMAKE_OPTIONS = subdir-objects

### EXECUTABLE SCRIPTS ###

TOOL_FILES = tools/commit.sh tools/note.sh tools/parse-report.pl tools/new-version.sh

XSL_FILES = tools/script-code.xsl tools/script-doc.xsl tools/events-code.xsl tools/events-doc.xsl tools/events-header.xsl tools/parse-code.xsl tools/hooks-doc.xsl tools/hooks-code.xsl tools/hooks-header.xsl

bin_SCRIPTS = awemud

noinst_SCRIPTS = startup bootstrap $(TOOL_FILES)

noinst_DATA = $(XSL_FILES)

EXTRA_DIST = awemud.in startup bootstrap $(TOOL_FILES) $(XSL_FILES)

### DATA SOURCES ###

noinst_DATA += src/xml/script-intr.xml src/xml/event-list.xml src/xml/parse-intr.xml src/xml/hook-list.xml

EXTRA_DIST += src/xml/script-intr.xml src/xml/event-list.xml src/xml/parse-intr.xml src/xml/hook-list.xml

### GAME SCRIPTS ###

SX_FILES=\
	scripts/awemud.sx \
	scripts/string.sx \
	scripts/corerules.sx \
	scripts/player.sx \
	scripts/npc.sx \
	scripts/object.sx \
	scripts/creation.sx \
	scripts/death.sx \
	scripts/store.sx \
	scripts/whereis.sx \
	scripts/example.zone.sx

SX_CONTROL=\
	scripts/load

AI_FILES=\
	scripts/animal.ai

EXTRA_DIST += $(SX_FILES) $(AI_FILES) $(SX_CONTROL)

gamescriptdir = ${localstatedir}/lib/@PACKAGE_TARNAME@/scripts
gamescript_DATA = $(SX_FILES) $(SX_CONTROL)

aiscriptdir = ${localstatedir}/lib/@PACKAGE_TARNAME@/scripts
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
USER_DOCS := \
	doc/parse.txt doc/zmp-x-awemud.txt

docdir = ${datadir}/doc/@PACKAGE_TARNAME@-@VERSION@
doc_DATA = README NEWS COPYING AUTHORS FAQ $(DOCBOOK_TXT) $(USER_DOCS)

htmldir = ${datadir}/doc/@PACKAGE_TARNAME@-@VERSION@/html
html_DATA = $(DOCBOOK_HTML)

pdfdir = ${datadir}/doc/@PACKAGE_TARNAME@-@VERSION@/pdf
pdf_DATA = $(DOCBOOK_PDF)

doc/script-ref.xml: tools/script-doc.xsl src/xml/script-intr.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/script-doc.xsl' '$(top_srcdir)/src/xml/script-intr.xml' > doc/script-ref.xml

doc/event-ref.xml: tools/events-doc.xsl src/xml/event-list.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/events-doc.xsl' '$(top_srcdir)/src/xml/event-list.xml' > doc/event-ref.xml

doc/hook-ref.xml: tools/hooks-doc.xsl src/xml/hook-list.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/hooks-doc.xsl' '$(top_srcdir)/src/xml/hook-list.xml' > doc/hook-ref.xml

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
CLEANFILES = doc/script-ref.xml doc/event-ref.xml doc/hook-ref.xml $(DOCBOOK_TXT) $(DOCBOOK_PDF)

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

PLAYERS_IN = data/admin.ply.in
PLAYERS = data/admin.ply
PLAYERINFO = data/badnames

ACCOUNTS_IN = data/admin.acct.in
ACCOUNTS = data/admin.acct

WORLD_IN = data/time.in data/weather.in
WORLD = data/time data/weather
ZONES = data/example.zone

HELP = data/awemud.help data/commands.help data/olc.help
GAMEDATA = data/messages data/races data/socials data/calendar data/skills data/uniqid data/traits
TEMPLATES = data/animals.npcs data/undead.npcs data/armor.objs data/misc.objs data/weapons.objs data/gems.objs 
DATAFILES = $(WORLD) $(HELP) $(GAMEDATA) $(TEMPLATES)

EXTRA_DIST += $(DATAFILES) $(PLAYERS_IN) $(ACCOUNTS_IN) $(WORLD_IN) $(ZONES) $(PLAYERINFO)

worlddir=${localstatedir}/lib/@PACKAGE_TARNAME@/world
world_DATA = $(WORLD) $(ZONES)

playerdir=${localstatedir}/lib/@PACKAGE_TARNAME@/players
player_DATA = $(PLAYERS) $(PLAYERINFO)

accountdir=${localstatedir}/lib/@PACKAGE_TARNAME@/accounts
account_DATA = $(ACCOUNTS)

helpdir=${localstatedir}/lib/@PACKAGE_TARNAME@/help
help_DATA = $(HELP)

blueprintdir=${localstatedir}/lib/@PACKAGE_TARNAME@/blueprints
blueprint_DATA = $(TEMPLATES)

gamedatadir=${localstatedir}/lib/@PACKAGE_TARNAME@/data
gamedata_DATA = $(GAMEDATA)

logdatadir=${localstatedir}/log
logdata_DATA =

rundatadir=${localstatedir}/run
rundata_DATA =

### HEADERS ###

noinst_HEADERS=\
	include/common/aweregex.h \
	include/common/awestr.h \
	include/common/error.h \
	include/common/gcbase.h \
	include/common/gclist.h \
	include/common/gcmap.h \
	include/common/gcset.h \
	include/common/gcvector.h \
	include/common/imanager.h \
	include/common/log.h \
	include/common/mail.h \
	include/common/md5.h \
	include/common/rand.h \
	include/common/streams.h \
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
	include/scriptix/scriptix.h \
	include/scriptix/struct.h \
	include/scriptix/sxstring.h \
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
	include/mud/char.h \
	include/mud/clock.h \
	include/mud/color.h \
	include/mud/command.h \
	include/mud/container.h \
	include/mud/control.h \
	include/mud/efactory.h \
	include/mud/elist.h \
	include/mud/entity.h \
	include/mud/event.h \
	include/mud/exit.h \
	include/mud/fileobj.h \
	include/mud/gametime.h \
	include/mud/help.h \
	include/mud/idmap.h \
	include/mud/message.h \
	include/mud/name.h \
	include/mud/network.h \
	include/mud/npc.h \
	include/mud/object.h \
	include/mud/olc.h \
	include/mud/parse.h \
	include/mud/pdesc.h \
	include/mud/player.h \
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

include/mud/eventids.h: tools/events-header.xsl src/xml/event-list.xml
	mkdir -p $(top_builddir)/include/mud
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/events-header.xsl' '$(top_srcdir)/src/xml/event-list.xml' > $(top_builddir)/include/mud/eventids.h

include/mud/hooks.h: tools/hooks-header.xsl src/xml/hook-list.xml
	mkdir -p $(top_builddir)/include/mud
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/hooks-header.xsl' '$(top_srcdir)/src/xml/hook-list.xml' > $(top_builddir)/include/mud/hooks.h

EXTRA_DIST += include/mud/eventids.h include/mud/hooks.h
BUILT_SOURCES = include/mud/eventids.h include/mud/hooks.h
CLEANFILES += include/mud/eventids.h include/mud/hooks.h

### EXECUTABLES ###

bin_PROGRAMS=awemud-ctrl awemud-bin

INCLUDES=-I@top_srcdir@/include/common -I@top_srcdir@/include/scriptix -I@top_srcdir@/include/mud -I@top_srcdir@/include

COMMON_SOURCES=\
	src/common/gc.cc \
	src/common/md5.cc \
	src/common/rand.cc \
	src/common/regex.cc \
	src/common/strings.cc

SCRIPTIX_SOURCES=\
	src/scriptix/array.cc \
	src/scriptix/atom.cc \
	src/scriptix/compiler.cc \
	src/scriptix/error.cc \
	src/scriptix/eval.cc \
	src/scriptix/function.cc \
	src/scriptix/global.cc \
	src/scriptix/grammar.cc \
	src/scriptix/iter.cc \
	src/scriptix/lexer.cc \
	src/scriptix/number.cc \
	src/scriptix/optimize.cc \
	src/scriptix/parser.cc \
	src/scriptix/string.cc \
	src/scriptix/struct.cc \
	src/scriptix/system.cc \
	src/scriptix/type.cc \
	src/scriptix/value.cc

EXTRA_DIST+=src/scriptix/grammar.hh src/scriptix/grammar.cc src/scriptix/lexer.cc src/scriptix/grammar.yy src/scriptix/lexer.ll

src/scriptix/grammar.cc src/scriptix/grammar.hh: src/scriptix/grammar.yy
	@PATH_BISON@ -y -d -o src/scriptix/grammar.cc src/scriptix/grammar.yy
	-[ -f src/scriptix/grammar.cc.h ] && mv -f src/scriptix/grammar.cc.h src/scriptix/grammar.hh

src/scriptix/lexer.cc: src/scriptix/lexer.ll src/scriptix/grammar.hh
	@PATH_FLEX@ -osrc/scriptix/lexer.cc src/scriptix/lexer.ll

awemud_ctrl_SOURCES=\
	src/util/awemud-ctrl.c

awemud_bin_SOURCES=\
	$(COMMON_SOURCES) \
	$(SCRIPTIX_SOURCES) \
	src/mud/account.cc \
	src/mud/action.cc \
	src/mud/ai.cc \
	src/mud/bindings.cc \
	src/mud/body.cc \
	src/mud/caffect.cc \
	src/mud/calendar.cc \
	src/mud/char.cc \
	src/mud/char_do.cc \
	src/mud/cmd_admin.cc \
	src/mud/cmd_builder.cc \
	src/mud/cmd_char.cc \
	src/mud/cmd_gm.cc \
	src/mud/cmd_init.cc \
	src/mud/cmd_player.cc \
	src/mud/combat.cc \
	src/mud/command.cc \
	src/mud/control.cc \
	src/mud/efactory.cc \
	src/mud/entity.cc \
	src/mud/error.cc \
	src/mud/event.cc \
	src/mud/eventids.cc \
	src/mud/exit.cc \
	src/mud/fileobj.cc \
	src/mud/help.cc \
	src/mud/hooks.cc \
	src/mud/idmap.cc \
	src/mud/imanager.cc \
	src/mud/log.cc \
	src/mud/login.cc \
	src/mud/mail.cc \
	src/mud/main.cc \
	src/mud/message.cc \
	src/mud/name.cc \
	src/mud/network.cc \
	src/mud/npc.cc \
	src/mud/object.cc \
	src/mud/olc.cc \
	src/mud/parse.cc \
	src/mud/pdesc.cc \
	src/mud/player.cc \
	src/mud/pmanager.cc \
	src/mud/race.cc \
	src/mud/room.cc \
	src/mud/settings.cc \
	src/mud/skill.cc \
	src/mud/social.cc \
	src/mud/streams.cc \
	src/mud/parse_commands.cc \
	src/mud/telnet.cc \
	src/mud/time.cc \
	src/mud/uniqid.cc \
	src/mud/weather.cc \
	src/mud/zmp.cc \
	src/mud/zone.cc

src/mud/bindings.cc: tools/script-code.xsl src/xml/script-intr.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/script-code.xsl' '$(top_srcdir)/src/xml/script-intr.xml' > src/mud/bindings.cc

src/mud/parse_commands.cc: tools/parse-code.xsl src/xml/parse-intr.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/parse-code.xsl' '$(top_srcdir)/src/xml/parse-intr.xml' > src/mud/parse_commands.cc

src/mud/eventids.cc: tools/events-code.xsl src/xml/event-list.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/events-code.xsl' '$(top_srcdir)/src/xml/event-list.xml' > src/mud/eventids.cc

src/mud/hooks.cc: tools/hooks-code.xsl src/xml/hook-list.xml
	$(PATH_XSLTPROC) '$(top_srcdir)/tools/hooks-code.xsl' '$(top_srcdir)/src/xml/hook-list.xml' > src/mud/hooks.cc

EXTRA_DIST += src/mud/bindings.cc src/mud/parse_commands.cc src/mud/eventids.cc src/mud/hooks.cc
CLEANFILES += src/mud/bindings.cc src/mud/parse_commands.cc src/mud/eventids.cc src/mud/hooks.cc
BUILT_SOURCES += src/mud/bindings.cc src/mud/parse_commands.cc src/mud/eventids.cc src/mud/hooks.cc
