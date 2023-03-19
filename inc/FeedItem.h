/*
 * Copyright (c) 2016 Evgenii Dobrovidov
 * This file is part of "RSS aggregator".
 *
 * "RSS aggregator" is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * "RSS aggregator" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with "RSS aggregator".  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef FEEDITEM_H_
#define FEEDITEM_H_

#include "Utilities.h"
#include <FXml.h>

using namespace Osp::Base::Collection;
using namespace Osp::Xml;

class FeedItem: public Object {
public:
	FeedItem(const String &title = String(L""), const String &content = String(L""));
	FeedItem(const FeedItem &other);
	virtual ~FeedItem(void) { }

	int GetID(void) const {
		return __id;
	}

	bool IsRead(void) const {
		return __read;
	}

	String GetTitle(void) const {
		return __title;
	}
	void SetTitle(const String &str) {
		__title = str;
	}

	String GetContent(void) const {
		return __content;
	}
	void SetContent(const String &str) {
		__content = str;
	}

	String GetGuidLink(void) const {
		return __guidLink;
	}
	void SetGuidLink(const String &str) {
		__guidLink = str;
	}

	long long GetPubDate(void) const {
		return __pubDate;
	}
	void SetPubDate(long long dt) {
		__pubDate = dt;
	}

	bool IsEnclosureDownloaded(void) const {
		return __enclosureDownloaded;
	}
	void SetEnclosure(const String &url, const String &mime, int len = 0);

	String GetEnclosureURL(void) const {
		return __enclosureURL;
	}
	String GetEnclosureMIME(void) const {
		return __enclosureMIME;
	}
	int GetEnclosureLength(void) const {
		return __enclosureLength;
	}

	String GetAllStrings(void) const;
	String GetFilterStrings(void) const;
	String GetUniqueString(void) const;

	void SetDataFromXML(const xmlNode &item_node, GregorianCalendar *calendar);

	virtual bool Equals(const Object& obj) const;

private:
	void SetID(int id) {
		__id = id;
	}
	void SetRead(bool val) {
		__read = val;
	}
	void SetEnclosureDownloaded(bool val) {
		__enclosureDownloaded = val;
	}

	int __id;
	bool __read;

	String __title;
	String __content;
	String __guidLink;
	long long __pubDate;

	bool __enclosureDownloaded;
	String __enclosureURL;
	String __enclosureMIME;
	int __enclosureLength;

	friend class FeedManager;
	friend class FeedItemCollection;
};

#endif
