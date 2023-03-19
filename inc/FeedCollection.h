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
#ifndef FEEDCOLLECTION_H_
#define FEEDCOLLECTION_H_

#include "FeedItem.h"
#include <FBase.h>

using namespace Osp::Base::Collection;

class FeedItemCollection: public ArrayList {
public:
	FeedItemCollection(void): __unreadCount(0), __maxItemID(0), __itemsChanged(false) { }

	virtual result Add(const Object& obj);

	void CopyFrom(const FeedItemCollection &other);

	int GetChannelID(void) const {
		return __channelID;
	}
	void SetChannelID(int id) {
		__channelID = id;
	}

	FeedItem *GetByID(int item_id) const;

	int GetUnreadItemsCount(void) const {
		return __unreadCount;
	}
	void SetItemAsRead(int itemID);
	void SetAllItemsAsRead(void);
	int SetItemsStatus(const FeedItemCollection &other);
	void SetItemEnclosureDownloaded(int itemID, const String &path);
	void SetItemEnclosureMIME(int itemID, const String &mime);
	void ResetItemEnclosure(int itemID);

	bool ContainsEnclosure(const String &url) const;

	bool Equals(const FeedItemCollection &other) const;
	void SetDataFromXML(const char *pData, int len);

	bool HasChanged(void) const {
		return __itemsChanged;
	}

	int GetMaxItemID(void) const;

private:
	void IncrementUnreadCount(void) {
		__unreadCount++;
	}
	void SetChanged(bool val) const {
		__itemsChanged = val;
	}

	int __channelID;
	int __unreadCount;
	int __maxItemID;

	mutable bool __itemsChanged;

	friend class FeedManager;
};

#endif
