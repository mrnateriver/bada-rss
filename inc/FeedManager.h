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
#ifndef FEEDMANAGER_H_
#define FEEDMANAGER_H_

#include "Channel.h"
#include "FeedItem.h"
#include "FeedCollection.h"

using namespace Osp::Base::Runtime;

class FeedManager {
public:
	static result Initialize(void);
	static FeedManager *GetInstance(void);
	static void RemoveInstance(void);

private:
	static FeedManager *__pInstance;

public:
	FeedManager(void);
	~FeedManager(void);

	result Construct(void);
	result UpdateDatabase(AppVersionInfo old_ver);

	ArrayList *GetChannelsN(void);
	const FeedItemCollection *GetFeedItems(int channel_id);

	ArrayList *FindChannelsN(const String &filter);

	const Channel *GetChannel(int channel_id);

	result AddChannel(const Channel *channel);
	result AddChannels(const IList &pChannels);

	int UpdateChannelFromXML(Channel *channel, const char *pData, int len);
	int UpdateChannelFromXML(int channel_id, const char *pData, int len);

	void UpdateChannelsOrder(int *ids, int count);

	result DeleteChannel(int channel_id);

	result MarkItemAsRead(Channel *channel, int item_id);
	result MarkItemAsRead(int channel_id, int item_id);

	result SetEnclosureAsDownloaded(Channel *channel, int item_id, const String &path);
	result SetEnclosureAsDownloaded(int channel_id, int item_id, const String &path);

	result SetEnclosureMIME(Channel *channel, int item_id, const String &mime);
	result SetEnclosureMIME(int channel_id, int item_id, const String &mime);

	result ResetEnclosure(Channel *channel, int item_id);
	result ResetEnclosure(int channel_id, int item_id);

	result MarkAllItemsAsRead(Channel *channel);
	result MarkAllItemsAsRead(int channel_id);

	result SetURLAndTTLForChannel(Channel *channel, const String &url, int ttl);
	result SetURLAndTTLForChannel(int channel_id, const String &url, int ttl);

	result SetFaviconForChannel(Channel *channel, const ByteBuffer &data);
	result SetFaviconForChannel(int channel_id, const ByteBuffer &data);

	int GetMaxChannelID(void) const {
		return __maxChannelID;
	}

	result CacheChannels(void);
	result CacheFeedItems(void);

	result SerializeAll(void);
	result SerializeChannels(void);
	result SerializeFeedItems(void);

	int GetGlobalUnreadCount(void) const;
	int GetUpdateInterval(void) const;

	Mutex *GetMutex(void) const {
		return __pMgrMutex;
	}

private:
	int GetChannelIndexFromID(int id) const;
	String GenerateDeleteStatementForChangedItems(void);
	bool HasAnyItemsChanged(void) const;
	void ReassignChannelsIDs(void);
	void ReassignItemsIDs(void);

	static const String DATA_PATH;

	ArrayListT<FeedItemCollection *> *__pFeedItems;
	ArrayList *__pChannels;

	bool __channelsChanged;
	int __maxChannelID;

	Mutex *__pMgrMutex;
};

#endif
