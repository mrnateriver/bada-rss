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

#include "FeedItem.h"
#include <typeinfo>

using namespace Osp::Base::Utility;

FeedItem::FeedItem(const String &title, const String &content) {
	__id = (int)0xFFFFFFFF;
	__read = false;

	__title = title;
	__content = content;

	__guidLink = L"";
	__pubDate = 0;

	__enclosureDownloaded = false;
	__enclosureURL = L"";
	__enclosureMIME = L"";
	__enclosureLength = 0;
}

FeedItem::FeedItem(const FeedItem &other) {
	__id = (int)0xFFFFFFFF;
	__read = false;

	__title = other.GetTitle();
	__content = other.GetContent();

	__guidLink = other.GetGuidLink();
	__pubDate = other.GetPubDate();

	__enclosureDownloaded = other.IsEnclosureDownloaded();
	__enclosureURL = other.GetEnclosureURL();
	__enclosureMIME = other.GetEnclosureMIME();
	__enclosureLength = other.GetEnclosureLength();
}

void FeedItem::SetEnclosure(const String &url, const String &mime, int len) {
	__enclosureURL = url;
	__enclosureMIME = mime;
	__enclosureLength = len;
}

String FeedItem::GetAllStrings(void) const {
	String res = GetTitle();
	res.Append(GetContent());
	res.Append(GetGuidLink());
	//we don't want to compare pubdate because the same item can be updated with a different one
	//res.Append(LongLong::ToString(GetPubDate()));

	return res;
}

String FeedItem::GetFilterStrings(void) const {
	String res = GetTitle();
	res.Append(' ');
	res.Append(GetContent());
	res.Append(' ');
	res.Append(GetGuidLink());

	return res;
}

String FeedItem::GetUniqueString(void) const {
	String path = L"";
	path.Append(GetGuidLink().GetHashCode());
	path.Append(L'_');
	path.Append(GetTitle().GetHashCode());
	path.Append(L'_');
	path.Append(GetAllStrings().GetHashCode());
	path.Replace(L'-', L'1');

	return path;
}

bool FeedItem::Equals(const Object& obj) const {
	if (typeid(obj) != typeid(FeedItem)) {
		return false;
	} else {
		const FeedItem *other = static_cast<const FeedItem *>(&obj);
		return other->GetAllStrings().Equals(GetAllStrings(), true);
	}
}

void FeedItem::SetDataFromXML(const xmlNode &item_node, GregorianCalendar *calendar) {
	String title;
	String content;
	String guidLink;

	FeedItem before = *this;

	result res = E_SUCCESS;
	if (!strcmp((const char*)item_node.name, "item")) {
		String pubDate;
		String link;

		String enc_url;
		String enc_mime;
		int enc_len = 0;

		//rss
		for (xmlNodePtr pCurNode = item_node.children; pCurNode; pCurNode = pCurNode->next) {
			if (pCurNode->type == XML_ELEMENT_NODE) {
				const char *pElem = (const char*)pCurNode->name;
				if (!pElem) {
					continue;
				}

				if (!strcmp(pElem, "enclosure")) {
					//parsing enclosure
					for (xmlAttrPtr pCurAttr = pCurNode->properties; pCurAttr; pCurAttr = pCurAttr->next) {
						char *pAttrVal = (char*)pCurAttr->children->content;
						if (pAttrVal && !strlen(pAttrVal)) {
							continue;
						}
						const char *pAttr = (const char*)pCurAttr->name;
						if (!pAttr) {
							continue;
						}

						if (!strcmp(pAttr, "url")) {
							res = StringUtil::Utf8ToString(pAttrVal, enc_url);
						} else if (!strcmp(pAttr, "length")) {
							String tmp;
							res = StringUtil::Utf8ToString(pAttrVal, tmp);
							if (!IsFailed(res)) {
								tmp.Trim();
								res = Integer::Parse(tmp, enc_len);
							}
						} else if (!strcmp(pAttr, "type")) {
							res = StringUtil::Utf8ToString(pAttrVal, enc_mime);
						}
						if (IsFailed(res)) {
							AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse enclosure attribute value for [%s]", GetErrorMessage(res), pAttr);
						}
					}
				} else if (pCurNode->children && (pCurNode->children->type == XML_TEXT_NODE || pCurNode->children->type == XML_CDATA_SECTION_NODE)) {
					char *pVal = (char*)pCurNode->children->content;
					if (pVal && !strlen(pVal)) {
						continue;
					}

					if (!strcmp(pElem, "title")) {
						res = StringUtil::Utf8ToString(pVal, title);
					} else if (!strcmp(pElem, "description")) {
						res = StringUtil::Utf8ToString(pVal, content);
					} else if (!strcmp(pElem, "guid")) {
						String permaLink = L"true";

						for (xmlAttrPtr pCurAttr = pCurNode->properties; pCurAttr; pCurAttr = pCurAttr->next) {
							char *pAttrVal = (char*)pCurAttr->children->content;
							if (pAttrVal && !strlen(pAttrVal)) {
								continue;
							}
							const char *pAttr = (const char*)pCurAttr->name;
							if (!pAttr) {
								continue;
							}

							if (!strcmp(pAttr, "isPermaLink")) {
								res = StringUtil::Utf8ToString(pAttrVal, permaLink);
							}
							if (IsFailed(res)) {
								AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse guid attribute value for [%s]", GetErrorMessage(res), pAttr);
							} else {
								break;
							}
						}

						if (permaLink.Equals(L"true", false)) {
							res = StringUtil::Utf8ToString(pVal, guidLink);
						} else {
							res = E_SUCCESS;
						}
					} else if (!strcmp(pElem, "link")) {
						res = StringUtil::Utf8ToString(pVal, link);
					} else if (!strcmp(pElem, "pubDate")) {
						res = StringUtil::Utf8ToString(pVal, pubDate);
					}
					//add support for 'media' extension xmlns maybe?
					if (IsFailed(res)) {
						AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse value for [%s]", GetErrorMessage(res), pElem);
					}
				}
			}
		}
		if (guidLink.IsEmpty()) {
			guidLink = link;
		}
		if (!pubDate.IsEmpty()) {
			DateTime pubDT = Utilities::ParseRFC822DateTimeToUTC(pubDate);
			if (!IsFailed(GetLastResult())) {
				//SetPubDate(Utilities::GetUTCUnixTicksFromUTCDateTime(pubDT));
				calendar->SetTime(pubDT);
				//set pubdate
				calendar->GetTimeInMillisecFromEpoch(__pubDate);
			} else {
				AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse pubDate: [%S]", GetErrorMessage(GetLastResult()), pubDate.GetPointer());
				SetLastResult(E_SUCCESS);
				__pubDate = 0;
			}
		}
		if (!enc_url.IsEmpty() && !enc_mime.IsEmpty()) {
			if (enc_mime.StartsWith(L"image/", 0)) {
				content.Append(L"<br /><img src=\"");
				content.Append(enc_url);
				content.Append(L"\" alt=\"enclosure\" />");
			} else {
				SetEnclosure(enc_url, enc_mime, enc_len);
			}
		}
	} else if (!strcmp((const char*)item_node.name, "entry")) {
		String summary;
		String published;
		String updated;

		//atom
		for (xmlNodePtr pCurNode = item_node.children; pCurNode; pCurNode = pCurNode->next) {
			if (pCurNode->type == XML_ELEMENT_NODE) {
				const char *pElem = (const char*)pCurNode->name;
				if (!pElem) {
					continue;
				}

				if (!strcmp(pElem, "content")) {
					String type;
					String src;

					for (xmlAttrPtr pCurAttr = pCurNode->properties; pCurAttr; pCurAttr = pCurAttr->next) {
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
						} else if (!strcmp(pAttr, "src")) {
							res = StringUtil::Utf8ToString(pAttrVal, src);
						}
						if (IsFailed(res)) {
							AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse link attribute value for [%s]", GetErrorMessage(res), pAttr);
						}
					}

					if (type.Equals(L"text", false) || type.Equals(L"html", false)
						|| type.Equals(L"xhtml", false) || type.StartsWith(L"text/", 0)) {
						char *pVal = (char*)pCurNode->children->content;
						if (pVal && !strlen(pVal)) {
							continue;
						}

						res = StringUtil::Utf8ToString(pVal, content);
						if (IsFailed(res)) {
							AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse content for atom entry", GetErrorMessage(res));
						}
					} else if (!type.IsEmpty()) {
						//parsing enclosure
						if (src.IsEmpty()) {
							//decode base64 content here
						} else {
							if (type.StartsWith(L"image/", 0)) {
								content.Append(L"<br /><img src=\"");
								content.Append(src);
								content.Append(L"\" alt=\"enclosure\" />");
							} else {
								SetEnclosure(src, type);
							}
						}
					}
				} else if (pCurNode->children && (pCurNode->children->type == XML_TEXT_NODE || pCurNode->children->type == XML_CDATA_SECTION_NODE)) {
					char *pVal = (char*)pCurNode->children->content;
					if (pVal && !strlen(pVal)) {
						continue;
					}

					if (!strcmp(pElem, "title")) {
						res = StringUtil::Utf8ToString(pVal, title);
					} else if (!strcmp(pElem, "summary")) {
						res = StringUtil::Utf8ToString(pVal, summary);
					} else if (!strcmp(pElem, "published")) {
						res = StringUtil::Utf8ToString(pVal, published);
					} else if (!strcmp(pElem, "updated")) {
						res = StringUtil::Utf8ToString(pVal, updated);
					}
					if (IsFailed(res)) {
						AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse value for [%s]", GetErrorMessage(res), pElem);
					}
				} else if (!strcmp(pElem, "link")) {
					String type;
					String rel;
					String href;

					for (xmlAttrPtr pCurAttr = pCurNode->properties; pCurAttr; pCurAttr = pCurAttr->next) {
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
							AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse link attribute value for [%s]", GetErrorMessage(res), pAttr);
						}
					}
					if (type.Equals(L"text/html", false) && rel.Equals(L"alternate", false)) {
						guidLink = href;
					}
				}
			}
		}

		if (title.IsEmpty()) {
			int bracketIndex = -1;
			while (!IsFailed(summary.IndexOf(L"<", 0, bracketIndex)) && bracketIndex >= 0) {
				int closingBracket = -1;
				if (!IsFailed(summary.IndexOf(L">", bracketIndex, closingBracket)) && closingBracket > bracketIndex) {
					summary.Remove(bracketIndex, closingBracket - bracketIndex + 1);
				}
			}
			summary.Replace(L"\n", L"");

			title = summary;
		}

		DateTime pubDT;
		if (published.IsEmpty()) {
			pubDT = Utilities::ParseRFC3339DateTimeToUTC(updated);
		} else {
			pubDT = Utilities::ParseRFC3339DateTimeToUTC(published);
		}
		if (!IsFailed(GetLastResult())) {
			//SetPubDate(Utilities::GetUTCUnixTicksFromUTCDateTime(pubDT));
			calendar->SetTime(pubDT);
			//set pubdate
			calendar->GetTimeInMillisecFromEpoch(__pubDate);
		} else {
			AppLogDebug("FeedItem::SetDataFromXML: [%s]: failed to parse atom date: [%S]", GetErrorMessage(GetLastResult()), published.IsEmpty() ? updated.GetPointer() : published.GetPointer());
			SetLastResult(E_SUCCESS);
			__pubDate = 0;
		}
	} else {
		AppLogDebug("FeedItem::SetDataFromXML: passed xml node is not an RSS/Atom entry");
	}

	SetTitle(title);
	SetContent(content);
	SetGuidLink(guidLink);

	int bracketIndex = -1;
	while (!IsFailed(__title.IndexOf(L"<", 0, bracketIndex)) && bracketIndex >= 0) {
		int closingBracket = -1;
		if (!IsFailed(__title.IndexOf(L">", bracketIndex, closingBracket)) && closingBracket > bracketIndex) {
			__title.Remove(bracketIndex, closingBracket - bracketIndex + 1);
		}
	}

	if (before.Equals(*this)) {
		AppLogDebug("FeedItem::SetDataFromXML: some errors occured while parsing RSS/Atom entry, or parsed entry is the same as current");
		SetLastResult(E_PARSING_FAILED);
	}
}
