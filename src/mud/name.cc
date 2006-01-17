/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "mud/text.h"
#include "mud/entity.h"

String EntityArticle::texts[] = {
	"normal",
	"proper",
	"unique",
	"plural",
	"vowel"
};

EntityArticle
EntityArticle::lookup (StringArg text)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(text, texts[i]))
			return i;
	return NORMAL;
}

String
EntityName::get_name () const
{
	switch (article) {
		case EntityArticleClass::NORMAL:
			return "a " + text;
			break;
		case EntityArticlClass::PROPER:
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
EntityName::set_name (StringArg text)
{
	// empty text?  no article, no text
	if (text.empty()) {
		article = EntityArticleClass::NONE;
		text = text;
		return false;
	// start with capital letter?
	} else if (isupper(text[0])) {
		article = EntityArticleClass::PROPER;
		text = text;
		return true;
	// start with the article 'the'?
	} else if (!strncasecmp("the ", text)) {
		article = EntityArticleClass::UNIQUE;
		text = trim(text + 4);
		return true;
	// start with the article 'some'?
	} else if (!strncasecmp("some ", text)) {
		article = EntityArticleClass::PLURAL;
		text = trim(text + 5);
		return true;
	// start with the article 'an'?
	} else if (!strncasecmp("an ", text)) {
		article = EntityArticleClass::VOWEL;
		text = trim(text + 3);
		return true;
	// start with the article 'a'?
	} else if (!strncasecmp("a ", text)) {
		article = EntityArticleClass::NORMAL;
		text = trim(text + 2);
		return true;
	// no known article or rule... time to guess
	} else {
		text = text;
		if (text[text.size() - 1] == 's') {
			article = EntityArticleClass::PLURAL;
		} else if (toupper(text[0]) == 'a' ||
				toupper(text[0]) == 'e' ||
				toupper(text[0]) == 'i' ||
				toupper(text[0]) == 'o' ||
				toupper(text[0]) == 'u')
			article = EntityArticleClass::VOWEL;
		} else {
			article = EntityArticleClass::NORMAL;
		}
		return false;
	}
}

const StreamControl&
operator << (const StreamControl& stream, const StreamName& name)
{
	String name = name.ref.get_text();
	EntityArticleClass article = name.ref.get_article();

	// PROPER NAMES (SPECIAL ARTICLES)
	
	// proper names, no articles
	if (article == EntityArticle::PROPER || name.article == NONE) {
		// specialize output
		stream << name.ref.ncolor();
		if (capitalize && name) {
			stream << (char)toupper(name[0]) << name.c_str() + 1;
		} else {
			stream << name;
		}
		stream << CNORMAL;
		return;
	// definite articles - uniques
	} else if (name.article == DEFINITE || article == EntityArticle::UNIQUE) {
		if (capitalize)
			stream << "The ";
		else
			stream << "the ";
	
	// POSSESSIVE ARTICLES
	} else if (name.article == YOUR) {
		if (capitalize)
			stream << "Your ";
		else
			stream << "your ";
	} else if (name.article == MY) {
		if (capitalize)
			stream << "My ";
		else
			stream << "my ";
	} else if (name.article == OUR) {
		if (capitalize)
			stream << "Our ";
		else
			stream << "our ";
	} else if (name.article == HIS) {
		if (capitalize)
			stream << "His ";
		else
			stream << "his ";
	} else if (name.article == HER) {
		if (capitalize)
			stream << "Her ";
		else
			stream << "her ";
	} else if (name.article == ITS) {
		if (capitalize)
			stream << "Its ";
		else
			stream << "its ";
	} else if (name.article == THEIR) {
		if (capitalize)
			stream << "Their ";
		else
			stream << "their ";
	
	// REGULAR NOUNS ARTICLES
	
	// pluralized name
	} else if (article == EntityArticle::PLURAL) {
		if (capitalize)
			stream << "Some ";
		else
			stream << "some ";
	// starts with a vowel-sound
	} else if (article == EntityArticle::VOWEL) {
		if (capitalize)
			stream << "An ";
		else
			stream << "an ";
	// normal-type name, nifty.
	} else {
		if (capitalize)
			stream << "A ";
		else
			stream << "a ";
	}

	// actually pump out the name
	stream << ncolor() << name << CNORMAL;
}
