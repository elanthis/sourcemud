/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "common/streams.h"
#include "common/log.h"
#include "common/file.h"
#include "mud/creature.h"
#include "mud/player.h"
#include "mud/body.h"
#include "mud/settings.h"
#include "mud/room.h"
#include "mud/macro.h"
#include "mud/color.h"
#include "mud/zone.h"
#include "mud/message.h"
#include "mud/account.h"
#include "mud/race.h"
#include "mud/caffect.h"
#include "mud/object.h"
#include "mud/hooks.h"
#include "mud/efactory.h"
#include "net/telnet.h"

// manager of players
_MPlayer MPlayer;

namespace
{
	// make a roman-numeral - FIXME: put somewhere better, make more correct
	const char* getRoman(int n)
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

Player::Player(std::tr1::shared_ptr<class Account> s_account, const std::string& s_id)
{
	// initialize
	account = s_account;

	conn = NULL;

	ninfo.last_rt = 0;
	ninfo.last_max_rt = 0;
	ninfo.last_hp = 0;
	ninfo.last_max_hp = 0;

	race = NULL;

	experience = 0;

	time_created = time(NULL);
	time_lastlogin = (time_t)0;
	total_playtime = 0;

	// set name
	name.setText(s_id);
	name.setArticle(EntityArticleClass::PROPER);

	// register
	MPlayer.player_list.push_back(this);
}

Player::~Player()
{
	// remove
	_MPlayer::PlayerList::iterator i = std::find(MPlayer.player_list.begin(), MPlayer.player_list.end(), this);
	if (i != MPlayer.player_list.end())
		MPlayer.player_list.erase(i);
}

void Player::saveData(File::Writer& writer)
{
	Creature::saveData(writer);

	writer.attr("player", "created", timeToStr(time_created));
	writer.attr("player", "lastlogin", timeToStr(time_lastlogin));
	writer.attr("player", "playtime", total_playtime);

	if (race != NULL)
		writer.attr("player", "race", race->getName());

	writer.attr("player", "birthday", birthday.encode());

	for (int i = 0; i < CreatureStatID::COUNT; ++i) {
		std::vector<File::Value> list;
		list.push_back(File::Value(File::Value::TYPE_STRING, CreatureStatID(i).getName()));
		list.push_back(File::Value(File::Value::TYPE_INT, tostr(base_stats[i])));
		writer.attr("player", "stat", list);
	}

	writer.attr("player", "gender", form.gender.getName());
	writer.attr("player", "build", form.build.getName());
	writer.attr("player", "height", form.height.getName());
	writer.attr("player", "skin_color", form.skin_color.getName());
	writer.attr("player", "eye_color", form.eye_color.getName());
	writer.attr("player", "hair_color", form.hair_color.getName());
	writer.attr("player", "hair_style", form.hair_style.getName());

	if (getRoom())
		writer.attr("player", "location", getRoom()->getId());

	writer.attr("player", "experience", experience);

	for (size_t i = 1; i < SkillID::size(); ++i) {
		if (skills.hasSkill(SkillID(i))) {
			std::vector<File::Value> list;
			list.push_back(File::Value(File::Value::TYPE_STRING, SkillID(i).getName()));
			list.push_back(File::Value(File::Value::TYPE_INT, tostr(skills.getSkill(SkillID(i)))));
			writer.attr("player", "skill", list);
		}
	}
}

void Player::save()
{
	std::string path = MPlayer.path(getId());

	// backup player file
	if (MSettings.getBackupPlayers()) {
		// only if it exists
		struct stat st;
		if (!stat(path.c_str(), &st)) {
			time_t base_t;
			time(&base_t);
			char time_buffer[15];
			strftime(time_buffer, sizeof(time_buffer), "%Y%m%d%H%M%S", localtime(&base_t));
			std::string backup = path + "." + std::string(time_buffer) + "~";
			if (File::rename(path, backup)) // move file
				Log::Error << "Backup of " << path << " to " << backup << " failed: " << strerror(errno);
		}
	}

	// do save
	mode_t omask = umask(0066);
	File::Writer writer(path);
	writer.comment("Player file: " + getId());
	time_t t;
	time(&t);
	writer.comment("Timestamp: " + std::string(ctime(&t)));
	writer.bl();
	saveData(writer);
	writer.close();
	umask(omask);

	return;
}

void Player::saveHook(File::Writer& writer)
{
	Creature::saveHook(writer);
	Hooks::savePlayer(this, writer);
}

int Player::loadNode(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
	FO_PARENT(Creature)
	// our primary name
	FO_ATTR("player", "name")
	name.setText(node.getString());
	name.setArticle(EntityArticleClass::PROPER);
	// description
	FO_ATTR("player", "desc")
	setDesc(node.getString());
	FO_ATTR("player", "gender")
	setGender(GenderType::lookup(node.getString()));
	FO_ATTR("player", "build")
	form.build = FormBuild::lookup(node.getString());
	FO_ATTR("player", "height")
	form.height = FormHeight::lookup(node.getString());
	FO_ATTR("player", "skin_color")
	form.skin_color = FormColor::create(node.getString());
	FO_ATTR("player", "eye_color")
	form.eye_color = FormColor::create(node.getString());
	FO_ATTR("player", "hair_color")
	form.hair_color = FormColor::create(node.getString());
	FO_ATTR("player", "hair_style")
	form.hair_style = FormHairStyle::lookup(node.getString());
	FO_ATTR("player", "race")
	race = MRace.get(node.getString());
	if (race == NULL) {
		Log::Error << node << ": Player has invalid race";
		throw File::Error();
	}
	FO_ATTR("player", "birthday")
	if (birthday.decode(node.getString()))
		throw File::Error("Invalid birthday");
	FO_ATTR("player", "location")
	location = MZone.getRoom(node.getString());
	if (location == NULL) {
		Log::Error << node << ": Unknown room";
		throw File::Error();
	}
	FO_ATTR("player", "experience")
	experience = node.getInt();
	FO_ATTR("player", "stat")
	CreatureStatID stat = CreatureStatID::lookup(node.getString(0));
	if (stat) {
		base_stats[stat.getValue()] = node.getInt(1);
	} else {
		Log::Error << "Unknown stat '" << node.getString(0) << "'";
		return -1;
	}
	FO_ATTR("player", "skill")
	SkillID skill = SkillID::lookup(node.getString(0));
	if (skill) {
		skills.setSkill(skill, node.getInt(1));
	} else {
		Log::Error << node << ": Unknown skill: " << node.getString(0);
		throw File::Error();
	}
	FO_ATTR("player", "created")
	time_created = strToTime(node.getString());
	FO_ATTR("player", "lastlogin")
	time_lastlogin = strToTime(node.getString());
	FO_ATTR("player", "playtime")
	total_playtime = node.getInt();
	FO_NODE_END
}

// 'startup' the player session
int Player::startSession()
{
	// login message
	clearScr();
	*this << "\n" << StreamMacro(MMessage.get("login")).set("player", this) << "\n";

	// not already active?  add to room...
	if (!isActive()) {
		if (location) {
			// announce arrival
			MZone.announce(CADMIN "**" CNORMAL " " CPLAYER + getId() + CNORMAL " has entered this world, leaving behind " + getGender().getHisHer() + " mundane life. " CADMIN "**" CNORMAL);

			// try to enter room
			if (!enter(location, NULL)) {
				*this << CADMIN "Internal error: could not enter room" CNORMAL;
				Log::Error << "Player '" << getId() << "' could not enter '" << location->getId() << "' at login";
				return -1;
			}
		} else {
			// location/room doesn't exist - go back to 'starting' location
			if (Hooks::playerStart(this) == 0) {
				*this << CADMIN "Internal error: no player_start hook" CNORMAL;
				Log::Error << "Player '" << getId() << "' could not login because there is no player_start hook";
				return -1;
			}

			// no valid location - eek!
			if (!location) {
				*this << CADMIN "Internal error: no start location given" CNORMAL;
				Log::Error << "Player '" << getId() << "' could not login because the player_start hook did not assign a start location";
				return -1;
			}

			// announce login
			MZone.announce(CADMIN "**" CNORMAL " " CPLAYER + getId() + CNORMAL " has entered this world, leaving behind " + getGender().getHisHer() + " mundane life. " CADMIN "**" CNORMAL);
		}

		// Example affect - make strong
		CreatureAffectGroup* strong = new CreatureAffectGroup("Strength", CreatureAffectType::INNATE, 60 * 30);
		strong->addAffect(new CreatureAffectStat(CreatureStatID::STRENGTH, 10));
		addAffect(strong);

		// update login time
		time_lastlogin = time(NULL);
		// already active... just "refresh" room
	} else {
		doLook();
	}

	// no timeout - yet
	ninfo.timeout_ticks = 0;

	return 0;
}

void Player::endSession()
{
	// update playtime
	total_playtime += time(NULL) - time_lastlogin;

	// save the player
	save();

	// quit message
	MZone.announce(CADMIN "**" CNORMAL " " CPLAYER + getId() + CNORMAL " has left this world, returning to " + getGender().getHisHer() + " mundane life. " CADMIN "**" CNORMAL);

	// disengage from game world
	destroy();

	// disconnect
	disconnect();
}

uint Player::getAge() const
{
	// calculate the age in years, based on birthdate and current time
	uint years = MTime.time.getYear() - birthday.getYear();
	if (MTime.time.getMonth() < birthday.getMonth())
		years --;
	else if (MTime.time.getMonth() == birthday.getMonth())
		if (MTime.time.getDay() < birthday.getDay())
			years --;
	return years;
}

void Player::kill(Creature *killer)
{
	// death message
	if (getRoom())
		*getRoom() << StreamIgnore(this) << StreamName(this, DEFINITE, true) << " has been slain!\n";
	*this << "You have been slain!\n";

	// now laying down
	position = CreaturePosition::LAY;

	// FIXME EVENT
	Hooks::playerDeath(this, killer);
}

void Player::displayInventory()
{
	// start - worn
	*this << "You are wearing ";

	// inventory variables
	uint loc = 0;
	Object* obj;
	Object* last = NULL;
	bool didshow = false;

	// worn items
	while ((obj = getWornAt(loc++)) != NULL) {
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
	while ((obj = getHeldAt(loc++)) != NULL) {
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

void Player::displaySkills()
{
	*this << "Skills:\n";

	setIndent(2);

	for (size_t i = 1; i < SkillID::size(); ++i) {
		if (skills.hasSkill(SkillID(i)))
			*this << SkillID(i).getName() << " " << getRoman(skills.getSkill(SkillID(i))) << "\n";
	}

	setIndent(0);
}

void Player::grantExp(uint amount)
{
	experience += amount;
}

void Player::recalcStats()
{
	Creature::recalcStats();

	// apply racial stat modifications
	if (race) {
		for (int i = 0; i < CreatureStatID::COUNT; ++i)
			setEffectiveStat(i, getBaseStat(i) + race->getStat(i));
	}
}

void Player::recalc()
{
	Creature::recalc();
}

void Player::heartbeat()
{
	// do creature update
	Creature::heartbeat();

	// timeout?  then die
	if (ninfo.timeout_ticks == 1) {
		Log::Info << "Player '" << getId() << "' has timed out.";
		endSession();
	} else if (ninfo.timeout_ticks > 0) {
		--ninfo.timeout_ticks;
	}

	// STATUS UPDATES

	// force a prompt redraw if these change
	uint rts = getRoundTime();
	if (getHP() != ninfo.last_hp || getMaxHP() != ninfo.last_max_hp || rts != ninfo.last_rt)
		getConn()->pconnForcePrompt();

	// store values
	ninfo.last_hp = getHP();
	ninfo.last_max_hp = getMaxHP();
	ninfo.last_rt = rts;

	// max is either current max, or zero if rt is done
	if (rts > ninfo.last_max_rt)
		ninfo.last_max_rt = rts;
	else if (rts == 0)
		ninfo.last_max_rt = 0;

	// update handler
	Hooks::playerHeartbeat(this);
}

void Player::activate()
{
	Creature::activate();

	if (account != NULL)
		account->incActive();
}

void Player::deactivate()
{
	if (account != NULL)
		account->decActive();

	Creature::deactivate();
}

void Player::showPrompt()
{
	*this << "< HP:" << getHP() << "/" << getMaxHP() << " RT:" << getRoundTime() << " >";
}

int Player::macroProperty(const StreamControl& stream, const std::string& comm, const MacroList& argv) const
{
	// RACE
	if (strEq(comm, "race")) {
		if (getRace())
			stream << getRace()->getName();
		// RACE ADJECTIVE
	} else if (strEq(comm, "race-adj")) {
		if (getRace())
			stream << getRace()->getAdj();
		// PHYSICAL FORM
	} else if (strEq(comm, "build")) {
		stream << form.build.getName();
	} else if (strEq(comm, "skin_color")) {
		stream << form.skin_color.getName();
	} else if (strEq(comm, "eye_color")) {
		stream << form.eye_color.getName();
	} else if (strEq(comm, "hair_color")) {
		stream << form.hair_color.getName();
	} else if (strEq(comm, "hair_style")) {
		stream << form.hair_style.getName();
	} else if (strEq(comm, "height")) {
		stream << form.height.getName();
		// default...
	} else {
		return Creature::macroProperty(stream, comm, argv);
	}

	return 0;
}

// connect to a telnet handler
void Player::connect(IPlayerConnection* handler)
{
	// can't be same connection
	assert(handler != conn);

	// had a previous connection?  kill it
	if (conn) {
		Log::Info << "Booting previous connection for player '" << getId() << "'";
		*this << CADMIN << "Connection replaced by second login.\n" CNORMAL;
		disconnect();
	}

	// set connection
	conn = handler;
	handler->pconnConnect(this);

	// reset all network info
	memset(&ninfo, 0, sizeof(ninfo));
}

// disconnect from a telnet handler
void Player::disconnect()
{
	// already disconnected?
	if (!getConn())
		return;

	// tell connection handler we're disconnecting
	getConn()->pconnDisconnect();

	// no more connection handler
	conn = NULL;

	// begin timeout
	ninfo.timeout_ticks = ROUNDS_TO_TICKS(60); // 60 second timeout
}

// output text
void Player::streamPut(const char* data, size_t len)
{
	if (getConn())
		getConn()->pconnWrite(data, len);
}

// toggle echo
void Player::toggleEcho(bool value)
{
	if (getConn())
		getConn()->pconnSetEcho(value);
}

// set indent
void Player::setIndent(uint level)
{
	if (getConn())
		getConn()->pconnSetIndent(level);
}

// get width of view
uint Player::getWidth()
{
	if (getConn())
		return getConn()->pconnGetWidth();
	else
		return TELNET_DEFAULT_WIDTH;
}

// clear screen
void Player::clearScr()
{
	if (getConn())
		getConn()->pconnClear();
}

// show player description
void Player::displayDesc(const StreamControl& stream) const
{
	stream << StreamMacro(getRace()->getDesc()).set("self", this);
}
