/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/string.h"
#include "common/streams.h"
#include "mud/name.h"
#include "mud/entity.h"

std::string EntityArticleClass::names[] = {
	"normal",
	"proper",
	"unique",
	"plural",
	"vowel"
};

EntityArticleClass
EntityArticleClass::lookup (const std::string& text)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(text, names[i]))
			return i;
	return NORMAL;
}

std::string
EntityName::get_name () const
{
	switch (article.get_value()) {
		case EntityArticleClass::NORMAL:
			return "a " + text;
			break;
		case EntityArticleClass::PROPER:
			return text;
			break;
		case EntityArticleClass::UNIQUE:
			return "the " + text;
			break;
		case EntityArticleClass::PLURAL:
			return "some " + text;
			break;
		case EntityArticleClass::VOWEL:
			return "an " + text;
			break;
		default:
			return text;
			break;
	}
}

bool
EntityName::set_name (const std::string& s_text)
{
	// empty text?  no article, no text
	if (s_text.empty()) {
		article = EntityArticleClass::NORMAL;
		text = s_text;
		return false;
	// start with capital letter?
	} else if (isupper(s_text[0])) {
		article = EntityArticleClass::PROPER;
		text = s_text;
		return true;
	// start with the article 'the'?
	} else if (!strncasecmp("the ", s_text.c_str(), 4)) {
		article = EntityArticleClass::UNIQUE;
		text = std::string(s_text.c_str() + 4);
		return true;
	// start with the article 'some'?
	} else if (!strncasecmp("some ", s_text.c_str(), 5)) {
		article = EntityArticleClass::PLURAL;
		text = std::string(s_text.c_str() + 5);
		return true;
	// start with the article 'an'?
	} else if (!strncasecmp("an ", s_text.c_str(), 3)) {
		article = EntityArticleClass::VOWEL;
		text = std::string(s_text.c_str() + 3);
		return true;
	// start with the article 'a'?
	} else if (!strncasecmp("a ", s_text.c_str(), 2)) {
		article = EntityArticleClass::NORMAL;
		text = std::string(s_text.c_str() + 2);
		return true;
	// no known article or rule... time to guess
	} else {
		text = s_text;
		if (s_text[s_text.size() - 1] == 's') {
			article = EntityArticleClass::PLURAL;
		} else if (toupper(s_text[0]) == 'a' ||
				toupper(s_text[0]) == 'e' ||
				toupper(s_text[0]) == 'i' ||
				toupper(s_text[0]) == 'o' ||
				toupper(s_text[0]) == 'u') {
			article = EntityArticleClass::VOWEL;
		} else {
			article = EntityArticleClass::NORMAL;
		}
		return false;
	}
}

bool
EntityName::matches (const std::string& match) const
{
	return phrase_match(get_text(), match);
}

const StreamControl&
operator << (const StreamControl& stream, const StreamName& name)
{
	std::string text = name.ref.get_name().get_text();
	EntityArticleClass article = name.ref.get_name().get_article();

	// PROPER NAMES (SPECIAL ARTICLES)
	
	// proper names, no articles
	if (article == EntityArticleClass::PROPER || name.article == NONE) {
		// specialize output
		stream << name.ref.ncolor();
		if (name.capitalize && !text.empty()) {
			stream << (char)toupper(text[0]) << text.c_str() + 1;
		} else {
			stream << text;
		}
		stream << CNORMAL;
		return stream;
	// definite articles - uniques
	} else if (name.article == DEFINITE || article == EntityArticleClass::UNIQUE) {
		if (name.capitalize)
			stream << "The ";
		else
			stream << "the ";
	
	// POSSESSIVE ARTICLES
	} else if (name.article == YOUR) {
		if (name.capitalize)
			stream << "Your ";
		else
			stream << "your ";
	} else if (name.article == MY) {
		if (name.capitalize)
			stream << "My ";
		else
			stream << "my ";
	} else if (name.article == OUR) {
		if (name.capitalize)
			stream << "Our ";
		else
			stream << "our ";
	} else if (name.article == HIS) {
		if (name.capitalize)
			stream << "His ";
		else
			stream << "his ";
	} else if (name.article == HER) {
		if (name.capitalize)
			stream << "Her ";
		else
			stream << "her ";
	} else if (name.article == ITS) {
		if (name.capitalize)
			stream << "Its ";
		else
			stream << "its ";
	} else if (name.article == THEIR) {
		if (name.capitalize)
			stream << "Their ";
		else
			stream << "their ";
	
	// REGULAR NOUNS ARTICLES
	
	// pluralized name
	} else if (article == EntityArticleClass::PLURAL) {
		if (name.capitalize)
			stream << "Some ";
		else
			stream << "some ";
	// starts with a vowel-sound
	} else if (article == EntityArticleClass::VOWEL) {
		if (name.capitalize)
			stream << "An ";
		else
			stream << "an ";
	// normal-type name, nifty.
	} else {
		if (name.capitalize)
			stream << "A ";
		else
			stream << "a ";
	}

	// actually pump out the name
	stream << name.ref.ncolor() << text << CNORMAL;

	return stream;
}
