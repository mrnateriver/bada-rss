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
#include "Channel.h"
#include "FeedItem.h"
#include <FXml.h>
#include <FIo.h>
#include <typeinfo>

using namespace Osp::Io;
using namespace Osp::Base::Utility;
using namespace Osp::Xml;

Channel::Channel(const String &title, const String &description, const String &link, const String &url) {
	__id = (int)0xFFFFFFFF;

	__title = title;
	__description = description;
	__link = link;
	__url = url;

	__items = null;

	__pubDate = 0;
	__ttl = 20;
	__faviconPath = L"";
	__pFavicon = null;
}

Channel::Channel(const Channel &other) {
	__id = (int)0xFFFFFFFF;

	FeedItemCollection *items = null;
	if (other.GetItems()) {
		items = new FeedItemCollection;
		items->Construct(other.GetItems()->GetCount());
		items->CopyFrom(*other.GetItems());
	}
	__items = items;

	__faviconPath = other.GetFaviconPath();

	__pFavicon = new Bitmap;
	__pFavicon->Construct(*other.GetFavicon(), Rectangle(0,0,other.GetFavicon()->GetWidth(),other.GetFavicon()->GetHeight()));

	SetFrom(other);
}

void Channel::SetFrom(const Channel &other) {
	__title = other.GetTitle();
	__description = other.GetDescription();
	__link = other.GetLink();
	__url = other.GetURL();

	__pubDate = other.GetPubDate();
	__ttl = other.GetTTL();
}

Channel::~Channel(void) {
	if (__pFavicon) {
		delete __pFavicon;
	}
}

String Channel::GetOPMLOutline(void) const {
	String res = L"<outline title=\"";

	String title = GetTitle();
	title.Replace(L"\"", L"&quot;");
	title.Replace(L"&", L"&amp;");
	res.Append(title);

	res.Append(L"\" text=\"");
	res.Append(title);

	res.Append(L"\" description=\"");

	String desc = GetDescription();
	desc.Replace(L"\"", L"&quot;");
	desc.Replace(L"&", L"&amp;");
	res.Append(desc);

	res.Append(L"\" htmlUrl=\"");
	String lnk = GetLink();
	lnk.Replace(L"&", L"&amp;");
	res.Append(lnk);

	res.Append(L"\" xmlUrl=\"");
	lnk = GetURL();
	lnk.Replace(L"&", L"&amp;");
	res.Append(lnk);

	res.Append(L"\" type=\"rss\" version=\"RSS2\" />");

	return res;
}

String Channel::GetAllStrings(void) const {
	String res = GetTitle();
	res.Append(GetDescription());
	res.Append(GetURL());
	res.Append(GetLink());
	res.Append(Integer::ToString(GetTTL()));

	return res;
}

String Channel::GetFilterStrings(void) const {
	String res = GetTitle();
	res.Append(' ');
	res.Append(GetDescription());
	res.Append(' ');
	res.Append(GetURL());

	return res;
}

String Channel::GetUniqueString(void) const {
	String path = L"";
	path.Append(GetURL().GetHashCode());
	path.Append(L'_');
	path.Append(GetTitle().GetHashCode());
	path.Append(L'_');
	path.Append(GetAllStrings().GetHashCode());
	path.Replace(L'-', L'1');

	return path;
}

bool Channel::Equals(const Object& obj) const {
	if (typeid(obj) != typeid(Channel)) {
		return false;
	} else {
		const Channel *other = static_cast<const Channel *>(&obj);
		return other->GetAllStrings().Equals(GetAllStrings(), false);
		/*if (!GetURL().IsEmpty() && !other->GetURL().IsEmpty()) {
			return GetURL().Equals(other->GetURL(), false);
		} else {
			return other->GetAllStrings().Equals(GetAllStrings(), true);
		}*/
	}
}

Bitmap *Channel::GetFavicon(void) const {
	if (!__pFavicon) {
		__pFavicon = new Bitmap;
		__pFavicon->Construct(*Utilities::GetStaticFavicon(), Rectangle(0,0,Utilities::GetStaticFavicon()->GetWidth(),Utilities::GetStaticFavicon()->GetHeight()));

		return __pFavicon;
	} else {
		return __pFavicon;
	}
}

bool Channel::SetDataFromXML(const char *pData, int len) {
	xmlDocPtr doc = xmlReadMemory(pData, len, null, null, XML_PARSE_NOCDATA);
	if (doc) {
		xmlNodePtr root = xmlDocGetRootElement(doc);
		if (root) {
			if (strcmp((const char*)root->name, "rss") && strcmp((const char*)root->name, "feed")) {
				AppLogDebug("Channel::SetDataFromXML: data is not an RSS/Atom structure");
				xmlFreeDoc(doc);
				return false;
			} else {
				String init_str = GetAllStrings();

				if (!strcmp((const char*)root->name, "rss")) {
					//rss
					result res = E_SUCCESS;
					for (xmlNodePtr pCurElem = root->children; pCurElem; pCurElem = pCurElem->next) {
						if (pCurElem->type == XML_ELEMENT_NODE && !strcmp((const char*)pCurElem->name, "channel")) {
							String title;
							String desc;
							String link;
							int ttl = 20;

							for (xmlNodePtr pCurOutline = pCurElem->children; pCurOutline; pCurOutline = pCurOutline->next) {
								if (pCurOutline->type == XML_ELEMENT_NODE && (pCurOutline->children && (pCurOutline->children->type == XML_TEXT_NODE || pCurOutline->children->type == XML_CDATA_SECTION_NODE))) {
									char *pVal = (char*)pCurOutline->children->content;
									if (pVal && !strlen(pVal)) {
										continue;
									}
									const char *pAttr = (const char*)pCurOutline->name;
									if (!pAttr) {
										continue;
									}

									if (!strcmp(pAttr, "title")) {
										res = StringUtil::Utf8ToString(pVal, title);
									} else if (!strcmp(pAttr, "description")) {
										res = StringUtil::Utf8ToString(pVal, desc);
									} else if (!strcmp(pAttr, "link")) {
										res = StringUtil::Utf8ToString(pVal, link);
									} else if (!strcmp(pAttr, "ttl")) {
										String num;
										res = StringUtil::Utf8ToString(pVal, num);
										if (!IsFailed(res)) {
											res = Integer::Parse(num, ttl);
										}
									}
									if (IsFailed(res)) {
										AppLogDebug("Channel::SetDataFromXML: [%s]: failed to parse value for [%s]", GetErrorMessage(res), pAttr);
									}
								}
							}

							SetTitle(title);
							SetDescription(desc);
							SetLink(link);
							SetTTL(ttl > 180 ? 180 : ttl);

							break;
						}
					}
				} else {
					//atom
					SetTTL(20);

					String title;
					String desc;
					String link;

					result res = E_SUCCESS;
					for (xmlNodePtr pCurElem = root->children; pCurElem; pCurElem = pCurElem->next) {
						if (pCurElem->type == XML_ELEMENT_NODE) {
							const char *pElem = (const char*)pCurElem->name;
							if (!pElem) {
								continue;
							}

							if (pCurElem->children && (pCurElem->children->type == XML_TEXT_NODE || pCurElem->children->type == XML_CDATA_SECTION_NODE)) {
								char *pVal = (char*)pCurElem->children->content;
								if (pVal && !strlen(pVal)) {
									continue;
								}

								if (!strcmp(pElem, "title")) {
									res = StringUtil::Utf8ToString(pVal, title);
								} else if (!strcmp(pElem, "subtitle")) {
									res = StringUtil::Utf8ToString(pVal, desc);
								}
								if (IsFailed(res)) {
									AppLogDebug("Channel::SetDataFromXML: [%s]: failed to parse value for [%s]", GetErrorMessage(res), pElem);
								}
							} else if (!strcmp(pElem, "link")) {
								String type;
								String rel;
								String href;

								for (xmlAttrPtr pCurAttr = pCurElem->properties; pCurAttr; pCurAttr = pCurAttr->next) {
									char *pAttrVal = (char*)pCurAttr->children->content;
									if (pAttrVal && !strlen(pAttrVal)) {
										continue;
									}
									const char *pAttr = (const char*)pCurAttr->name;
									if (!pAttr) {
										continue;
									}

									if (!strcmp(pAttr, "type")) {
										res = StringUtil::Utf8ToString(pAttrVal, type);
									} else if (!strcmp(pAttr, "rel")) {
										res = StringUtil::Utf8ToString(pAttrVal, rel);
									} else if (!strcmp(pAttr, "href")) {
										res = StringUtil::Utf8ToString(pAttrVal, href);
									}
									if (IsFailed(res)) {
										AppLogDebug("Channel::SetDataFromXML: [%s]: failed to parse link attribute value for [%s]", GetErrorMessage(res), pAttr);
									}
								}

								if (type.Equals(L"text/html", false) && rel.Equals(L"alternate", false)) {
									link = href;
								}
							}
						}
					}

					SetTitle(title);
					SetDescription(desc);
					SetLink(link);
				}
				SetPubDate(Utilities::GetCurrentUTCUnixTicks());

				FeedItemCollection *items = new FeedItemCollection;
				items->Construct(20);
				items->SetDataFromXML(pData, len);

				__items = items;

				xmlFreeDoc(doc);
				if (!init_str.Equals(GetAllStrings(), false)) {
					return true;
				} else {
					return false;
				}
			}
		} else {
			xmlFreeDoc(doc);
			return false;
		}
	}
	return false;
}

void Channel::RemoveObsoleteEnclosures(void) {
	Directory *dir = new Directory;
	String path = L"/Home/enclosures/" + GetUniqueString() + L"/";
	if (!IsFailed(dir->Construct(path))) {
		DirEnumerator *pEnum = dir->ReadN();
		if (pEnum) {
			while (!IsFailed(pEnum->MoveNext())) {
				DirEntry entry = pEnum->GetCurrentDirEntry();
				if (GetItems() && !GetItems()->ContainsEnclosure(path + entry.GetName())) {
					File::Remove(path + entry.GetName());
				}
			}
			delete pEnum;
		}
	}
	delete dir;
}
