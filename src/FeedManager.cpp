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
#include "FeedManager.h"
#include <FIo.h>
#include <FMedia.h>
#include <typeinfo>

using namespace Osp::Io;
using namespace Osp::Media;
using namespace Osp::Base::Collection;

extern const String FeedManager::DATA_PATH = L"/Home/data.bin";
extern FeedManager *FeedManager::__pInstance;

#define MUTEX_ACQUIRE __pMgrMutex->Acquire()
#define MUTEX_RELEASE __pMgrMutex->Release()

result FeedManager::Initialize(void) {
	__pInstance = new FeedManager();
	return __pInstance->Construct();
}

void FeedManager::RemoveInstance(void) {
	delete __pInstance;
	__pInstance = null;
}

FeedManager *FeedManager::GetInstance(void) {
	return __pInstance;
}

FeedManager::FeedManager(void) {
	__pFeedItems = null;
	__pChannels = null;

	__channelsChanged = false;

	__maxChannelID = 0;

	__pMgrMutex = new Mutex;
	__pMgrMutex->Create();
}

FeedManager::~FeedManager(void) {
	if (__pFeedItems) {
		IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			FeedItemCollection *coll;
			pEnum->GetCurrent(coll);
			if (coll) {
				coll->RemoveAll(true);
				delete coll;
			}
		}
		delete pEnum;

		__pFeedItems->RemoveAll();
		delete __pFeedItems;
	}
	if (__pChannels) {
		__pChannels->RemoveAll(true);
		delete __pChannels;
	}
	if (__pMgrMutex) {
		delete __pMgrMutex;
	}
}

struct ChannelsUpdateStruct {
	int id;
	String title;
	String desc;
	String links;
	long long pub_date;
	int ttl;
};

struct ItemsUpdateStruct {
	int id;
	int chan_id;
	String title;
	String content;
	String guid_enc;
	long long pub_date;
	int read;
	int enc_downloaded;
	int enc_len;
};

bool operator==(const ChannelsUpdateStruct &one, const ChannelsUpdateStruct &two) {
	return one.id == two.id && one.pub_date == two.pub_date && one.ttl == two.ttl && one.title.Equals(two.title, true) && one.desc.Equals(two.desc, true) && one.links.Equals(two.links, true);
}

bool operator!=(const ChannelsUpdateStruct &one, const ChannelsUpdateStruct &two) {
	return one.id != two.id || one.pub_date != two.pub_date || one.ttl != two.ttl || !one.title.Equals(two.title, true) || !one.desc.Equals(two.desc, true) || !one.links.Equals(two.links, true);
}

bool operator==(const ItemsUpdateStruct &one, const ItemsUpdateStruct &two) {
	return one.id == two.id && one.enc_downloaded == two.enc_downloaded && one.enc_len == two.enc_len && one.chan_id == two.chan_id &&
			one.pub_date == two.pub_date && one.read == two.read && one.content.Equals(two.content, true) && one.guid_enc.Equals(two.guid_enc, true) && one.title.Equals(two.title, true);
}

bool operator!=(const ItemsUpdateStruct &one, const ItemsUpdateStruct &two) {
	return one.id != two.id || one.enc_downloaded != two.enc_downloaded || one.enc_len != two.enc_len || one.chan_id != two.chan_id ||
			one.pub_date != two.pub_date || one.read != two.read || !one.content.Equals(two.content, true) || !one.guid_enc.Equals(two.guid_enc, true) || !one.title.Equals(two.title, true);
}

result FeedManager::UpdateDatabase(AppVersionInfo old_ver) {
	//version after 1.0.1 is 1.2.0
	if (old_ver.major == 1 && old_ver.minor < 2) {
		Database *pDb = new Database;
		result res = pDb->Construct(DATA_PATH, false);
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::UpdateDatabase: [%s]: failed to load database file", GetErrorMessage(res));
			delete pDb;
			return res;
		}

		res = pDb->ExecuteSql(L"CREATE TABLE tmp_channels (entry_id INTEGER UNIQUE, title TEXT, desc TEXT, links TEXT, ttl_pub_date BLOB)", true);
		if (IsFailed(res)) {
			delete pDb;
			return res;
		}

		DbEnumerator *pEnum = pDb->QueryN(L"SELECT * FROM channels");
		res = GetLastResult();
		if (IsFailed(res)) {
			pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

			delete pDb;
			return res;
		}

		ArrayListT<ChannelsUpdateStruct> chan_update_data;
		chan_update_data.Construct(10);

		if (pEnum) {
			int id;
			String title;
			String desc;
			String html_link;
			String xml_link;
			String favicon;
			long long pub_date;
			int ttl;

			while (!IsFailed(pEnum->MoveNext())) {
				result mres[8];
				mres[0] = pEnum->GetIntAt(0, id);
				mres[1] = pEnum->GetStringAt(2, title);//shift to 2 because we don't care about rss_ver
				mres[2] = pEnum->GetStringAt(3, desc);
				mres[3] = pEnum->GetStringAt(4, html_link);
				mres[4] = pEnum->GetStringAt(5, xml_link);
				mres[5] = pEnum->GetInt64At(6, pub_date);
				mres[6] = pEnum->GetIntAt(7, ttl);
				mres[7] = pEnum->GetStringAt(8, favicon);

				for(int i = 0; i < 8; i++) {
					if (IsFailed(mres[i])) {
						pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

						delete pEnum;
						delete pDb;
						return mres[i];
					}
				}

				ChannelsUpdateStruct st = { id, title, desc, html_link + L"~%~" + xml_link + L"~%~" + favicon, pub_date, ttl };
				chan_update_data.Add(st);
			}
			delete pEnum;
		}

		res = pDb->BeginTransaction();
		if (IsFailed(res)) {
			pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

			delete pDb;
			return res;
		}

		DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO tmp_channels VALUES (?, ?, ?, ?, ?)");
		res = GetLastResult();
		if (IsFailed(res)) {
			pDb->RollbackTransaction();
			pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

			delete pDb;
			return res;
		}

		byte *pubDateTTL = new byte[12];
		for (int i = 0; i < 12; i++) {
			pubDateTTL[i] = 0;
		}

		for (int i = 0; i < chan_update_data.GetCount(); i++) {
			ChannelsUpdateStruct st;
			chan_update_data.GetAt(i, st);

			result bind_res[5];
			bind_res[0] = pEntries->BindInt(0, st.id);
			bind_res[1] = pEntries->BindString(1, st.title);
			bind_res[2] = pEntries->BindString(2, st.desc);
			bind_res[3] = pEntries->BindString(3, st.links);

			*((long long*)pubDateTTL) = st.pub_date;
			*((int*)(pubDateTTL + 8)) = st.ttl;
			bind_res[4] = pEntries->BindBlob(4, (void*)pubDateTTL, 12);

			for(int i = 0; i < 5; i++) {
				if (IsFailed(bind_res[i])) {
					pDb->RollbackTransaction();
					pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

					delete pEntries;
					delete pDb;
					return bind_res[i];
				}
			}

			pDb->ExecuteStatementN(*pEntries);
			res = GetLastResult();
			if (IsFailed(res)) {
				pDb->RollbackTransaction();
				pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

				delete pEntries;
				delete pDb;
				return res;
			}
		}
		delete pEntries;
		delete []pubDateTTL;

		res = pDb->CommitTransaction();
		if (IsFailed(res)) {
			pDb->RollbackTransaction();
			pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

			delete pDb;
			return res;
		}

		res = pDb->ExecuteSql(L"DROP TABLE channels", true);
		if (IsFailed(res)) {
			pDb->ExecuteSql(L"DROP TABLE tmp_channels", true);

			delete pDb;
			return res;
		}
		res = pDb->ExecuteSql(L"ALTER TABLE tmp_channels RENAME TO channels", true);
		if (IsFailed(res)) {
			delete pDb;
			return res;
		}

		res = pDb->ExecuteSql(L"CREATE TABLE tmp_items (entry_id INTEGER PRIMARY KEY, channel_id INTEGER, title TEXT, content TEXT, guid_enc TEXT, data BLOB)", true);
		if (IsFailed(res)) {
			delete pDb;
			return res;
		}

		pEnum = pDb->QueryN(L"SELECT * FROM items");
		res = GetLastResult();
		if (IsFailed(res)) {
			pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

			delete pDb;
			return res;
		}

		ArrayListT<ItemsUpdateStruct> items_update_data;
		items_update_data.Construct(20);

		if (pEnum) {
			int id;
			int chan_id;
			String title;
			String content;
			String guidLink;
			long long pubDate;
			int read;
			int enc_downloaded;
			String enc_url;
			String enc_mime;
			int enc_len;

			while (!IsFailed(pEnum->MoveNext())) {
				result mres[11];
				mres[0] = pEnum->GetIntAt(0, id);
				mres[1] = pEnum->GetIntAt(1, chan_id);
				mres[2] = pEnum->GetStringAt(2, title);
				mres[3] = pEnum->GetStringAt(4, content);//shift to 4 because we don't care for summary
				mres[4] = pEnum->GetStringAt(5, guidLink);
				mres[5] = pEnum->GetInt64At(6, pubDate);
				mres[6] = pEnum->GetIntAt(7, read);
				mres[7] = pEnum->GetIntAt(8, enc_downloaded);
				mres[8] = pEnum->GetStringAt(9, enc_url);
				mres[9] = pEnum->GetStringAt(10, enc_mime);
				mres[10] = pEnum->GetIntAt(11, enc_len);

				for(int i = 0; i < 11; i++) {
					if (IsFailed(mres[i])) {
						pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

						delete pEnum;
						delete pDb;
						return mres[i];
					}
				}

				ItemsUpdateStruct st = { id, chan_id, title, content, guidLink + L"~%~" + enc_url + L"~%~" + enc_mime, pubDate, read, enc_downloaded, enc_len };
				items_update_data.Add(st);
			}
			delete pEnum;
		}

		res = pDb->BeginTransaction();
		if (IsFailed(res)) {
			pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

			delete pDb;
			return res;
		}

		pEntries = pDb->CreateStatementN(L"INSERT INTO tmp_items VALUES (?, ?, ?, ?, ?, ?)");
		res = GetLastResult();
		if (IsFailed(res)) {
			pDb->RollbackTransaction();
			pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

			delete pDb;
			return res;
		}

		byte *itemData = new byte[14];
		for (int i = 0; i < 14; i++) {
			itemData[i] = 0;
		}

		for (int i = 0; i < items_update_data.GetCount(); i++) {
			ItemsUpdateStruct st;
			items_update_data.GetAt(i, st);

			result bind_res[6];
			bind_res[0] = pEntries->BindInt(0, st.id);
			bind_res[1] = pEntries->BindInt(1, st.chan_id);
			bind_res[2] = pEntries->BindString(2, st.title);
			bind_res[3] = pEntries->BindString(3, st.content);
			bind_res[4] = pEntries->BindString(4, st.guid_enc);

			*((long long*)itemData) = st.pub_date;
			*((int*)(itemData + 8)) = st.enc_len;
			*((unsigned char*)(itemData + 12)) = (unsigned char)st.read;
			*((unsigned char*)(itemData + 13)) = (unsigned char)st.enc_downloaded;
			bind_res[5] = pEntries->BindBlob(5, (void*)itemData, 14);

			for(int i = 0; i < 6; i++) {
				if (IsFailed(bind_res[i])) {
					pDb->RollbackTransaction();
					pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

					delete pEntries;
					delete pDb;
					return bind_res[i];
				}
			}

			pDb->ExecuteStatementN(*pEntries);
			res = GetLastResult();
			if (IsFailed(res)) {
				pDb->RollbackTransaction();
				pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

				delete pEntries;
				delete pDb;
				return res;
			}
		}
		delete pEntries;
		delete []itemData;

		res = pDb->CommitTransaction();
		if (IsFailed(res)) {
			pDb->RollbackTransaction();
			pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

			delete pDb;
			return res;
		}

		res = pDb->ExecuteSql(L"DROP TABLE items", true);
		if (IsFailed(res)) {
			pDb->ExecuteSql(L"DROP TABLE tmp_items", true);

			delete pDb;
			return res;
		}
		res = pDb->ExecuteSql(L"ALTER TABLE tmp_items RENAME TO items", true);
		if (IsFailed(res)) {
			delete pDb;
			return res;
		}

		AppVersionInfo cur_ver = Utilities::GetAppVersion();
		res = pDb->ExecuteSql(L"INSERT INTO db_info VALUES ("+Integer::ToString(cur_ver.major)+L","+Integer::ToString(cur_ver.minor)+L","+Integer::ToString(cur_ver.revision)+L")", true);
		if (IsFailed(res)) {
			delete pDb;
			return res;
		}

		res = pDb->ExecuteSql(L"DELETE FROM db_info WHERE ver_major="+Integer::ToString(old_ver.major)+L" AND ver_minor="+Integer::ToString(old_ver.minor)+L" AND ver_revision="+Integer::ToString(old_ver.revision), true);
		if (IsFailed(res)) {
			delete pDb;
			return res;
		}

		delete pDb;
		return E_SUCCESS;
	} else {
		return E_INVALID_ARG;
	}
}

result FeedManager::Construct(void) {
	if (File::IsFileExist(DATA_PATH)) {
		Database *pDb = new Database;
		result res = pDb->Construct(DATA_PATH, false);
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::Initialize: [%s]: failed to load database file", GetErrorMessage(res));
			delete pDb;
			return res;
		}

		DbEnumerator *pEnum = pDb->QueryN(L"SELECT ver_major, ver_minor, ver_revision FROM db_info");
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::Initialize: [%s]: failed to query database", GetErrorMessage(res));
			delete pDb;
			return res;
		}
		if (pEnum) {
			while (!IsFailed(pEnum->MoveNext())) {
				int major_ver, minor_ver, revision;

				res = pEnum->GetIntAt(0, major_ver);
				if (IsFailed(res)) {
					AppLogDebug("FeedManager::Initialize: [%s]: failed to get version data for database", GetErrorMessage(res));
					delete pEnum;
					delete pDb;
					return res;
				}
				res = pEnum->GetIntAt(1, minor_ver);
				if (IsFailed(res)) {
					AppLogDebug("FeedManager::Initialize: [%s]: failed to get version data for database", GetErrorMessage(res));
					delete pEnum;
					delete pDb;
					return res;
				}
				res = pEnum->GetIntAt(2, revision);
				if (IsFailed(res)) {
					AppLogDebug("FeedManager::Initialize: [%s]: failed to get version data for database", GetErrorMessage(res));
					delete pEnum;
					delete pDb;
					return res;
				}

				AppVersionInfo ver = Utilities::GetAppVersion();
				if (major_ver != ver.major || minor_ver != ver.minor || revision != ver.revision) {
					delete pEnum;
					delete pDb;

					if (ver.major > major_ver || (ver.major == major_ver && ver.minor > minor_ver) || (ver.major == major_ver && ver.minor == minor_ver && ver.revision > revision)) {
						AppVersionInfo old_ver = { major_ver, minor_ver, revision };
						res = UpdateDatabase(old_ver);
						if (IsFailed(res)) {
							delete pEnum;
							delete pDb;

							File::Remove(DATA_PATH);
							return Construct();
						} else {
							return res;
						}
					} else {
						return E_INVALID_FORMAT;
					}
				} else {
					delete pEnum;
					delete pDb;
					return E_SUCCESS;
				}
			}
			delete pEnum;
			delete pDb;
			return E_DATA_NOT_FOUND;
		} else {
			delete pDb;
			return E_INVALID_FORMAT;
		}
	} else {
		Database *pDb = new Database;
		result res = pDb->Construct(DATA_PATH, true);
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::Initialize: [%s]: failed to initialize database file", GetErrorMessage(res));

			delete pDb;
			return res;
		}

		result mres[4];
		mres[0] = pDb->ExecuteSql(L"CREATE TABLE db_info (ver_major INTEGER, ver_minor INTEGER, ver_revision INTEGER)", true);
		AppVersionInfo ver = Utilities::GetAppVersion();
		mres[1] = pDb->ExecuteSql(L"INSERT INTO db_info VALUES (" + Integer::ToString(ver.major) + L"," + Integer::ToString(ver.minor) + L"," + Integer::ToString(ver.revision) + L")", true);

		mres[2] = pDb->ExecuteSql(L"CREATE TABLE channels (entry_id INTEGER UNIQUE, title TEXT, desc TEXT, links TEXT, ttl_pub_date BLOB)", true);
		//mres[3] = pDb->ExecuteSql(L"CREATE TABLE items (entry_id INTEGER PRIMARY KEY, channel_id INTEGER, title TEXT, content TEXT, guid_link TEXT, pub_date INTEGER, read INTEGER, enc_downloaded INTEGER, enc_url TEXT, enc_mime TEXT, enc_len INTEGER)", true);
		mres[3] = pDb->ExecuteSql(L"CREATE TABLE items (entry_id INTEGER PRIMARY KEY, channel_id INTEGER, title TEXT, content TEXT, guid_enc TEXT, data BLOB)", true);

		delete pDb;

		for(int i = 0; i < 4; i++) {
			if (IsFailed(mres[i])) {
				AppLogDebug("FeedManager::Initialize: [%s]: failed to initialize database structure at [%d]", GetErrorMessage(mres[i]), i);

				return mres[i];
			}
		}
	}
	return E_SUCCESS;
}

ArrayList *FeedManager::GetChannelsN(void) {
	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			return null;
		}
	}

	MUTEX_ACQUIRE;
	ArrayList *pRet = new ArrayList;
	result res = pRet->Construct(*__pChannels);
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::GetChannelsN: [%s]: failed to construct array list for return", GetErrorMessage(res));
		SetLastResult(res);
		return null;
	}
	MUTEX_RELEASE;
	return pRet;
}

const FeedItemCollection *FeedManager::GetFeedItems(int channel_id) {
	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			return null;
		}
	}

	MUTEX_ACQUIRE;
	FeedItemCollection *res = null;
	IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		pEnum->GetCurrent(res);
		if (res) {
			if (res->GetChannelID() == channel_id) {
				break;
			}
		}
	}
	delete pEnum;
	MUTEX_RELEASE;
	return res;
}

ArrayList *FeedManager::FindChannelsN(const String &filter) {
	if (filter.IsEmpty()) {
		return GetChannelsN();
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::FindChannelsN: [%s]: failed to cache channels", GetErrorMessage(res));
			SetLastResult(res);
			return null;
		}
	}

	ArrayList *pRet = new ArrayList;
	result res = pRet->Construct(10);
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::FindChannelsN: [%s]: failed to construct array list for return", GetErrorMessage(res));
		SetLastResult(res);
		return null;
	}

	String filter_str = filter;
	if (Utilities::IsFirmwareLaterThan10()) {
		filter_str.ToLowerCase();
	} else {
		filter_str.ToLower();
	}

	MUTEX_ACQUIRE;
	String chan_str;
	int indOf = 0;
	IEnumerator *pEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);

			chan_str = chan->GetFilterStrings();
			if (Utilities::IsFirmwareLaterThan10()) {
				chan_str.ToLowerCase();
			} else {
				chan_str.ToLower();
			}

			res = chan_str.IndexOf(filter_str, 0, indOf);
			if (!IsFailed(res)) {
				pRet->Add(*chan);
			}
		}
	}
	delete pEnum;
	MUTEX_RELEASE;

	return pRet;
}

const Channel *FeedManager::GetChannel(int channel_id) {
	if (channel_id < 0) {
		SetLastResult(E_INVALID_ARG);
		return null;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::GetChannel: [%s]: failed to cache channels", GetErrorMessage(res));
			SetLastResult(res);
			return null;
		}
	}

	MUTEX_ACQUIRE;
	Channel *res = null;
	IEnumerator *pEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan->GetID() == channel_id) {
				res = chan;
				break;
			}
		}
	}
	delete pEnum;
	MUTEX_RELEASE;

	return res;
}

result FeedManager::AddChannel(const Channel *channel) {
	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::AddChannel: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::AddChannel: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	int index = 0;
	Channel *found = null;
	IEnumerator *pEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan->GetURL() == channel->GetURL()) {
				found = chan;
				break;
			}
			index++;
		}
	}
	delete pEnum;

	if (found) {
		if (!__pChannels->Contains(*channel)) {
			found->SetFrom(*channel);
			__channelsChanged = true;
			//don't change ID because it already has one
		}
		MUTEX_RELEASE;
		return E_SUCCESS;
	} else {
		Channel *to_add = new Channel(*channel);

		__pChannels->Add(*to_add);
		__channelsChanged = true;

		if (to_add->GetItems()) {
			__pFeedItems->Add(const_cast<FeedItemCollection *>(to_add->GetItems()));
			to_add->GetItems()->SetChanged(true);
		}

		to_add->SetID(GetMaxChannelID() + 1);
		__maxChannelID++;
	}
	MUTEX_RELEASE;
	return E_SUCCESS;
}

result FeedManager::AddChannels(const IList &pChannels) {
	IEnumerator *pEnum = pChannels.GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			AddChannel(chan);
		}
	}
	delete pEnum;

	return E_SUCCESS;
}

int FeedManager::UpdateChannelFromXML(Channel *channel, const char *pData, int len) {
	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::DeleteChannel: [%s]: failed to cache channels", GetErrorMessage(res));
			SetLastResult(res);
			return -1;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::DeleteChannel: [%s]: failed to cache items list first", GetErrorMessage(res));
			SetLastResult(res);
			return -1;
		}
	}

	if (Utilities::GetCurrentUTCUnixTicks() - channel->GetPubDate() < channel->GetTTL() * 60 * 1000) {
		SetLastResult(E_SUCCESS);
		return 0;
	}

	MUTEX_ACQUIRE;
	bool found = false;
	IEnumerator *pEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan == channel) {
				found = true;
				break;
			}
		}
	}
	delete pEnum;

	int new_items = 0;
	if (found) {
		if (channel->SetDataFromXML(pData, len)) {
			__channelsChanged = true;
		}

		int item_index = 0;
		FeedItemCollection *res = null;
		FeedItemCollection *items = null;
		IEnumeratorT<FeedItemCollection *> *pEnumT = __pFeedItems->GetEnumeratorN();
		while (!IsFailed(pEnumT->MoveNext())) {
			pEnumT->GetCurrent(res);
			if (res) {
				if (res->GetChannelID() == channel->GetID()) {
					items = res;
					break;
				}
				item_index++;
			}
		}
		delete pEnumT;

		channel->SetID(channel->GetID());
		if (items) {
			new_items = const_cast<FeedItemCollection *>(channel->GetItems())->SetItemsStatus(*items);
			if (!items->HasChanged() && items->Equals(*channel->GetItems())) {
				channel->GetItems()->SetChanged(false);
			}

			items->RemoveAll(true);
			delete items;

			__pFeedItems->SetAt(const_cast<FeedItemCollection *>(channel->GetItems()), item_index);
		} else {
			__pFeedItems->Add(const_cast<FeedItemCollection *>(channel->GetItems()));
			new_items = channel->GetItems()->GetCount();
		}
	} else {
		channel->SetDataFromXML(pData, len);
	}
	MUTEX_RELEASE;

	SetLastResult(E_SUCCESS);
	return new_items;
}

int FeedManager::UpdateChannelFromXML(int channel_id, const char *pData, int len) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return UpdateChannelFromXML(const_cast<Channel *>(chan), pData, len);
	} else {
		SetLastResult(E_OBJ_NOT_FOUND);
		return -1;
	}
}

void FeedManager::UpdateChannelsOrder(int *ids, int count) {
	if (!ids || count <= 0 || count != __pChannels->GetCount()) {
		return;
	}
	for (int i = 0; i < count; i++) {
		Object *obj = __pChannels->GetAt(i);
		if (obj && typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (*(ids + i) != chan->GetID()) {
				int index = 0;
				Channel *thisChan = null;
				IEnumerator *pChansEnum = __pChannels->GetEnumeratorN();
				while (!IsFailed(pChansEnum->MoveNext())) {
					Object *itObj = pChansEnum->GetCurrent();
					if (itObj && typeid(*itObj) == typeid(Channel)) {
						Channel *itChan = static_cast<Channel *>(itObj);
						if (itChan->GetID() == *(ids + i)) {
							thisChan = itChan;
							break;
						} else {
							index++;
						}
					}
				}
				delete pChansEnum;
				if (thisChan) {
					__pChannels->RemoveAt(index, false);
					__pChannels->InsertAt(*thisChan, i);

					__channelsChanged = true;
				}
			}
		}
	}
}

result FeedManager::DeleteChannel(int channel_id) {
	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::DeleteChannel: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::DeleteChannel: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	int indOf = -1;
	bool found = false;
	IEnumerator *pEnum = __pChannels->GetEnumeratorN();
	Channel *found_chan = null;
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			found_chan = static_cast<Channel *>(obj);
			indOf++;
			if (found_chan->GetID() == channel_id) {
				found = true;
				break;
			}
		}
	}
	delete pEnum;

	if (indOf >= 0 && found_chan && found) {
		if (found_chan) {
			File::Remove(L"/Home/favicons/" + found_chan->GetUniqueString() + L".png");
			Directory::Remove(L"/Home/enclosures/" + found_chan->GetUniqueString() + L"/", true);
		}
		__pChannels->RemoveAt(indOf, true);
		__channelsChanged = true;

		int item_index = 0;
		FeedItemCollection *res = null;
		found = false;
		IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			pEnum->GetCurrent(res);
			if (res) {
				if (res->GetChannelID() == channel_id) {
					found = true;
					break;
				}
			}
			item_index++;
		}
		delete pEnum;

		if (res && found) {
			res->RemoveAll(true);
			delete res;

			__pFeedItems->RemoveAt(item_index);
		}

		MUTEX_RELEASE;
		return E_SUCCESS;
	} else {
		AppLogDebug("FeedManager::DeleteChannel: couldn't find channel with specified ID");

		MUTEX_RELEASE;
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::MarkItemAsRead(Channel *channel, int item_id) {
	if (!channel) {
		return E_INVALID_ARG;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::MarkItemAsRead: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::MarkItemAsRead: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	if (channel->GetItems()) {
		const_cast<FeedItemCollection *>(channel->GetItems())->SetItemAsRead(item_id);
	}
	MUTEX_RELEASE;

	return E_SUCCESS;
}

result FeedManager::MarkItemAsRead(int channel_id, int item_id) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return MarkItemAsRead(const_cast<Channel *>(chan), item_id);
	} else {
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::SetEnclosureAsDownloaded(Channel *channel, int item_id, const String &path) {
	if (!channel) {
		return E_INVALID_ARG;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetEnclosureAsDownloaded: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetEnclosureAsDownloaded: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	if (channel->GetItems()) {
		const_cast<FeedItemCollection *>(channel->GetItems())->SetItemEnclosureDownloaded(item_id, path);
	}
	MUTEX_RELEASE;

	return E_SUCCESS;
}

result FeedManager::SetEnclosureAsDownloaded(int channel_id, int item_id, const String &path) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return SetEnclosureAsDownloaded(const_cast<Channel *>(chan), item_id, path);
	} else {
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::SetEnclosureMIME(Channel *channel, int item_id, const String &mime) {
	if (!channel) {
		return E_INVALID_ARG;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetEnclosureMIME: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetEnclosureMIME: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	if (channel->GetItems()) {
		const_cast<FeedItemCollection *>(channel->GetItems())->SetItemEnclosureMIME(item_id, mime);
	}
	MUTEX_RELEASE;

	return E_SUCCESS;
}

result FeedManager::SetEnclosureMIME(int channel_id, int item_id, const String &mime) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return SetEnclosureMIME(const_cast<Channel *>(chan), item_id, mime);
	} else {
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::ResetEnclosure(Channel *channel, int item_id) {
	if (!channel) {
		return E_INVALID_ARG;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::ResetEnclosure: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::ResetEnclosure: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	if (channel->GetItems()) {
		const_cast<FeedItemCollection *>(channel->GetItems())->ResetItemEnclosure(item_id);
	}
	MUTEX_RELEASE;

	return E_SUCCESS;
}

result FeedManager::ResetEnclosure(int channel_id, int item_id) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return ResetEnclosure(const_cast<Channel *>(chan), item_id);
	} else {
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::MarkAllItemsAsRead(Channel *channel) {
	if (!channel) {
		return E_INVALID_ARG;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::MarkItemAsRead: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::MarkItemAsRead: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	if (channel->GetItems()) {
		const_cast<FeedItemCollection *>(channel->GetItems())->SetAllItemsAsRead();
	}
	MUTEX_RELEASE;

	return E_SUCCESS;
}

result FeedManager::MarkAllItemsAsRead(int channel_id) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return MarkAllItemsAsRead(const_cast<Channel *>(chan));
	} else {
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::SetURLAndTTLForChannel(Channel *channel, const String &url, int ttl) {
	if (!channel) {
		return E_INVALID_ARG;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetURLAndTTLForChannel: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetURLAndTTLForChannel: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	bool found = false;
	IEnumerator *pEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan == channel) {
				found = true;
				break;
			}
		}
	}
	delete pEnum;

	channel->SetURL(url);
	channel->SetTTL(ttl);

	if (found) {
		__channelsChanged = true;
	}

	MUTEX_RELEASE;
	return E_SUCCESS;
}

result FeedManager::SetURLAndTTLForChannel(int channel_id, const String &url, int ttl) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return SetURLAndTTLForChannel(const_cast<Channel *>(chan), url, ttl);
	} else {
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::SetFaviconForChannel(Channel *channel, const ByteBuffer &data) {
	if (!channel) {
		return E_INVALID_ARG;
	}

	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetFaviconForChannel: [%s]: failed to cache channels", GetErrorMessage(res));
			return res;
		}
	}

	if (!__pFeedItems) {
		result res = CacheFeedItems();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SetFaviconForChannel: [%s]: failed to cache items list first", GetErrorMessage(res));
			return res;
		}
	}

	MUTEX_ACQUIRE;
	bool found = false;
	IEnumerator *pEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan == channel) {
				found = true;
				break;
			}
		}
	}
	delete pEnum;

	Image *img = new Image;
	result res = img->Construct();
	if (!IsFailed(res)) {
		Bitmap *icon = null;
		icon = img->DecodeN(data, IMG_FORMAT_PNG, BITMAP_PIXEL_FORMAT_ARGB8888);

		res = GetLastResult();
		if (!IsFailed(res)) {
			String url = Utilities::SaveFavicon(icon, *channel);

			res = GetLastResult();
			if (!IsFailed(res)) {
				if (channel->GetFavicon()) {
					delete channel->GetFavicon();
				}

				channel->SetFavicon(icon);
				channel->SetFaviconPath(url);
			} else {
				AppLogDebug("FeedManager::SetFaviconForChannel: [%s]: failed to encode favicon bitmap", GetErrorMessage(res));

				delete img;
				return res;
			}
		} else {
			AppLogDebug("FeedManager::SetFaviconForChannel: [%s]: failed to decode favicon bitmap", GetErrorMessage(res));

			delete img;
			return res;
		}
	}
	delete img;

	if (found) {
		__channelsChanged = true;
	}

	MUTEX_RELEASE;
	return E_SUCCESS;
}

result FeedManager::SetFaviconForChannel(int channel_id, const ByteBuffer &data) {
	const Channel *chan = GetChannel(channel_id);
	if (chan) {
		return SetFaviconForChannel(const_cast<Channel *>(chan), data);
	} else {
		return E_OBJ_NOT_FOUND;
	}
}

result FeedManager::CacheChannels(void) {
	if (__pChannels) {
		AppLogDebug("FeedManager::CacheChannels: channels are already cached");
		return E_INVALID_STATE;
	}

	MUTEX_ACQUIRE;
	__pChannels = new ArrayList;
	result res = __pChannels->Construct(10);
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::CacheChannels: [%s]: failed to construct channels list", GetErrorMessage(res));
		delete __pChannels;
		__pChannels = null;

		MUTEX_RELEASE;
		return res;
	}

	Database *pDb = new Database;
	res = pDb->Construct(DATA_PATH, false);
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::CacheChannels: [%s]: failed to load database file", GetErrorMessage(res));
		delete pDb;

		MUTEX_RELEASE;
		return res;
	}

	DbEnumerator *pEnum = pDb->QueryN(L"SELECT * FROM channels");
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::CacheChannels: [%s]: failed to execute query statement", GetErrorMessage(res));
		delete pDb;

		MUTEX_RELEASE;
		return res;
	}

	if (pEnum) {
		int id;
		String title;
		String description;
		String links;

		byte *pubDateTTL = new byte[12];
		for (int i = 0; i < 12; i++) {
			pubDateTTL[i] = 0;
		}

		String html_link;
		String xml_link;
		String favicon;

		while (!IsFailed(pEnum->MoveNext())) {
			result mres[5];
			mres[0] = pEnum->GetIntAt(0, id);
			mres[1] = pEnum->GetStringAt(1, title);
			mres[2] = pEnum->GetStringAt(2, description);
			mres[3] = pEnum->GetStringAt(3, links);
			mres[4] = pEnum->GetBlobAt(4, (void*)pubDateTTL, 12);

			for(int i = 0; i < 5; i++) {
				if (IsFailed(mres[i])) {
					AppLogDebug("FeedManager::CacheChannels: [%s]: failed to fetch query results at [%d]", GetErrorMessage(mres[i]), i);
					delete pEnum;
					delete pDb;

					MUTEX_RELEASE;
					return mres[i];
				}
			}

			html_link = L"";
			xml_link = L"";
			favicon = L"";

			int sepIndex = -1;
			if (!IsFailed(links.IndexOf(L"~%~", 0, sepIndex)) && sepIndex >= 0) {
				links.SubString(0, sepIndex, html_link);
				links.Remove(0, sepIndex + 3);
			}
			if (!IsFailed(links.IndexOf(L"~%~", 0, sepIndex)) && sepIndex >= 0) {
				links.SubString(0, sepIndex, xml_link);
				links.Remove(0, sepIndex + 3);

				favicon = links;
			}

			FeedItemCollection *items = new FeedItemCollection;
			items->Construct(20);

			Channel *pChan = new Channel(title, description, html_link, xml_link);
			pChan->SetItems(items);
			pChan->SetID(id);
			pChan->SetPubDate(*((long long*)pubDateTTL));
			pChan->SetTTL(*((int*)(pubDateTTL + 8)));
			if (!favicon.IsEmpty()) {
				pChan->SetFaviconPath(favicon);
				pChan->SetFavicon(Utilities::GetBitmapN(favicon, true));
			}

			__pChannels->Add(*pChan);
			if (id > __maxChannelID) {
				__maxChannelID = id;
			}
		}
		delete pEnum;
		delete []pubDateTTL;
	}
	delete pDb;

	MUTEX_RELEASE;
	return E_SUCCESS;
}

result FeedManager::CacheFeedItems(void) {
	if (!__pChannels) {
		result res = CacheChannels();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::CacheFeedItems: failed to cache channels list first");
			return res;
		}
	}

	if (__pFeedItems) {
		AppLogDebug("FeedManager::CacheFeedItems: feed items are already cached");
		return E_INVALID_STATE;
	}

	MUTEX_ACQUIRE;
	__pFeedItems = new ArrayListT<FeedItemCollection *>;
	result res = __pFeedItems->Construct(10);
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::CacheFeedItems: [%s]: failed to construct feed items list", GetErrorMessage(res));
		delete __pFeedItems;
		__pFeedItems = null;

		MUTEX_RELEASE;
		return res;
	}

	Database *pDb = new Database;
	res = pDb->Construct(DATA_PATH, false);
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::CacheFeedItems: [%s]: failed to load database file", GetErrorMessage(res));
		delete pDb;

		MUTEX_RELEASE;
		return res;
	}

	DbEnumerator *pEnum = pDb->QueryN(L"SELECT channel_id, title, content, guid_enc, data FROM items");
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogDebug("FeedManager::CacheFeedItems: [%s]: failed to execute query statement", GetErrorMessage(res));
		delete pDb;

		MUTEX_RELEASE;
		return res;
	}

	ArrayListT<int> *obsolete = new ArrayListT<int>;
	if (IsFailed(obsolete->Construct(10))) {
		delete obsolete;
		obsolete = null;
	}
	if (pEnum) {
		int chan_id;
		String title;
		String content;
		String guidEnc;

		byte *data = new byte[14];
		for (int i = 0; i < 14; i++) {
			data[i] = 0;
		}

		String guidLink;
		String enc_url;
		String enc_mime;

		while (!IsFailed(pEnum->MoveNext())) {
			result mres[5];
			mres[0] = pEnum->GetIntAt(0, chan_id);
			mres[1] = pEnum->GetStringAt(1, title);
			mres[2] = pEnum->GetStringAt(2, content);
			mres[3] = pEnum->GetStringAt(3, guidEnc);
			mres[4] = pEnum->GetBlobAt(4, (void*)data, 14);

			for(int i = 0; i < 5; i++) {
				if (IsFailed(mres[i])) {
					AppLogDebug("FeedManager::CacheFeedItems: [%s]: failed to fetch query results at [%d]", GetErrorMessage(mres[i]), i);
					delete pEnum;
					delete pDb;

					MUTEX_RELEASE;
					return mres[i];
				}
			}

			guidLink = L"";
			enc_url = L"";
			enc_mime = L"";

			int sepIndex = -1;
			if (!IsFailed(guidEnc.IndexOf(L"~%~", 0, sepIndex)) && sepIndex >= 0) {
				guidEnc.SubString(0, sepIndex, guidLink);
				guidEnc.Remove(0, sepIndex + 3);
			}
			if (!IsFailed(guidEnc.IndexOf(L"~%~", 0, sepIndex)) && sepIndex >= 0) {
				guidEnc.SubString(0, sepIndex, enc_url);
				guidEnc.Remove(0, sepIndex + 3);

				enc_mime = guidEnc;
			}

			FeedItem *pItem = new FeedItem(title, content);
			pItem->SetRead((bool)*((unsigned char*)(data + 12)));
			pItem->SetGuidLink(guidLink);
			pItem->SetPubDate(*((long long*)data));
			pItem->SetEnclosure(enc_url, enc_mime, *((int*)(data + 8)));
			pItem->SetEnclosureDownloaded((bool)*((unsigned char*)(data + 13)));

			const Channel *chan = GetChannel(chan_id);
			if (chan) {
				const_cast<FeedItemCollection *>(chan->GetItems())->Add(*pItem);
			} else if (obsolete && !obsolete->Contains(chan_id)) {
				obsolete->Add(chan_id);
			}
		}
		delete pEnum;
		delete []data;
	}

	if (obsolete && obsolete->GetCount() > 0) {
		res = pDb->BeginTransaction();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::CacheFeedItems: [%s]: failed to begin transaction", GetErrorMessage(res));
		} else {
			IEnumeratorT<int> *pEnum = obsolete->GetEnumeratorN();
			while (!IsFailed(pEnum->MoveNext())) {
				int id = -1;
				pEnum->GetCurrent(id);
				if (id >= 0) {
					pDb->ExecuteSql(L"DELETE FROM items WHERE channel_id=" + Integer::ToString(id), false);
				}
			}
			delete pEnum;

			res = pDb->CommitTransaction();
			if (IsFailed(res)) {
				AppLogDebug("FeedManager::CacheFeedItems: [%s]: failed to commit transaction", GetErrorMessage(res));
			}
		}
	}
	if (obsolete) {
		obsolete->RemoveAll();
		delete obsolete;
	}
	delete pDb;

	IEnumerator *pChansEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pChansEnum->MoveNext())) {
		Object *obj = pChansEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan->GetItems()) {
				__pFeedItems->Add(const_cast<FeedItemCollection *>(chan->GetItems()));
				chan->GetItems()->SetChanged(false);
				//remove obsolete enclosure
				chan->RemoveObsoleteEnclosures();
			}
		}
	}
	delete pChansEnum;

	MUTEX_RELEASE;
	return E_SUCCESS;
}

result FeedManager::SerializeAll(void) {
	result res = SerializeChannels();
	if (IsFailed(res)) {
		return res;
	} else return SerializeFeedItems();
}

result FeedManager::SerializeChannels(void) {
	if (!__channelsChanged) {
		return E_SUCCESS;
	}
	if (__pChannels) {
		AppLogDebug("Started channels serialization...");
		MUTEX_ACQUIRE;
		Database *pDb = new Database;
		result res = pDb->Construct(DATA_PATH, false);
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeChannels: [%s]: failed to load database file", GetErrorMessage(res));
			delete pDb;

			MUTEX_RELEASE;
			return res;
		}

		res = pDb->BeginTransaction();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeChannels: [%s]: failed to begin transaction", GetErrorMessage(res));
			delete pDb;

			MUTEX_RELEASE;
			return res;
		}

		DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO channels VALUES (?, ?, ?, ?, ?)");
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeChannels: [%s]: failed to initialize transaction statement", GetErrorMessage(res));
			pDb->RollbackTransaction();

			delete pDb;

			MUTEX_RELEASE;
			return res;
		}

		res = pDb->ExecuteSql(L"DELETE FROM channels", false);
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeChannels: [%s]: failed to execute statement for clearing table", GetErrorMessage(res));
			pDb->RollbackTransaction();

			delete pEntries;
			delete pDb;

			MUTEX_RELEASE;
			return res;
		}

		String links = L"";
		byte *pubDateTTL = new byte[12];
		for (int i = 0; i < 12; i++) {
			pubDateTTL[i] = 0;
		}

		IEnumerator *pEnum = __pChannels->GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			Object *obj = pEnum->GetCurrent();
			if (typeid(*obj) == typeid(Channel)) {
				Channel *chan = static_cast<Channel *>(obj);

				result bind_res[5];
				bind_res[0] = pEntries->BindInt(0, chan->GetID());
				bind_res[1] = pEntries->BindString(1, chan->GetTitle());
				bind_res[2] = pEntries->BindString(2, chan->GetDescription());

				links = chan->GetLink() + L"~%~" + chan->GetURL() + L"~%~" + chan->GetFaviconPath();
				bind_res[3] = pEntries->BindString(3, links);

				*((long long*)pubDateTTL) = chan->GetPubDate();
				*((int*)(pubDateTTL + 8)) = chan->GetTTL();
				bind_res[4] = pEntries->BindBlob(4, (void*)pubDateTTL, 12);

				for(int i = 0; i < 5; i++) {
					if (IsFailed(bind_res[i])) {
						AppLogDebug("FeedManager::SerializeChannels: [%s]: failed to bind transaction data at [%d]", GetErrorMessage(res), i);
						pDb->RollbackTransaction();

						delete pEntries;
						delete pDb;
						delete pEnum;

						MUTEX_RELEASE;
						return bind_res[i];
					}
				}

				pDb->ExecuteStatementN(*pEntries);
				res = GetLastResult();
				if (IsFailed(res)) {
					AppLogDebug("FeedManager::SerializeChannels: [%s]: failed to execute transaction statement", GetErrorMessage(res));
					pDb->RollbackTransaction();

					delete pEntries;
					delete pDb;
					delete pEnum;

					MUTEX_RELEASE;
					return res;
				}
			}
		}
		delete pEnum;
		delete pEntries;
		delete []pubDateTTL;

		res = pDb->CommitTransaction();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeChannels: [%s]: failed to commit transaction", GetErrorMessage(res));
		} else {
			__channelsChanged = false;
		}
		delete pDb;

		MUTEX_RELEASE;
		AppLogDebug("Finished channels serialization!");
		return res;
	} else {
		MUTEX_RELEASE;
		return E_INVALID_STATE;
	}
}

result FeedManager::SerializeFeedItems(void) {
	if (!HasAnyItemsChanged()) {
		return E_SUCCESS;
	}
	if (__pFeedItems) {
		AppLogDebug("Started items serialization...");
		MUTEX_ACQUIRE;
		Database *pDb = new Database;
		result res = pDb->Construct(DATA_PATH, false);
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeFeedItems: [%s]: failed to load database file", GetErrorMessage(res));
			delete pDb;

			MUTEX_RELEASE;
			return res;
		}

		res = pDb->BeginTransaction();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeFeedItems: [%s]: failed to begin transaction", GetErrorMessage(res));
			delete pDb;

			MUTEX_RELEASE;
			return res;
		}

		DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO items (channel_id, title, content, guid_enc, data) VALUES (?, ?, ?, ?, ?)");
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogDebug("FeedManager::SerializeFeedItems: [%s]: failed to initialize transaction statement", GetErrorMessage(res));
			pDb->RollbackTransaction();

			delete pDb;

			MUTEX_RELEASE;
			return res;
		}

		String delete_stmt = GenerateDeleteStatementForChangedItems();
		if (!delete_stmt.IsEmpty()) {
			res = pDb->ExecuteSql(delete_stmt, false);
			if (IsFailed(res)) {
				AppLogDebug("FeedManager::SerializeFeedItems: [%s]: failed to execute statement for clearing items table", GetErrorMessage(res));
				pDb->RollbackTransaction();

				delete pEntries;
				delete pDb;

				MUTEX_RELEASE;
				return res;
			}

			String guid_enc = L"";
			byte *data = new byte[14];
			for (int i = 0; i < 14; i++) {
				data[i] = 0;
			}

			FeedItemCollection *pChanCol = null;
			IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
			while (!IsFailed(pEnum->MoveNext())) {
				pEnum->GetCurrent(pChanCol);
				if (pChanCol && pChanCol->HasChanged()) {
					AppLogDebug("Serializing channel [%d]...", pChanCol->GetChannelID());

					IEnumerator *pColEnum = pChanCol->GetEnumeratorN();
					while (!IsFailed(pColEnum->MoveNext())) {
						FeedItem *item = static_cast<FeedItem *>(pColEnum->GetCurrent());

						//don't check for errors or anything to optimize performance
						pEntries->BindInt(0, pChanCol->GetChannelID());
						pEntries->BindString(1, item->GetTitle());
						pEntries->BindString(2, item->GetContent());

						guid_enc = item->GetGuidLink() + L"~%~" + item->GetEnclosureURL() + L"~%~" + item->GetEnclosureMIME();
						pEntries->BindString(3, guid_enc);

						*((long long*)data) = item->GetPubDate();
						*((int*)(data + 8)) = item->GetEnclosureLength();
						*((unsigned char*)(data + 12)) = (unsigned char)item->IsRead();
						*((unsigned char*)(data + 13)) = (unsigned char)item->IsEnclosureDownloaded();
						pEntries->BindBlob(4, (void*)data, 14);

						pDb->ExecuteStatementN(*pEntries);
						res = GetLastResult();
						if (IsFailed(res)) {
							AppLogDebug("FeedManager::SerializeFeedItems: [%s]: failed to execute transaction statement", GetErrorMessage(res));
							pDb->RollbackTransaction();

							delete pEntries;
							delete pDb;
							delete pEnum;
							delete pColEnum;

							MUTEX_RELEASE;
							return res;
						}
					}
					delete pColEnum;

					AppLogDebug("Serialized channel!");
				}
			}
			delete pEnum;
			delete pEntries;
			delete []data;

			AppLogDebug("Committing transaction...");
			res = pDb->CommitTransaction();
			if (IsFailed(res)) {
				AppLogDebug("FeedManager::SerializeFeedItems: [%s]: failed to commit transaction", GetErrorMessage(res));
			}
		} else {
			pDb->RollbackTransaction();
			delete pEntries;
		}
		delete pDb;

		MUTEX_RELEASE;
		AppLogDebug("Finished items serialization!");
		return res;
	} else {

		MUTEX_RELEASE;
		return E_INVALID_STATE;
	}
}

String FeedManager::GenerateDeleteStatementForChangedItems(void) {
	String stmt = L"DELETE FROM items WHERE ";
	int count = 0;

	FeedItemCollection *res = null;
	IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		pEnum->GetCurrent(res);
		if (res) {
			if (res->HasChanged()) {
				stmt.Append(L"channel_id=");
				stmt.Append(res->GetChannelID());
				stmt.Append(L" OR ");

				count++;
			}
		}
	}
	delete pEnum;

	if (count > 0) {
		stmt.Remove(stmt.GetLength() - 4, 4);
	} else {
		stmt = L"";
	}
	return stmt;
}

bool FeedManager::HasAnyItemsChanged(void) const {
	FeedItemCollection *res = null;
	IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		pEnum->GetCurrent(res);
		if (res) {
			if (res->HasChanged()) {
				delete pEnum;
				return true;
			}
		}
	}
	delete pEnum;
	return false;
}

void FeedManager::ReassignChannelsIDs(void) {
	int id = 0;
	IEnumerator *pChansEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pChansEnum->MoveNext())) {
		Object *obj = pChansEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			chan->SetID(id++);
		}
	}
	delete pChansEnum;
}

void FeedManager::ReassignItemsIDs(void) {
	int id = 0;
	FeedItemCollection *res = null;
	IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		pEnum->GetCurrent(res);
		if (res) {
			IEnumerator *pSecEnum = res->GetEnumeratorN();
			while (!IsFailed(pSecEnum->MoveNext())) {
				Object *obj = pSecEnum->GetCurrent();
				if (typeid(*obj) == typeid(FeedItem)) {
					FeedItem *item = static_cast<FeedItem *>(obj);
					item->SetID(id++);
				}
			}
			delete pSecEnum;
		}
	}
	delete pEnum;
}

int FeedManager::GetGlobalUnreadCount(void) const {
	MUTEX_ACQUIRE;
	int count = 0;
	FeedItemCollection *res = null;
	IEnumeratorT<FeedItemCollection *> *pEnum = __pFeedItems->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		pEnum->GetCurrent(res);
		if (res) {
			count += res->GetUnreadItemsCount();
		}
	}
	delete pEnum;
	MUTEX_RELEASE;
	return count;
}

int FeedManager::GetUpdateInterval(void) const {
	int high_count = 0;
	int min_int = 0xFFFFFFFF;

	int low_count = 0;
	int low_sum = 0;

	MUTEX_ACQUIRE;
	IEnumerator *pChansEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pChansEnum->MoveNext())) {
		Object *obj = pChansEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan->GetTTL() > 20) {
				high_count++;
				if (chan->GetTTL() < min_int) {
					min_int = chan->GetTTL();
				}
			} else {
				low_count++;
				low_sum += chan->GetTTL();
			}
		}
	}
	delete pChansEnum;
	MUTEX_RELEASE;

	if (high_count >= __pChannels->GetCount()) {
		return min_int;
	} else {
		return (int)((double)low_sum / low_count);
	}
}

int FeedManager::GetChannelIndexFromID(int id) const {
	int index = 0;
	MUTEX_ACQUIRE;
	IEnumerator *pChansEnum = __pChannels->GetEnumeratorN();
	while (!IsFailed(pChansEnum->MoveNext())) {
		Object *obj = pChansEnum->GetCurrent();
		if (obj && typeid(*obj) == typeid(Channel)) {
			Channel *chan = static_cast<Channel *>(obj);
			if (chan->GetID() == id) {
				break;
			} else {
				index++;
			}
		}
	}
	delete pChansEnum;
	MUTEX_RELEASE;
	return index;
}
