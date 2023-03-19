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
#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "Utilities.h"
#include "FeedCollection.h"

class Channel: public Object {
public:
	Channel(const String &title = String(L""), const String &description = String(L""), const String &link = String(L""), const String &url = String(L""));
	Channel(const Channel &other);
	void SetFrom(const Channel &other);
	virtual ~Channel(void);

	int GetID(void) const {
		return __id;
	}
	bool IsIDSet(void) const {
		return __id != (int)0xFFFFFFFF;
	}

	String GetTitle(void) const {
		return __title;
	}
	void SetTitle(const String &str) {
		__title = str;
	}
	String GetDescription(void) const {
		return __description;
	}
	void SetDescription(const String &str) {
		__description = str;
	}
	String GetURL(void) const {
		return __url;
	}
	void SetURL(const String &str) {
		__url = str;
	}
	String GetLink(void) const {
		return __link;
	}
	void SetLink(const String &str) {
		__link = str;
	}

	const FeedItemCollection *GetItems(void) const {
		return __items;
	}
	void SetItems(FeedItemCollection *items) {
		__items = items;
	}

	long long GetPubDate(void) const {
		return __pubDate;
	}
	void SetPubDate(long long dt) {
		__pubDate = dt;
	}
	int GetTTL(void) const {
		return __ttl;
	}
	void SetTTL(int val) {
		__ttl = val > 180 ? 180 : val;
	}

	String GetFaviconPath(void) const {
		return __faviconPath;
	}
	void SetFaviconPath(const String &str) {
		__faviconPath = str;
	}

	Bitmap *GetFavicon(void) const;
	void SetFavicon(Bitmap *bmp) {
		__pFavicon = bmp;
	}

	String GetOPMLOutline(void) const;
	String GetAllStrings(void) const;
	String GetFilterStrings(void) const;
	String GetUniqueString(void) const;

	bool SetDataFromXML(const char *pData, int len);

	void RemoveObsoleteEnclosures(void);

	virtual bool Equals(const Object& obj) const;

private:
	void SetID(int id) {
		__id = id;
		if (__items) {
			__items->SetChannelID(id);
		}
	}

	int __id;

	String __title;
	String __description;
	String __link;
	String __url;

	FeedItemCollection *__items;

	long long __pubDate;
	int __ttl;
	String __faviconPath;
	mutable Bitmap *__pFavicon;

	friend class FeedManager;
};

#endif
