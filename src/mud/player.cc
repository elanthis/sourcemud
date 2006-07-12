/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <fnmatch.h>
#include <time.h>
#include <sys/stat.h>

#include "mud/char.h"
#include "mud/player.h"
#include "common/string.h"
#include "mud/body.h"
#include "mud/settings.h"
#include "mud/room.h"
#include "mud/parse.h"
#include "common/streams.h"
#include "mud/color.h"
#include "mud/zone.h"
#include "mud/message.h"
#include "mud/telnet.h"
#include "mud/account.h"
#include "common/log.h"
#include "mud/zmp.h"
#include "mud/eventids.h"
#include "mud/race.h"
#include "mud/caffect.h"
#include "mud/object.h"
#include "mud/hooks.h"

// manager of players
SPlayerManager PlayerManager;

namespace {
	// make a roman-numeral - FIXME: put somewhere better, make more correct
	const char* get_roman (int n)
	{
		static char b[128];
		char* c = b;

		// zero or negative?
		if (n <= 0) {
			strcpy(b, "-");
			return b;
		}

		// max five hundred
		if (n > 500)
			n = 500;

		// hundreds
		int h = n / 100;
		n %= 100;
		while (h-- > 0)
			*(c++) = 'C';

		// ninty
		if (n / 10 == 9) {
			n -= 90;
			*(c++) = 'X';
			*(c++) = 'C';
		}

		// fifties
		int f = n / 50;
		n %= 50;
		while (f-- > 0)
			*(c++) = 'L';

		// fourty 
		if (n / 10 == 4) {
			n -= 40;
			*(c++) = 'X';
			*(c++) = 'L';
		}

		// tens
		int t = n / 10;
		n %= 10;
		while (t-- > 0)
			*(c++) = 'X';

		*c = 0;

		// nine
		if (n == 9)
			strcpy(c, "IX");
		else if (n == 8)
			strcpy(c, "IIX");
		else if (n == 7)
			strcpy(c, "VII");
		else if (n == 6)
			strcpy(c, "VI");
		else if (n == 5)
			strcpy(c, "V");
		else if (n == 4)
			strcpy(c, "IV");
		else if (n == 3)
			strcpy(c, "III");
		else if (n == 2)
			strcpy(c, "II");
		else if (n == 1)
			strcpy(c, "I");

		return b;
	}
}

SCRIPT_TYPE(Player);
Player::Player (class Account* s_account, StringArg s_id) : Character (AweMUD_PlayerType), birthday()
{
	// initialize
	account = s_account;

	conn = NULL;

	pdesc.height = 68;

	ninfo.last_rt = 0;
	ninfo.last_max_rt = 0;
	ninfo.last_hp = 0;
	ninfo.last_max_hp = 0;

	race = NULL;

	for (int i = 0; i < NUM_EXPS; ++ i) {
		exp[i] = 0;
	}

	// set name
	name.set_text(s_id);
	name.set_article(EntityArticleClass::PROPER);

	// register
	PlayerManager.player_list.push_back(this);
}

Player::~Player (void)
{
	// remove
	SPlayerManager::PlayerList::iterator i = std::find(PlayerManager.player_list.begin(), PlayerManager.player_list.end(), this);
	if (i != PlayerManager.player_list.end())
		PlayerManager.player_list.erase(i);
}
	
void
Player::save (File::Writer& writer)
{
	Character::save(writer);

	if (race != NULL)
		writer.attr(S("race"), race->get_name());

	writer.attr(S("birthday"), birthday.encode());

	writer.attr(S("alignment"), alignment);

	for (int i = 0; i < CharStatID::COUNT; ++i)
		writer.keyed(S("stat"), CharStatID(i).get_name(), base_stats[i]);
	
	for (TraitMap::const_iterator i = pdesc.traits.begin(); i != pdesc.traits.end(); ++i)
		writer.keyed(S("trait"), CharacterTraitID::nameof(i->first), i->second.get_name());

	writer.attr(S("gender"), pdesc.gender.get_name());
	writer.attr(S("height"), pdesc.height);

	if (get_room()) 
		writer.attr(S("location"), get_room()->get_id());

	writer.attr(S("general_xp"), exp[EXP_GENERAL]);
	writer.attr(S("warrior_xp"), exp[EXP_WARRIOR]);
	writer.attr(S("rogue_xp"), exp[EXP_ROGUE]);
	writer.attr(S("caster_xp"), exp[EXP_CASTER]);

	for (SSkillManager::SkillList::const_iterator i = SkillManager.get_skills().begin(); i != SkillManager.get_skills().end(); ++i)
		writer.keyed(S("skill"), (*i)->get_name(), skills.get_skill((*i)->get_id()));
}

void
Player::save (void)
{
	String path = PlayerManager.path(get_id());

	// backup player file
	if (SettingsManager.get_backup_players()) {
		// only if it exists
		struct stat st;
		if (!stat(path, &st)) {
			time_t base_t;
			time (&base_t);
			char time_buffer[15];
			strftime (time_buffer, sizeof (time_buffer), "%Y%m%d%H%M%S", localtime (&base_t));
			String backup = path + S(".") + String(time_buffer) + S("~");
			if (rename (path, backup)) // move file
				Log::Error << "Backup of " << path << " to " << backup << " failed: " << strerror(errno);
		}
	}

	// do save
	mode_t omask = umask(0066);
	File::Writer writer(path);
	writer.comment(String("Player file: ") + get_id());
	time_t t;
	time(&t);
	writer.comment(String("Timestamp: ") + String(ctime(&t)));
	writer.bl();
	save(writer);
	writer.close();
	umask(omask);

	// log
	Log::Info << "Saved player " << get_id();

	return;
}

void
Player::save_hook (ScriptRestrictedWriter* writer)
{
	Character::save_hook(writer);
	Hooks::save_player(this, writer);
}

int
Player::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_PARENT(Character)
		// our primary name
		FO_ATTR("name")
			name.set_text(node.get_data());
			name.set_article(EntityArticleClass::PROPER);
		// description
		FO_ATTR("desc")
			set_desc(node.get_data());
		FO_ATTR("gender")
			set_gender(GenderType::lookup(node.get_data()));
		FO_ATTR("alignment")
			FO_TYPE_ASSERT(INT);
			set_alignment(tolong(node.get_data()));
		FO_ATTR("race")
			race = RaceManager.get (node.get_data());
			if (race == NULL) {
				Log::Error << "Player has invalid race '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
				return -1;
			}
		FO_ATTR("birthday")
			if (birthday.decode(node.get_data()))
				throw File::Error (S("Invalid birthday"));
		FO_KEYED("trait")
			FO_TYPE_ASSERT(STRING)
			CharacterTraitID trait = CharacterTraitID::lookup(node.get_key());
			if (!trait.valid())
				throw File::Error (S("Unknown trait"));
			CharacterTraitValue value = CharacterTraitManager.get_trait(node.get_data());
			if (!value.valid())
				throw File::Error (S("Unknown trait value"));
			pdesc.traits[trait] = value;
		FO_ATTR("height")
			FO_TYPE_ASSERT(INT);
			pdesc.height = tolong(node.get_data());
		FO_ATTR("location")
			location = ZoneManager.get_room(node.get_data());
			if (location == NULL)
				Log::Error << "Unknown room '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_ATTR("general_xp")
			exp[EXP_GENERAL] = node.get_int();
		FO_ATTR("warrior_xp")
			exp[EXP_WARRIOR] = node.get_int();
		FO_ATTR("rogue_xp")
			exp[EXP_ROGUE] = node.get_int();
		FO_ATTR("caster_xp")
			exp[EXP_CASTER] = node.get_int();
		FO_KEYED("stat")
			FO_TYPE_ASSERT(INT);
			CharStatID stat = CharStatID::lookup(node.get_key());
			if (stat) {
				base_stats[stat.get_value()] = node.get_int();
			} else {
				Log::Error << "Unknown stat '" << node.get_key() << "'";
				throw File::Error(S("invalid value"));
			}
		FO_KEYED("skill")
			FO_TYPE_ASSERT(INT);
			SkillInfo* info = SkillManager.get_by_name(node.get_key());
			if (info != NULL) {
				skills.set_skill(info->get_id(), node.get_int());
			} else {
				Log::Error << "Unknown skill '" << node.get_key() << "'";
				throw File::Error(S("invalid value"));
			}
	FO_NODE_END
}

// 'startup' the player session
int
Player::start (void)
{
	// login message
	clear_scr();
	*this << "\n" << StreamParse (MessageManager.get(S("login")), S("user"), this) << "\n";

	// not already active?  add to room...
	if (!is_active()) {
		if (location) {
			// announce arrival
			ZoneManager.announce (CADMIN "**" CNORMAL " " CPLAYER + get_id() + CNORMAL " has entered this world, leaving behind " + get_gender().get_hisher() + " mundane life. " CADMIN "**" CNORMAL);

			// try to enter room
			if (!enter(location, NULL)) {
				*this << CADMIN "Internal error: could not enter room" CNORMAL;
				Log::Error << "Player '" << get_id() << "' could not enter '" << location->get_id() << "' at login";
				return -1;
			}
		} else {
			// location/room doesn't exist - go back to 'starting' location
			if (Hooks::player_start(this) == 0) {
				*this << CADMIN "Internal error: no player_start hook" CNORMAL;
				Log::Error << "Player '" << get_id() << "' could not login because there is no player_start hook";
				return -1;
			}

			// no valid location - eek!
			if (!location) {
				*this << CADMIN "Internal error: no start location given" CNORMAL;
				Log::Error << "Player '" << get_id() << "' could not login because the player_start hook did not assign a start location";
				return -1;
			}

			// announce login
			ZoneManager.announce (CADMIN "**" CNORMAL " " CPLAYER + get_id() + CNORMAL " has entered this world, leaving behind " + get_gender().get_hisher() + " mundane life. " CADMIN "**" CNORMAL);
		}

		// Example affect - make strong
		CharacterAffectGroup* strong = new CharacterAffectGroup(S("Strength"), CharacterAffectType::INNATE, 60 * 30);
		strong->add_affect(new CharacterAffectStat(CharStatID::STRENGTH, 10));
		add_affect(strong);
	// already active... just "refresh" room
	} else {
		do_look();
	}

	// no timeout - yet
	ninfo.timeout_ticks = 0;

	return 0;
}

// quit from game
void
Player::quit (void)
{
	// save the player
	save();

	// quit message
	ZoneManager.announce (CADMIN "**" CNORMAL " " CPLAYER + get_id() + CNORMAL " has left this world, returning to " + get_gender().get_hisher() + " mundane life. " CADMIN "**" CNORMAL);

	// disengage from game world
	destroy();

	// disconnect
	disconnect();
}

uint
Player::get_age (void) const
{
	// calculate the age in years, based on birthdate and current time
	uint years = TimeManager.time.get_year() - birthday.get_year();
	if (TimeManager.time.get_month() < birthday.get_month())
		years --;
	else if (TimeManager.time.get_month() == birthday.get_month())
		if (TimeManager.time.get_day() < birthday.get_day())
			years --;
	return years;
}

void
Player::kill (Character *killer)
{
	// death message
	if (get_room())
		*get_room() << StreamIgnore(this) << StreamName(this, DEFINITE, true) << " has been slain!\n";
	*this << "You have been slain!\n";

	// now laying down
	position = CharPosition::LAY;

	// event/hook
	Events::send_death(get_room(), this, killer);
	Hooks::player_death(this, killer);
}

void
Player::display_inventory (void)
{
	// start - worn
	*this << "You are wearing ";

	// inventory variables
	uint loc = 0;
	Object* obj;
	Object* last = NULL;
	bool didshow = false;

	// worn items
	while ((obj = get_worn_at(loc++)) != NULL) {
		// we had one already?
		if (last) {
			// prefix
			if (didshow)
				*this << ", ";
			didshow = true;
			// do show
			*this << StreamName(last, INDEFINITE);
		}
		// remember this object
		last = obj;
	}
	// spit out the left over
	if (last) {
		// prefix
		if (didshow)
			*this << " and ";
		// show it
		*this << StreamName(last, INDEFINITE);
	} else {
		*this << "nothing";
	}

	// start - helf
	*this << ".  You are holding ";

	// held items
	loc = 0;
	didshow = false;
	last = NULL;
	while ((obj = get_held_at(loc++)) != NULL) {
		// we had one already?
		if (last) {
			// prefix
			if (didshow)
				*this << ", ";
			didshow = true;
			// show
			*this << StreamName(last, INDEFINITE);
		}
		last = obj;
	}
	// show the last one
	if (last) {
		// prefix
		if (didshow)
			*this << " and ";
		// show it
		*this << StreamName(last, INDEFINITE);
	} else {
		*this << "nothing";
	}

	// coins
	*this << ".  You have " << coins << " coins.\n";
}

void
Player::display_skills (void)
{
	*this << "Skills:\n";

	set_indent(2);

	for (SSkillManager::SkillList::const_iterator i = SkillManager.get_skills().begin(); i != SkillManager.get_skills().end(); ++i) {
		int ranks = skills.get_skill((*i)->get_id());
		if (!(*i)->is_secret() && (!(*i)->is_restricted() || ranks > 0))
			*this << (*i)->get_name() << " " << get_roman(ranks) << "\n";
	}

	set_indent(0);
}

void
Player::grant_exp (uint type, uint amount) {
	assert ( type < NUM_EXPS );
	if (amount == 0)
		return;

	// figure out the general exp to grant - 25%
	uint general = amount / 4;
	amount -= general;

	// increase the specified and general pool accordingly
	exp[EXP_GENERAL] += general;
	exp[type] += amount;
}

void
Player::recalc_stats (void)
{
	Character::recalc_stats();

	// apply racial stat modifications
	if (race) {
		for (int i = 0; i < CharStatID::COUNT; ++i)
			set_effective_stat(i, get_base_stat(i) + race->get_stat(i));
	}
}

void
Player::recalc (void)
{
	Character::recalc();
}

void
Player::heartbeat(void) {
	// do character update
	Character::heartbeat();

	// timeout?  then die
	if (ninfo.timeout_ticks == 1) {
		Log::Info << "Player '" << get_id() << "' has timed out.";
		quit();
	} else if (ninfo.timeout_ticks > 0) {
		--ninfo.timeout_ticks;
	}

	// STATUS UPDATES

	// health
	if (get_hp() != ninfo.last_hp || get_max_hp() != ninfo.last_max_hp) {
		if (get_telnet()) {
			if (get_telnet()->has_zmp_net_awemud()) {
				ZMPPack zmp(S("net.awemud.status.set"));
				zmp.add(S("hp"));
				zmp.add(ninfo.last_hp);
				zmp.add(ninfo.last_max_hp);
				zmp.send(get_telnet());
			} else if (get_hp() >= get_max_hp()) {
				get_telnet()->force_update();
			}
		}
	}

	// store
	ninfo.last_hp = get_hp();
	ninfo.last_max_hp = get_max_hp();

	// round time
	uint rts = get_round_time();
	if (rts != ninfo.last_rt) {

		if (get_telnet()) {
			if (get_telnet()->has_zmp_net_awemud()) {
				// send zmp
				ZMPPack rt(S("net.awemud.status.set"));
				rt.add(S("rt"));
				rt.add(ninfo.last_rt);
				rt.add(ninfo.last_max_rt);
				rt.send(get_telnet());
			} else if (rts == 0) {
				get_telnet()->force_update();
			}
		}
	}

	// store rt
	ninfo.last_rt = rts;

	// max rt is the highest rt found, reset at 0
	if (ninfo.last_rt > ninfo.last_max_rt)
		ninfo.last_max_rt = ninfo.last_rt;
	else if (ninfo.last_rt == 0)
		ninfo.last_max_rt = 0;

	// update handler
	Hooks::player_heartbeat(this);
}

void
Player::activate (void)
{
	Character::activate();

	if (account != NULL)
		account->inc_active();
}

void
Player::deactivate (void)
{
	if (account != NULL)
			account->dec_active();

	Character::deactivate();
}

void
Player::show_prompt (void)
{
	// no telnet handler?  bail
	if (!get_telnet())
		return;

	// net.awemud around?  just show >
	if (get_telnet()->has_zmp_net_awemud()) {
		*this << ">";
	// do the full/stock prompt
	} else {
		char prompt[128];
		snprintf (prompt, 128, "-- HP:%d/%d RT:%u >", get_hp (), get_max_hp (), get_round_time ());
		*this << prompt;
	}
}

int
Player::parse_property (const StreamControl& stream, StringArg comm, const ParseArgs& argv) const
{
	// RACE
	if (str_eq(comm, S("race"))) {
		if (get_race())
			stream << get_race()->get_name();
		return 0;
	}
	// RACE ADJECTIVE
	else if (str_eq(comm, S("race-adj"))) {
		if (get_race())
			stream << get_race()->get_adj();
		return 0;
	}
	// TRAIT NAME
	else if (str_eq(comm, S("trait-name")) && argv.size() == 1) {
		stream << get_trait(CharacterTraitID::lookup(argv[0].get_string())).get_name();
		return 0;
	}
	// TRAIT DESC
	else if (str_eq(comm, S("trait")) && argv.size() == 1) {
		stream << get_trait(CharacterTraitID::lookup(argv[0].get_string())).get_desc();
		return 0;
	}
	// HEIGHT
	else if (str_eq(comm, S("height"))) {
		int feet = get_height() / 12;
		if (feet > 0)
			stream << feet << '\'';
		stream << get_height() % 12 << '"';
		return 0;
	}
	// HEIGHT (RAW)
	else if (str_eq(comm, S("raw-height"))) {
		stream << get_height();
		return 0;
	}
	// default...
	else {
		return Character::parse_property(stream, comm, argv);
	}
}

// connect to a telnet handler
void
Player::connect (TelnetHandler* handler)
{
	// can't be same connection
	assert(handler != conn);

	// had a previous connection?  kill it
	if (conn) {
		Log::Info << "Booting previous connection for player '" << get_id() << "'";
		*this << CADMIN << "Connection replaced by second login.\n" CNORMAL;
		disconnect();
	}

	// set connection
	conn = handler;
	
	// reset all network info
	memset(&ninfo, 0, sizeof(ninfo));
}

// disconnect from a telnet handler
void
Player::disconnect (void)
{
	// already disconnected?
	if (!get_telnet())
		return;

	TelnetHandler* telnet = get_telnet();
	
	// remove telnet handler
	conn = NULL;

	// end the current telnet mode
	telnet->finish();

	// begin timeout
	ninfo.timeout_ticks = ROUNDS_TO_TICKS(60); // 60 second timeout
}

// output text
void
Player::stream_put (const char* data, size_t len)
{
	if (get_telnet())
		get_telnet()->stream_put(data, len);
}

// toggle echo
void
Player::toggle_echo (bool value)
{
	if (get_telnet())
		get_telnet()->toggle_echo(value);
}

// set indent
void
Player::set_indent (uint level)
{
	if (get_telnet())
		get_telnet()->set_indent(level);
}

// get width of view
uint
Player::get_width (void)
{
	if (get_telnet())
		return get_telnet()->get_width();
	else
		return TELNET_DEFAULT_WIDTH;
}

// clear screen
void
Player::clear_scr (void)
{
	if (get_telnet())
		get_telnet()->clear_scr();
}

// show player description
void
Player::display_desc (const StreamControl& stream) const
{
	stream << StreamParse(get_race()->get_desc(), S("player"), this);
}

// get player trait
CharacterTraitValue
Player::get_trait (CharacterTraitID id) const
{
	TraitMap::const_iterator i = pdesc.traits.find(id);
	if (i != pdesc.traits.end())
		return i->second;
	return CharacterTraitValue();
}

// set player trait
void
Player::set_trait (CharacterTraitID id, CharacterTraitValue value)
{
	pdesc.traits[id] = value;
}
