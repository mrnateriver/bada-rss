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
#include "FeedCollection.h"
#include <FXml.h>
#include <typeinfo>

using namespace Osp::Base::Runtime;
using namespace Osp::Base::Utility;
using namespace Osp::Xml;

result FeedItemCollection::Add(const Object& obj) {
	if (typeid(obj) == typeid(FeedItem)) {
		FeedItem *item = (FeedItem *)&obj;

		item->SetID(__maxItemID++);
		if (!item->IsRead()) {
			__unreadCount++;
		}
		__itemsChanged = true;
	}
	return ArrayList::Add(obj);
}

void FeedItemCollection::CopyFrom(const FeedItemCollection &other) {
	IEnumerator *pEnum = other.GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			Add(*(new FeedItem(*(static_cast<FeedItem *>(obj)))));
		}
	}
	delete pEnum;
	__unreadCount = other.__unreadCount;
}

int FeedItemCollection::GetMaxItemID(void) const {
	int max_id = 0;
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			if (item->GetID() > max_id) {
				max_id = item->GetID();
			}
		}
	}
	delete pEnum;
	return max_id;
}

void FeedItemCollection::SetDataFromXML(const char *pData, int len) {
	xmlDocPtr doc = xmlReadMemory(pData, len, null, null, XML_PARSE_NOCDATA);
	if (doc) {
		xmlNodePtr root = xmlDocGetRootElement(doc);
		if (root) {
			if (strcmp((const char*)root->name, "rss") && strcmp((const char*)root->name, "feed")) {
				AppLogDebug("FeedItemCollection::SetDataFromXML: data is not an RSS/Atom structure");
				xmlFreeDoc(doc);
				return;
			}

			bool isRSS = false;
			if (!strcmp((const char*)root->name, "rss")) {
				isRSS = true;
			}

			GregorianCalendar calendar;
			calendar.Construct();

			for (xmlNodePtr pCurElem = root->children; pCurElem; pCurElem = pCurElem->next) {
				if (pCurElem->type == XML_ELEMENT_NODE) {
					if (isRSS && !strcmp((const char*)pCurElem->name, "channel")) {
						for (xmlNodePtr pCurRSSElem = pCurElem->children; pCurRSSElem; pCurRSSElem = pCurRSSElem->next) {
							if (pCurRSSElem->type == XML_ELEMENT_NODE && !strcmp((const char*)pCurRSSElem->name, "item")) {
								FeedItem *pItem = new FeedItem;
								pItem->SetDataFromXML(*pCurRSSElem, &calendar);
								if (!IsFailed(GetLastResult())) {
									Add(*pItem);
									Thread::Yield();
								} else {
									delete pItem;
								}
							}
						}
						break;
					} else {
						if (!strcmp((const char*)pCurElem->name, "entry")) {
							FeedItem *pItem = new FeedItem;
							pItem->SetDataFromXML(*pCurElem, &calendar);
							if (!IsFailed(GetLastResult())) {
								Add(*pItem);
								Thread::Yield();
							} else {
								delete pItem;
							}
						}
					}
				}
			}
		}
		xmlFreeDoc(doc);
	}
	//__unreadCount = GetCount();
	//__itemsChanged = true;
}

FeedItem *FeedItemCollection::GetByID(int item_id) const {
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			if (item->GetID() == item_id) {
				delete pEnum;
				return item;
			}
		}
	}
	delete pEnum;
	return null;
}

void FeedItemCollection::SetItemAsRead(int itemID) {
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			if (item->GetID() == itemID && !item->IsRead()) {
				item->SetRead(true);
				__unreadCount--;
				if (__unreadCount < 0) {
					__unreadCount = 0;
				}
				__itemsChanged = true;
				break;
			}
		}
	}
	delete pEnum;
}

void FeedItemCollection::SetItemEnclosureDownloaded(int itemID, const String &path) {
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			if (item->GetID() == itemID) {
				item->SetEnclosure(path + L"~%~" + item->GetEnclosureURL(), item->GetEnclosureMIME(), item->GetEnclosureLength());
				item->SetEnclosureDownloaded(true);

				__itemsChanged = true;
				break;
			}
		}
	}
	delete pEnum;
}

void FeedItemCollection::SetItemEnclosureMIME(int itemID, const String &mime) {
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			if (item->GetID() == itemID) {
				item->SetEnclosure(item->GetEnclosureURL(), mime, item->GetEnclosureLength());

				__itemsChanged = true;
				break;
			}
		}
	}
	delete pEnum;
}

void FeedItemCollection::ResetItemEnclosure(int itemID) {
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			if (item->GetID() == itemID) {
				String url = L"";
				String tmp = item->GetEnclosureURL();
				int sepIndex = -1;
				if (!IsFailed(tmp.IndexOf(L"~%~", 0, sepIndex)) && sepIndex >= 0 && (sepIndex + 3) < tmp.GetLength()) {
					tmp.SubString(sepIndex + 3, url);
				}
				item->SetEnclosure(url, item->GetEnclosureMIME(), item->GetEnclosureLength());
				item->SetEnclosureDownloaded(false);

				__itemsChanged = true;
				break;
			}
		}
	}
	delete pEnum;
}

bool FeedItemCollection::ContainsEnclosure(const String &url) const {
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			int sInd = -1;
			if (!IsFailed(item->GetEnclosureURL().IndexOf(url, 0, sInd)) && sInd >= 0) {
				delete pEnum;
				return true;
			}
		}
	}
	delete pEnum;
	return false;
}

void FeedItemCollection::SetAllItemsAsRead(void) {
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);
			if (!item->IsRead()) {
				item->SetRead(true);
				__itemsChanged = true;
			}
		}
	}
	delete pEnum;
	__unreadCount = 0;
}

int FeedItemCollection::SetItemsStatus(const FeedItemCollection &other) {
	if (!other.GetCount()) {
		return GetCount();
	}

	int res = GetCount();
	__unreadCount = GetCount();
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(FeedItem)) {
			FeedItem *item = static_cast<FeedItem *>(obj);

			int otherIndex = -1;
			if (!IsFailed(other.IndexOf(*item, otherIndex)) && otherIndex >= 0) {
				res--;//we've found the same item in previous collection - so it's no longer new

				const Object *otherObj = other.GetAt(otherIndex);
				if (typeid(*otherObj) == typeid(FeedItem)) {
					const FeedItem *otherItem = static_cast<const FeedItem *>(otherObj);

					if (otherItem->IsRead() && !item->IsRead()) {
						item->SetRead(true);
						__itemsChanged = true;
						__unreadCount--;
					}
					if (!otherItem->GetEnclosureURL().IsEmpty() && otherItem->IsEnclosureDownloaded()) {
						item->SetEnclosure(otherItem->GetEnclosureURL(), otherItem->GetEnclosureMIME(), otherItem->GetEnclosureLength());
						item->SetEnclosureDownloaded(true);
					}
				}
			}
		}
	}
	delete pEnum;

	if (__unreadCount < 0) {
		__unreadCount = 0;
	}
	return res;
}

bool FeedItemCollection::Equals(const FeedItemCollection &other) const {
	bool eq = true;
	IEnumerator *pEnum = GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (!other.Contains(*obj)) {
			eq = false;
			break;
		}
	}
	delete pEnum;

	if (eq) {
		pEnum = other.GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			Object *obj = pEnum->GetCurrent();
			if (!Contains(*obj)) {
				eq = false;
				break;
			}
		}
		delete pEnum;
	}
	return eq;
}
