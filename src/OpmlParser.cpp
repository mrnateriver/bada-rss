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
#include "OpmlParser.h"
#include <FIo.h>
#include <FContent.h>
#include <typeinfo>

using namespace Osp::Base::Utility;
using namespace Osp::Io;
using namespace Osp::Content;

ArrayList *OPMLParser::ParseOPMLBodyN(xmlNodePtr pBodyNode) const {
	ArrayList *pList = new ArrayList;
	result res = pList->Construct(5);
	if (IsFailed(res)) {
		AppLogDebug("OPMLParser::ParseOPMLBody: [%s]: failed to construct array list", GetErrorMessage(res));
		delete pList;
		SetLastResult(res);
		return null;
	}

	for (xmlNodePtr pCurOutline = pBodyNode->children; pCurOutline; pCurOutline = pCurOutline->next) {
		if (!strcmp((const char*)pCurOutline->name, "outline")) {
			bool got_type = false;
			String title;
			String desc;
			String text;
			String link;
			String url;

			for (xmlAttrPtr pCurAttr = pCurOutline->properties; pCurAttr; pCurAttr = pCurAttr->next) {
				char *pVal = (char*)pCurAttr->children->content;
				if (pVal && !strlen(pVal)) {
					continue;
				}
				const char *pAttr = (const char*)pCurAttr->name;
				if (!pAttr) {
					continue;
				}

				if (!strcmp(pAttr, "type")) {
					String type;
					res = StringUtil::Utf8ToString(pVal, type);
					if (IsFailed(res) || !type.Equals(L"rss", false)) {
						AppLogDebug("OPMLParser::ParseOutline: [%s]: failed to parse outline type or it's not an RSS one", GetErrorMessage(res));
					} else {
						got_type = true;
					}
				} else if (!strcmp(pAttr, "title")) {
					res = StringUtil::Utf8ToString(pVal, title);
				} else if (!strcmp(pAttr, "description")) {
					res = StringUtil::Utf8ToString(pVal, desc);
				} else if (!strcmp(pAttr, "text")) {
					res = StringUtil::Utf8ToString(pVal, text);
				} else if (!strcmp(pAttr, "htmlUrl")) {
					res = StringUtil::Utf8ToString(pVal, link);
				} else if (!strcmp(pAttr, "xmlUrl")) {
					res = StringUtil::Utf8ToString(pVal, url);
				}
				if (IsFailed(res)) {
					AppLogDebug("OPMLParser::ParseOutline: [%s]: failed to parse outline attribute value for [%s]", GetErrorMessage(res), pAttr);
				}
			}

			if (got_type) {
				if (!url.IsEmpty()) {
					if (!url.StartsWith(L"http://", 0)) {
						url.Insert(L"http://", 0);
					}
					if (url.EndsWith(L"/")) {
						url.Remove(url.GetLength() - 1, 1);
					}
					Channel *pChan = new Channel(title, desc.IsEmpty() ? text : desc, link, url);
					pList->Add(*pChan);
				}
			} else {
				ArrayList *pIntList = ParseOPMLBodyN(pCurOutline);
				if (!pIntList) {
					AppLogDebug("OPMLParser::ParseOutline: failed to parse inlined outline");
				} else {
					pList->AddItems(*pIntList);
					delete pIntList;
				}
			}
		}
	}

	SetLastResult(E_SUCCESS);
	return pList;
}

ArrayList *OPMLParser::ParseFileN(const String &opmlFile) const {
	if (!File::IsFileExist(opmlFile)) {
		AppLogDebug("OPMLParser::ParseFileN: file doesn't exist");
		SetLastResult(E_FILE_NOT_FOUND);
		return null;
	}

	File *file = new File;
	result res = file->Construct(opmlFile, L"r", false);
	if (IsFailed(res)) {
		AppLogDebug("OPMLParser::ParseFileN: [%s]: failed to open file for parsing", GetErrorMessage(res));
		SetLastResult(res);
		delete file;
		return null;
	}

	FileAttributes attrs;
	res = File::GetAttributes(opmlFile, attrs);
	if (IsFailed(res)) {
		AppLogDebug("OPMLParser::ParseFileN: [%s]: failed to get file attributes for parsing", GetErrorMessage(res));
		SetLastResult(res);
		delete file;
		return null;
	}

	ByteBuffer *pBuf = new ByteBuffer;
	pBuf->Construct((int)attrs.GetFileSize());

	file->Read(*pBuf);
	delete file;
	file = null;
	pBuf->Rewind();

	byte *cArray = new byte[11];
	pBuf->GetArray((byte*)cArray, 0, 10);
	cArray[10] = 0;

	String check = String((char*)cArray);
	if (!check.StartsWith(L"<?xml", 0) && !check.StartsWith(L"<opml", 0)) {
		SetLastResult(E_INVALID_FORMAT);
		delete []cArray;
		delete pBuf;
		return null;
	}
	delete []cArray;
	pBuf->Rewind();

	xmlDocPtr pDocument = xmlReadMemory((const char*)pBuf->GetPointer(), pBuf->GetLimit(), null, null, XML_PARSE_NOCDATA | XML_PARSE_NOENT | XML_PARSE_RECOVER);//xmlParseFile((const char*)pBuf->GetPointer());
	if (!pDocument) {
		AppLogDebug("OPMLParser::ParseFileN: failed to parse file, xml error: [%d], line: [%d]", xmlLastError.code, xmlLastError.line);
		SetLastResult(E_PARSING_FAILED);
		delete pBuf;
		return null;
	}

	xmlNodePtr pRoot = xmlDocGetRootElement(pDocument);
	if (strcmp((const char*)pRoot->name, "opml")) {
		AppLogDebug("OPMLParser::ParseFileN: file is not an OPML file");
		xmlFreeDoc(pDocument);
		SetLastResult(E_INVALID_FORMAT);
		delete pBuf;
		return null;
	}

	ArrayList *pList = null;
	for (xmlNodePtr pCurElem = pRoot->children; pCurElem; pCurElem = pCurElem->next) {
		if (pCurElem->type == XML_ELEMENT_NODE && !strcmp((const char*)pCurElem->name, "body")) {
			pList = ParseOPMLBodyN(pCurElem);
			break;
		}
	}
	xmlFreeDoc(pDocument);
	delete pBuf;

	if (!pList) {
		AppLogDebug("OPMLParser::ParseFileN: [%s]: failed to parse body", GetErrorMessage(GetLastResult()));
		return null;
	} else {
		SetLastResult(E_SUCCESS);
		return pList;
	}
}

result OPMLParser::SaveToFile(const String &opmlFile, const ArrayList &channels) const {
	int slashIndex = 0;
	if (IsFailed(opmlFile.LastIndexOf(L'/', opmlFile.GetLength() - 1, slashIndex))) {
		AppLogDebug("OPMLParser::SaveToFile: wrong path specified");
		return E_INVALID_FORMAT;
	}

	String filename;
	if (IsFailed(opmlFile.SubString(slashIndex + 1, filename))) {
		AppLogDebug("OPMLParser::SaveToFile: wrong path specified");
		return E_INVALID_FORMAT;
	}
	filename.Insert(L"/Home/temporary/", 0);

	File *file = new File;
	result res = file->Construct(filename, L"w", true);
	if (IsFailed(res)) {
		AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to open file for writing", GetErrorMessage(res));
		delete file;
		return E_INVALID_FORMAT;
	}

	String header = L"<?xml version=\"1.0\" encoding=\"UTF-8\"?><opml version=\"2.0\"><head><title>Feeds exported from Aggregator ";
	header.Append(Utilities::GetAppVersionString());
	header.Append(L"</title><dateCreated>");
	header.Append(Utilities::FormatUTCDateTimeRFC822(Utilities::GetCurrentUTCDateTime()));
	header.Append(L"</dateCreated></head><body>");

	res = file->Write(header);
	if (IsFailed(res)) {
		AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to write header", GetErrorMessage(res));
		delete file;
		return res;
	}

	String buffer;
	int counter = 0;
	IEnumerator *pEnum = channels.GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (obj && (typeid(*obj) == typeid(Channel))) {
			Channel *chan = static_cast<Channel *>(obj);

			buffer.Append(chan->GetOPMLOutline());
			counter++;
			if (counter >= 5) {
				res = file->Write(buffer);
				if (IsFailed(res)) {
					AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to write outline", GetErrorMessage(res));
					delete pEnum;
					delete file;
					return res;
				}
				buffer.Clear();
			}
		}
	}
	if (!buffer.IsEmpty()) {
		res = file->Write(buffer);
		if (IsFailed(res)) {
			AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to write outline", GetErrorMessage(res));
			delete pEnum;
			delete file;
			return res;
		}
	}
	delete pEnum;

	res = file->Write(L"</body></opml>");
	if (IsFailed(res)) {
		AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to write body closing", GetErrorMessage(res));
		delete file;
		return res;
	}

	res = file->Flush();
	if (IsFailed(res)) {
		AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to flush buffer", GetErrorMessage(res));
		delete file;
		return res;
	}
	delete file;

	if (opmlFile.StartsWith(L"/Home/", 0)) {
		if (File::IsFileExist(opmlFile)) {
			res = File::Remove(opmlFile);
			if (IsFailed(res)) {
				AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to overwrite existing file", GetErrorMessage(res));
				File::Remove(filename);
				return res;
			}
		}

		res = File::Move(filename, opmlFile);
		if (IsFailed(res)) {
			AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to move temporary file to destination", GetErrorMessage(res));
			File::Remove(filename);
			return res;
		}
	} else {
		if (File::IsFileExist(opmlFile)) {
			OtherContentInfo info;
			res = info.Construct(opmlFile);
			if (IsFailed(res)) {
				AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to construct content info for overwriting file", GetErrorMessage(res));
				File::Remove(filename);
				return res;
			}

			ContentManager conMgr;
			res = conMgr.Construct();
			if (IsFailed(res)) {
				AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to construct content manager for overwriting file", GetErrorMessage(res));
				File::Remove(filename);
				return res;
			}

			ContentId conID = conMgr.CreateContent(info);
			res = GetLastResult();
			if (IsFailed(res)) {
				AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to create content for overwriting file", GetErrorMessage(res));
				File::Remove(filename);
				return res;
			}

			res = conMgr.DeleteContent(conID);
			if (IsFailed(res)) {
				AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to overwrite existing file", GetErrorMessage(res));
				File::Remove(filename);
				return res;
			}
		}

		res = ContentManagerUtil::MoveToMediaDirectory(filename, opmlFile);
		if (IsFailed(res)) {
			AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to move temporary file to destination", GetErrorMessage(res));
			File::Remove(filename);
			return res;
		}

		OtherContentInfo info;
		res = info.Construct(opmlFile);
		if (IsFailed(res)) {
			AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to construct content info", GetErrorMessage(res));
			return res;
		}

		ContentManager conMgr;
		res = conMgr.Construct();
		if (IsFailed(res)) {
			AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to construct content manager", GetErrorMessage(res));
			return res;
		}

		conMgr.CreateContent(info);
		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogDebug("OPMLParser::SaveToFile: [%s]: failed to create content entry", GetErrorMessage(res));
			return res;
		}
	}

	return E_SUCCESS;
}
